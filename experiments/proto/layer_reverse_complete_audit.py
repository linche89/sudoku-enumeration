"""Read-only independent RVF5 chunk audit; never recomputes F5 values.

Uses the previously independently implemented Python checksum, not the C++
producer/reader. Optional JSON output is exclusive and must be under data/logs.
"""
import argparse
import array
import csv
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import sys
import threading
import time

import psutil
from layer_shared_checkpoint_audit import checksum, MASK

ROOT = Path(__file__).resolve().parents[2]
N = 96452976
L4SHA = '7BA5E5BA3255DD17851043521F67FB4EE70F76AE565FD6CA9AD962E7D5014D94'
L5SHA = 'A5FDDB22F8C79FD4DDC0015795558FBF1647F0DB556248F5C9551F2DB24891DF'
FIELDS = ('semantics','l4_header','support_header','repair','entries','chunk',
          'begin','count','payload_hash','header_hash','live','value_sum','reserved0','reserved1')


def require(ok, why):
    if not ok:
        raise ValueError(why)


def header(raw):
    require(len(raw) >= 256, 'truncated header')
    h = dict(zip(FIELDS, struct.unpack_from('<14Q', raw, 144)))
    h.update(magic=raw[:8], version=struct.unpack_from('<I',raw,8)[0],
             c=struct.unpack_from('<I',raw,12)[0], l4sha=raw[16:80], supportsha=raw[80:144])
    candidate = bytearray(raw[:256]); struct.pack_into('<Q',candidate,216,0)
    require(h['magic'] in (b'RVF5MT01',b'RVF5CK01') and h['version']==1 and h['c'] in (5,6), 'header domain')
    require(h['semantics']==0x313044544f4f5235 and not h['reserved0'] and not h['reserved1'], 'semantics/reserved')
    require(checksum(candidate)==h['header_hash'], 'header checksum')
    require(all(re.fullmatch(b'[A-F0-9]{64}',h[k]) for k in ('l4sha','supportsha')), 'SHA shape')
    require(0 < h['entries'] < 2**32-64 and 0 < h['chunk'] <= 1000000 and
            0 <= h['begin'] <= h['entries'] and h['count'] <= h['entries']-h['begin'], 'header range')
    return h


def values(raw, h, stabs):
    require(len(raw)==256+8*h['count'] and len(stabs)==h['count'], 'payload length')
    payload=raw[256:]
    require(checksum(payload)==h['payload_hash'], 'payload checksum')
    vv=array.array('Q',payload)
    if sys.byteorder!='little': vv.byteswap()
    live=0; total=0
    for value,stab in zip(vv,stabs):
        require(bool(value)==bool(stab) and value%120==0, 'value/hole/factorial')
        if value: live+=1; total+=value
    require(live==h['live'] and total&MASK==h['value_sum'], 'payload counters')
    return live,total&MASK


def native_header(path):
    with path.open('rb') as stream: raw=stream.read(128)
    require(len(raw)==128, 'native header length')
    h=struct.unpack('<QIIII13Q',raw)
    check=bytearray(raw); struct.pack_into('<Q',check,120,0)
    require(h[0]==0x314B434C4A464453 and h[1]==2 and h[2]==6 and checksum(check)==h[-1], 'native header binding')
    require(path.stat().st_size==128+36*h[7]+16*h[15], 'native image length')
    return h


