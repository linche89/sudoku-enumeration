#!/usr/bin/env python3
"""Bounded independent qualification of the native shared-F4 audit, not counting.

All constructed checkpoints stay in a fresh data/logs directory and carry only
synthetic arithmetic. Reports are TEST_ONLY_PASS, never production ancestors.
"""
from __future__ import annotations
import argparse
import copy
import hashlib
import json
from pathlib import Path
import random
import shutil
import struct
import subprocess
import sys
import time
import uuid

from layer_shared_checkpoint_audit import MASK, PINNED_SHA, SEMANTICS, checksum
from layer_shared_resume_audit import HardBounds, require
from layer_shared_bitset_audit import AUDIT_TYPE, CHUNK, LOG_ROOT, N, predecessor, write_json
from layer_shared_bitset_probe import synthetic_tests

NONE = 0xFFFFFFFF


def sha(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest().upper()


def make_header(magic: bytes, begin: int = 0, aliases: list[int] | None = None,
                values: list[int] | None = None) -> bytes:
    out = bytearray(256)
    out[:8] = magic
    struct.pack_into('<II',out,8,1,6)
    out[16:80] = PINNED_SHA.encode('ascii')
    for offset,value in ((80,SEMANTICS),(88,10599038447932118899),(96,8375264605690466664),
                         (104,N),(112,CHUNK)):
        struct.pack_into('<Q',out,offset,value)
    if aliases is not None:
        ab = struct.pack('<'+'I'*len(aliases),*aliases)
        vb = struct.pack('<'+'Q'*len(values),*values)
        reps = [i for i,a in enumerate(aliases) if a == begin+i]
        for offset,value in ((120,begin),(128,len(aliases)),(136,checksum(vb,checksum(ab))),
                             (152,len(reps)),(160,sum(a != NONE for a in aliases)),
                             (168,sum(values[i] for i in reps)&MASK)):
            struct.pack_into('<Q',out,offset,value)
    struct.pack_into('<Q',out,144,checksum(out))
    return bytes(out)


def chunk(begin: int, aliases: list[int], values: list[int]) -> bytes:
    return make_header(b'SFR4CK01',begin,aliases,values)+struct.pack('<'+'I'*len(aliases),*aliases)+struct.pack('<'+'Q'*len(values),*values)


def reseal_header(raw: bytes, offset: int, value: int, width: int = 8) -> bytes:
    out = bytearray(raw)
    struct.pack_into('<I' if width == 4 else '<Q',out,offset,value)
    struct.pack_into('<Q',out,144,0)
    struct.pack_into('<Q',out,144,checksum(out[:256]))
    return bytes(out)


def direct(aliases: list[int], values: list[int]) -> dict | None:
    prefix = len(aliases)
    live = reps = total = resolved = future = holes = 0
    for i,(a,v) in enumerate(zip(aliases,values)):
        if a == NONE:
            if v != 0: return None
            holes += 1
        else:
            if not 0 <= a < N: return None
            live += 1
            if a == i:
                if v <= 0 or v % 24: return None
                reps += 1
                total = (total+v)&MASK
            elif v: return None
    for a in aliases:
        if a == NONE: continue
        if a < prefix:
            if aliases[a] != a or values[a] <= 0: return None
            resolved += 1
        else: future += 1
    return dict(live_records=live,holes=holes,closed_representatives=reps,value_checksum=total,
                aliases_to_closed_representatives=resolved,aliases_to_future_uncomputed_representatives=future)


def run_worker(worker: Path, root: Path, label: str, aliases: list[int], values: list[int],
               expected: bool = True, mutate=None, change_plan=None, change_copy=None,
               arguments=None) -> dict:
    case = root/label
    case.mkdir()
    dirs = {name:case/name for name in ('current','after','before')}
    for path in dirs.values(): path.mkdir()
    blobs = {f'chunk-{i:016x}.bin':chunk(i,aliases[i:i+CHUNK],values[i:i+CHUNK])
             for i in range(0,len(aliases),CHUNK)}
    blobs['manifest.bin'] = make_header(b'SFR4MT01')
    if mutate: blobs = mutate(blobs)
    for name,raw in blobs.items():
        for directory in dirs.values():
            (directory/name).write_bytes(raw)  # Generated disposable finite fixture, never a source edit.
    if change_copy: change_copy(dirs)
    prior = CHUNK
    plan = f'SHARED_F4_BITSET_PLAN_V1\n{len(aliases)} {prior} {len(blobs)} {len(aliases)*12+256*len(blobs)} resume\n'
    for name,raw in sorted(blobs.items()):
        old = name == 'manifest.bin' or int(name[6:22],16) < prior
        plan += f'{name}\t{len(raw)}\t{sha(raw)}\t{int(old)}\n'
    if change_plan: plan = change_plan(plan)
    plan_path = case/'plan.txt'
    plan_path.write_text(plan,encoding='ascii',newline='\n')
    command = [str(worker.resolve()),'plan='+str(plan_path.resolve()),'plan_sha256='+sha(plan.encode('ascii')),
               'current='+str(dirs['current'].resolve()),'after='+str(dirs['after'].resolve()),
               'before='+str(dirs['before'].resolve()),'maxseconds=5','maxrssmib=128']
    if arguments: command = arguments(command)
    completed = subprocess.run(command,capture_output=True,text=True,timeout=6)
    (case/'stdout.jsonl').write_text(completed.stdout,encoding='utf-8')
    (case/'stderr.txt').write_text(completed.stderr,encoding='utf-8')
    require((completed.returncode == 0) == expected, f'wrong native result {label}: {completed.stderr}')
    report = dict(name=label,expected_accept=expected,exit_code=completed.returncode,
                  command=command,stderr=completed.stderr.strip())
    if expected:
        rows = [json.loads(line) for line in completed.stdout.splitlines()]
        result = rows[-1]
        reference = direct(aliases,values)
        require(reference is not None and result.get('worker_status') == 'PASS', 'accepted native certificate is missing')
        require(all(result[k] == v for k,v in reference.items()), 'native/direct exact counters differ')
        report['counters'] = reference
        report['worker_seconds'] = result['elapsed_seconds']
    else:
        require('"worker_status":"PASS"' not in completed.stdout, 'failure emitted an accepted certificate')
    return report


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--worker',type=Path,required=True)
    parser.add_argument('--output-dir',type=Path)
    parser.add_argument('--historical-window2',action='store_true')
    args = parser.parse_args()
    root = args.output_dir or LOG_ROOT/('shared-bitset-audit-gate-'+uuid.uuid4().hex)
    require(not root.exists() and root.resolve().is_relative_to(LOG_ROOT.resolve()), 'fresh test directory under data/logs required')
    root.mkdir()
    guard = HardBounds(120,1<<30)
    try:
        rng = random.Random(20260905)
        n = 2*CHUNK
        reps = sorted({0,1,20,CHUNK-1,CHUNK,40000,n-1}|set(rng.sample(range(n),1024)))
        rep_set = set(reps)
        aliases = [rng.choice(reps) for _ in range(n)]
        values = [0]*n
        for r in reps: aliases[r],values[r] = r,24*rng.randrange(1,10000)
        for i in range(n):
            if i not in rep_set and rng.randrange(10) == 0: aliases[i] = NONE
            elif i not in rep_set and rng.randrange(5) == 0: aliases[i] = rng.choice([n,N-1,800000000])
        report = dict(status='TEST_ONLY_PASS',scope='synthetic native byte/closure and protocol qualification; no F4 recalculation',
                      seed=20260905,theorem_differential=synthetic_tests(),native_cases=[])
        cases = report['native_cases']
        cases.append(run_worker(args.worker,root,'valid_mixed_future_and_holes',aliases,values))
        aa,vv = aliases.copy(),values.copy()
        vv[0] = vv[1] = (MASK//24)*24
        cases.append(run_worker(args.worker,root,'valid_u64_checksum_wrap',aa,vv))
        # Independent fixed point predicate vs native bitmap across many random components.
        for trial in range(8):
            aa = [NONE]*n; vv = [0]*n
            for start in range(0,n,127):
                stop = min(n,start+127)
                selected = rng.sample(range(start,stop),rng.randrange(1,min(12,stop-start)+1))
                for i in range(start,stop): aa[i] = rng.choice(selected)
                for i in selected: aa[i],vv[i] = i,24*rng.randrange(1,10000)
            cases.append(run_worker(args.worker,root,f'valid_random_components_{trial}',aa,vv))
        nonrep = next(i for i in range(n) if i not in rep_set)
        mutations = {
            'self_zero':lambda a,v:v.__setitem__(0,0),
            'self_not_factorial':lambda a,v:v.__setitem__(0,25),
            'nonrep_value':lambda a,v:v.__setitem__(nonrep,24),
            'outside_domain':lambda a,v:(a.__setitem__(nonrep,N),v.__setitem__(nonrep,0)),
            'hole_has_value':lambda a,v:(a.__setitem__(nonrep,NONE),v.__setitem__(nonrep,24)),
            'two_cycle':lambda a,v:(a.__setitem__(0,1),a.__setitem__(1,0),v.__setitem__(0,0),v.__setitem__(1,0)),
            'alias_to_hole':lambda a,v:(a.__setitem__(nonrep,0),a.__setitem__(0,NONE),v.__setitem__(0,0)),
            'alias_to_nonself':lambda a,v:(a.__setitem__(nonrep,0),a.__setitem__(0,20),v.__setitem__(0,0)),
        }
        for name,mutation in mutations.items():
            aa,vv = aliases.copy(),values.copy();mutation(aa,vv)
            require(direct(aa,vv) is None,'direct negative fixture unexpectedly valid')
            cases.append(run_worker(args.worker,root,name,aa,vv,False))
        first = 'chunk-0000000000000000.bin'
        for name,offset,value,width in [
            ('wrong_C',12,5,4),('wrong_version',8,2,4),('wrong_semantics',80,0,8),
            ('wrong_source_header',88,1,8),('wrong_repair',96,1,8),('wrong_domain',104,N-1,8),
            ('wrong_chunk_size',112,1,8),('wrong_range',120,1,8),('wrong_count',128,CHUNK-1,8),
            ('wrong_counter',152,0,8),('reserved_nonzero',176,1,8)]:
            def mutate(blobs,offset=offset,value=value,width=width):
                blobs[first] = reseal_header(blobs[first],offset,value,width);return blobs
            cases.append(run_worker(args.worker,root,name,aliases,values,False,mutate=mutate))
        def bad_payload(blobs):
            raw = bytearray(blobs[first]);raw[260] ^= 1;blobs[first] = bytes(raw);return blobs
        cases.append(run_worker(args.worker,root,'bad_payload_checksum',aliases,values,False,mutate=bad_payload))
        def bad_header(blobs):
            raw = bytearray(blobs[first]);raw[144] ^= 1;blobs[first] = bytes(raw);return blobs
        cases.append(run_worker(args.worker,root,'bad_header_checksum',aliases,values,False,mutate=bad_header))
        def bad_source(blobs):
            raw = bytearray(blobs[first]);raw[16] = ord('0');blobs[first] = reseal_header(bytes(raw),144,0);return blobs
        cases.append(run_worker(args.worker,root,'wrong_source_sha',aliases,values,False,mutate=bad_source))
        def bad_manifest(blobs):
            blobs['manifest.bin'] = reseal_header(blobs['manifest.bin'],120,1);return blobs
        cases.append(run_worker(args.worker,root,'manifest_has_records',aliases,values,False,mutate=bad_manifest))
        for label,change in [('trailing_plan',lambda p:p+'TRAILING\n'),('blank_trailing_plan',lambda p:p+'\n'),
                             ('bad_plan_magic',lambda p:p.replace('PLAN_V1','PLAN_V2',1)),
                             ('wrong_prior_copy_flag',lambda p:p.replace('\t1\n','\t0\n',1)),
                             ('path_traversal_name',lambda p:p.replace(first,'../'+first,1)),
                             ('short_plan',lambda p:'\n'.join(p.splitlines()[:-1])+'\n'),
                             ('wrong_plan_bound',lambda p:p.replace(f'{n} {CHUNK} 3 {n*12+768}',f'{n} {CHUNK} 3 1',1))]:
            cases.append(run_worker(args.worker,root,label,aliases,values,False,change_plan=change))
        for kind in ('current','after','before'):
            def corrupt(dirs,kind=kind):
                path = dirs[kind]/first;raw = bytearray(path.read_bytes());raw[-1] ^= 1;path.write_bytes(raw)
            cases.append(run_worker(args.worker,root,'corrupt_'+kind,aliases,values,False,change_copy=corrupt))
        def trailing(dirs):
            path = dirs['current']/first;path.write_bytes(path.read_bytes()+b'!')
        cases.append(run_worker(args.worker,root,'trailing_input_bytes',aliases,values,False,change_copy=trailing))
        for label,old,new in [('bad_plan_sha','plan_sha256=','plan_sha256='+'0'*64),
                             ('zero_seconds','maxseconds=','maxseconds=0'),
                             ('excessive_seconds','maxseconds=','maxseconds=181'),
                             ('bad_rss','maxrssmib=','maxrssmib=127')]:
            cases.append(run_worker(args.worker,root,label,aliases,values,False,
                arguments=lambda cmd,old=old,new=new:[new if x.startswith(old) else x for x in cmd]))
        # A generic PASS or explicit TEST flag must not be laundered into a new ancestor.
        old_path = LOG_ROOT/'c6-direct-route-20260905/shared-window4-independent-audit.json'
        old_raw = old_path.read_bytes();old = json.loads(old_raw)
        predecessor(old_path,sha(old_raw))
        typed = copy.deepcopy(old)
        typed.update(audit_type=AUDIT_TYPE,qualification_replay=False,protocol_test=False,
                     prefix_snapshot=False,backup_snapshot_test=False,global_alias_closure_checked=True)
        typed_path = root/'prior-recognized-new-type.json';write_json(typed_path,typed)
        predecessor(typed_path,sha(typed_path.read_bytes()))
        admissions = []
        for key,value in [('status','TEST_ONLY_PASS'),('prefix_snapshot',True),('backup_snapshot_test',True),
                          ('qualification_replay',True),('protocol_test',True),('audit_type','unknown'),
                          ('scope','arbitrary PASS')]:
            poisoned = copy.deepcopy(old);poisoned[key] = value
            path = root/('prior-'+key+'.json');write_json(path,poisoned)
            accepted = False
            try: predecessor(path,sha(path.read_bytes()));accepted = True
            except ValueError as error: admissions.append(dict(field=key,rejected=True,message=str(error)))
            require(not accepted,'poisoned predecessor accepted: '+key)
        report['predecessor_rejections'] = admissions
        for key in ('qualification_replay','protocol_test','global_alias_closure_checked'):
            missing = copy.deepcopy(typed);del missing[key]
            path = root/('prior-new-type-missing-'+key+'.json');write_json(path,missing)
            try:
                predecessor(path,sha(path.read_bytes()))
                raise AssertionError('new audit type accepted without mandatory flag: '+key)
            except ValueError as error:
                admissions.append(dict(field='new_type_missing_'+key,rejected=True,message=str(error)))
        report['recognized_production_predecessor_types_accepted'] = ['legacy_window4',AUDIT_TYPE]
        if args.historical_window2:
            stem = '20260905-145352-cd47a5df238a497582627f60acf94586'
            backup_root = Path('D:/sudoku_FJ_checkpoint_backups/c6_shared_f4_20260905')
            source = backup_root/(stem+'-after')
            local = root/'historical-window2-local';local.mkdir()
            for path in source.iterdir():
                if path.name == 'manifest.bin' or path.name.startswith('chunk-') and path.suffix == '.bin':
                    require(path.stat().st_size <= 300256,'historical bounded file too large')
                    shutil.copyfile(path,local/path.name)
            require(sum(p.stat().st_size for p in local.iterdir()) == 6305632,'historical copy byte count differs')
            output = root/'historical-window2-protocol.json'
            command = [sys.executable,str(Path(__file__).with_name('layer_shared_bitset_audit.py')),
                '--namespace',str(local),'--backup-before',str(backup_root/(stem+'-before')),
                '--backup-after',str(source),'--previous-report',str(LOG_ROOT/'c6-direct-route-20260905/shared-window1-independent-audit.json'),
                '--previous-sha256','B83BAF974C5574FFF62D7632FC27D8CA9E72659949221B1E0552F7708B746AFC',
                '--worker',str(args.worker),'--stdout',str(LOG_ROOT/('shared-f4-window-'+stem)/'stdout.log'),
                '--expected-prefix','525000','--max-bytes','8388608','--maxseconds','20','--protocol-test','--output',str(output)]
            completed = subprocess.run(command,capture_output=True,text=True,timeout=25)
            (root/'historical-protocol.stdout.txt').write_text(completed.stdout,encoding='utf-8')
            (root/'historical-protocol.stderr.txt').write_text(completed.stderr,encoding='utf-8')
            require(completed.returncode == 0,'historical protocol rejected: '+completed.stderr)
            observed = json.loads(output.read_text(encoding='utf-8'))
            reference = json.loads((LOG_ROOT/'c6-direct-route-20260905/shared-window2-independent-audit.json').read_text())
            keys = ('closed_prefix','total_bytes','live_records','closed_representatives','value_checksum',
                    'aliases_to_closed_representatives','aliases_to_future_uncomputed_representatives')
            require(observed['status'] == 'TEST_ONLY_PASS' and all(observed[k] == reference[k] for k in keys),
                    'historical protocol differential differs')
            report['historical_protocol'] = dict(command=command,exact_equal=True,output=str(output),sha256=sha(output.read_bytes()))
        report.update(native_accepts=sum(r['expected_accept'] for r in cases),
                      native_rejections=sum(not r['expected_accept'] for r in cases),
                      elapsed_seconds=time.monotonic()-guard.started,peak_rss_bytes=guard.peak,
                      worker_sha256=sha(args.worker.read_bytes()))
        write_json(root/'summary.json',report)
        print(json.dumps({k:v for k,v in report.items() if k != 'native_cases'},indent=2))
        print('[OK] native bitset audit finite qualification; TEST_ONLY_PASS')
    finally: guard.close()


if __name__ == '__main__': main()
