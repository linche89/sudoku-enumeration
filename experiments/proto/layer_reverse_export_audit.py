"""Independent streaming readback of CLOSED T5 export against audited F5 chunks.

Checks every original key, every stabilizer, every weighted value, full SHA
and its physical copy. No recurrence or native producer reader is called.
"""
import argparse
import array
import hashlib
import json
import os
from pathlib import Path
import struct
import sys
import threading
import time
import psutil
from layer_reverse_complete_audit import ROOT,N,L5SHA,native_header,require,checksum,header

p=argparse.ArgumentParser(description=__doc__)
for name in ('input','backup','namespace','support','chunk_audit','output'):
    p.add_argument('--'+name.replace('_','-'),type=Path,required=True)
p.add_argument('--expected-sha256',required=True)
p.add_argument('--checkpointreadonly',action='store_true',required=True)
a=p.parse_args()
require(a.output.resolve().is_relative_to(ROOT/'data/logs') and not a.output.exists(),'fresh output')
began=time.monotonic(); done=threading.Event(); peak=[0]
def guard():
    proc=psutil.Process()
    while not done.wait(.2):
        peak[0]=max(peak[0],proc.memory_info().rss)
        if time.monotonic()-began>180 or peak[0]>1<<30:
            print('EXPORT_AUDIT_GUARD_STOP',flush=True);os._exit(98)
threading.Thread(target=guard,daemon=True).start()
audit=json.loads(a.chunk_audit.read_text())
require(audit['status']=='PASS' and audit['entries']==N and Path(audit['namespace']).resolve()==a.namespace.resolve(),'bound prior chunk audit')
h=native_header(a.input); old=native_header(a.support)
config=checksum(struct.pack('<6Q',0x4C445043414E3031,6,12,24,0,0))
require(h[3]==5 and h[4]==0 and h[5]==config and h[7]==N and h[8]==221 and h[15]==0,'plain complete production L5')
require(old[7]==N-2 and old[3]==5,'support shape')
for path,wanted in ((a.input,a.expected_sha256),(a.backup,a.expected_sha256),(a.support,L5SHA)):
    with path.open('rb') as stream:
        require(hashlib.file_digest(stream,'sha256').hexdigest().upper()==wanted.upper(),'full SHA '+str(path))
with a.input.open('rb') as dest,a.support.open('rb') as source:
    dest.seek(128);source.seek(128);left=24*(N-2)
    while left:
        size=min(left,8<<20);require(dest.read(size)==source.read(size),'original key prefix differs');left-=size
mass=live=0;final_stabs=[]
with a.input.open('rb') as weights,a.input.open('rb') as stabilizers,a.support.open('rb') as original:
    weights.seek(128+24*N);stabilizers.seek(128+32*N);original.seek(128+32*(N-2))
    for begin in range(0,N,10000):
        count=min(10000,N-begin)
        raw=(a.namespace/f'f5-chunk-{begin:016x}.bin').read_bytes();ch=header(raw)
        require(ch['begin']==begin and ch['count']==count,'chunk geometry changed')
        require(checksum(raw[256:])==ch['payload_hash'],'chunk payload changed')
        fv=array.array('Q',raw[256:]);tt=array.array('Q',weights.read(8*count));ss=array.array('I',stabilizers.read(4*count))
        old_count=max(0,min(count,N-2-begin));oo=array.array('I',original.read(4*old_count))
        if sys.byteorder!='little':
            for row in (fv,tt,ss,oo):row.byteswap()
        if begin+count>N-2:oo.extend((1440,240));final_stabs=list(ss[-2:])
        require(ss==oo and len(tt)==len(fv)==count,'stab/length mismatch')
        for f,t,s in zip(fv,tt,ss):
            if not s:require(f==t==0,'nonzero hole');continue
            require(s>0 and 46080%s==0 and f>0 and f%120==0 and t==f*(46080//s),'weighted value mismatch')
            live+=1;mass+=46080//s
        if begin%10000000==0:print(f'EXPORT_VALUES_CHECKED {begin+count}/{N}',flush=True)
require(live==96452755 and mass==4439972139072,'complete mass/live')
result=dict(status='PASS',scope='all original keys, all repaired stabilizers and T5=m*F5 values; not independent F5 numerical reevaluation',
            entries=N,live=live,holes=221,mass=mass,sha256=a.expected_sha256.upper(),bytes=a.input.stat().st_size,
            input=str(a.input.resolve()),backup=str(a.backup.resolve()),seconds=time.monotonic()-began,
            guard_seconds=180,rss_limit_bytes=1<<30,sampled_peak_rss_bytes=peak[0],appended_stabilizers=final_stabs,
            chunk_audit_sha256=hashlib.sha256(a.chunk_audit.read_bytes()).hexdigest().upper())
with a.output.open('x',encoding='utf-8') as stream:json.dump(result,stream,indent=2)
done.set();print(json.dumps(result),flush=True)