def self_test():
    raw=bytearray(280); raw[:8]=b'RVF5CK01'; struct.pack_into('<II',raw,8,1,6)
    raw[16:80]=L4SHA.encode(); raw[80:144]=L5SHA.encode()
    payload=struct.pack('<3Q',120,0,240); raw[256:]=payload
    fields=[0x313044544f4f5235,11,22,33,3,3,0,3,checksum(payload),0,2,360,0,0]
    struct.pack_into('<14Q',raw,144,*fields); struct.pack_into('<Q',raw,216,checksum(raw[:256]))
    require(values(raw,header(raw),[1,0,1])==(2,360), 'valid fixture')
    rejected=0
    for offset in (0,8,12,16,80,144,176,192,216,240,256):
        bad=bytearray(raw); bad[offset]^=1
        try: values(bad,header(bad),[1,0,1])
        except ValueError: rejected+=1
        else: raise ValueError('corruption accepted')
    for stabs in ([0,1,1],[1,0]):
        try: values(raw,header(raw),stabs)
        except ValueError: rejected+=1
        else: raise ValueError('wrong support accepted')
    print(f'REVERSE_AUDIT_SELF_TEST_PASS accepted=1 rejected={rejected}',flush=True)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--self-test',action='store_true')
    p.add_argument('--namespace',type=Path)
    p.add_argument('--backup',type=Path)
    p.add_argument('--l4',type=Path)
    p.add_argument('--support',type=Path)
    p.add_argument('--summary',type=Path)
    p.add_argument('--output',type=Path)
    p.add_argument('--checkpointreadonly',action='store_true')
    a=p.parse_args()
    self_test()
    if a.self_test: return
    require(a.checkpointreadonly and all((a.namespace,a.backup,a.l4,a.support,a.summary,a.output)), 'explicit read-only inputs required')
    require(a.output.resolve().is_relative_to(ROOT/'data/logs') and not a.output.exists(), 'fresh log output required')
    began=time.monotonic(); done=threading.Event(); peak=[0]
    def guard():
        proc=psutil.Process()
        while not done.wait(.2):
            peak[0]=max(peak[0],proc.memory_info().rss)
            if time.monotonic()-began>180 or peak[0]>1<<30:
                print('AUDIT_GUARD_STOP no certificate',flush=True); os._exit(98)
    threading.Thread(target=guard,daemon=True).start()
    l4=native_header(a.l4); support=native_header(a.support)
    require(l4[3]==4 and l4[4]==0 and support[3]==5 and support[7]==N-2 and support[8]==221, 'input geometry')
    with a.support.open('rb') as stream:
        require(hashlib.file_digest(stream,'sha256').hexdigest().upper()==L5SHA, 'support full SHA')
    with (a.backup/'receipt.csv').open(encoding='utf-8-sig',newline='') as stream: rows=list(csv.DictReader(stream))
    receipt={r['Name']:r for r in rows}
    expected={'manifest.bin'}|{f'f5-chunk-{i:016x}.bin' for i in range(0,N,10000)}
    require(len(receipt)==len(rows)==len(expected) and set(receipt)==expected, 'receipt inventory')
    for base in (a.namespace,a.backup):
        names={x.name for x in base.iterdir() if x.name=='manifest.bin' or re.fullmatch(r'f5-chunk-[0-9a-f]{16}\.bin',x.name)}
        require(names==expected,'committed inventory differs')
    manifest=None; prefix=live=total=byte_count=0
    names=['manifest.bin']+sorted(expected-{'manifest.bin'})
    with a.support.open('rb') as source:
        source.seek(128+32*(N-2)) # native keys, T, then u32 stabilizers
        for ix,name in enumerate(names):
            path=a.namespace/name
            raw=path.read_bytes(); copied=(a.backup/name).read_bytes()
            digest=hashlib.sha256(raw).hexdigest().upper()
            require(raw==copied and digest==receipt[name]['SHA256'] and len(raw)==int(receipt[name]['Bytes']), 'backup/receipt mismatch '+name)
            h=header(raw); byte_count+=len(raw)
            require(h['c']==6 and h['entries']==N and h['chunk']==10000 and h['repair']==11401178190082244558 and
                    h['l4sha']==L4SHA.encode() and h['supportsha']==L5SHA.encode() and
                    h['l4_header']==l4[-1] and h['support_header']==support[-1], 'pinned lineage')
            if manifest is None:
                require(h['magic']==b'RVF5MT01' and len(raw)==256 and all(h[k]==0 for k in ('begin','count','payload_hash','live','value_sum')), 'manifest geometry')
                manifest=h; continue
            require(h['magic']==b'RVF5CK01' and h['begin']==prefix and h['count']==min(10000,N-prefix), 'chunk continuity')
            old_count=max(0,min(h['count'],N-2-prefix))
            stabs=array.array('I',source.read(4*old_count))
            if sys.byteorder!='little': stabs.byteswap()
            if prefix+h['count']>N-2: stabs.extend((1440,240))
            ll,ss=values(raw,h,stabs); live+=ll;total=(total+ss)&MASK;prefix+=h['count']
            if ix%1000==0: print(f'AUDITED prefix={prefix}/{N} seconds={time.monotonic()-began:.3f}',flush=True)
    summary_lines=[s for s in a.summary.read_text().splitlines() if s.startswith('SUMMARY ')]
    require(len(summary_lines)==1,'unique native summary')
    terminal=dict(re.findall(r'(\w+)=([^\s]+)',summary_lines[0]))
    require(terminal.get('status')=='CLOSED_F5_CATALOGUE' and int(terminal['closed_prefix'])==prefix==N and
            int(terminal['live_closed'])==live==96452755 and int(terminal['F5_checksum_mod2_64'])==total and
            terminal['source_checkpointreadonly']=='yes' and terminal['N6']=='NOT_COMPUTED','complete scope/counters')
    result=dict(status='PASS',scope='complete F5 byte/lineage/hole/value-shape integrity; NOT independent F5 numerical reevaluation',
                entries=prefix,live=live,holes=N-live,checksum_mod2_64=total,files=len(names),bytes_per_copy=byte_count,
                namespace=str(a.namespace.resolve()),backup=str(a.backup.resolve()),seconds=time.monotonic()-began,
                sampled_peak_rss_bytes=peak[0],guard_seconds=180,rss_limit_bytes=1<<30,
                manifest_sha256=hashlib.sha256((a.namespace/'manifest.bin').read_bytes()).hexdigest().upper(),
                backup_receipt_sha256=hashlib.sha256((a.backup/'receipt.csv').read_bytes()).hexdigest().upper())
    with a.output.open('x',encoding='utf-8') as stream: json.dump(result,stream,indent=2)
    done.set(); print(json.dumps(result),flush=True)


if __name__=='__main__': main()
