#!/usr/bin/env python3
"""SHA-pinned shared-F4 resume audit using an independent native bitset reader.

Only data/logs artifacts are written. The original catalogue is never opened.
Production mode checks a strictly extended prefix, both physical backups, local
record semantics, and all current-prefix alias targets. Same-prefix differential
qualification is explicit TEST_ONLY_PASS and cannot seed a production chain.
"""
from __future__ import annotations
import argparse
import csv
import hashlib
import io
import json
from pathlib import Path
import re
import struct
import subprocess
import time

from layer_shared_checkpoint_audit import MASK, PINNED_SHA, checksum, fields, header, repair_key
from layer_shared_resume_audit import HardBounds, bounded_bytes, require

AUDIT_TYPE = 'shared-f4-native-bitset-resume-v1'
LEGACY_SCOPES = {
    'committed chunk integrity and physical-backup audit; not independent F4 recomputation',
    'independent immutable-file/resume/backup audit, not independent F4 recalculation',
    'streaming immutable resume/backup integrity audit; not independent F4 recalculation',
}
N, CHUNK = 903398621, 25000
MAX_FILES = (N+CHUNK-1)//CHUNK+1
MAX_BYTES = 12*N+256*MAX_FILES
PATTERN = re.compile(r'chunk-[0-9a-f]{16}\.bin\Z')
SHA = re.compile(r'[A-F0-9]{64}\Z')
LOG_ROOT = Path(__file__).resolve().parents[2]/'data'/'logs'


def digest(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest().upper()


def inventory(directory: Path) -> list[str]:
    require(directory.is_dir(), 'missing audit input directory: '+str(directory))
    names = []
    for path in directory.iterdir():
        if path.name == 'manifest.bin' or PATTERN.fullmatch(path.name):
            names.append(path.name)
            require(len(names) <= MAX_FILES, 'committed inventory exceeds complete domain')
    return sorted(names)  # .tmp is never opened and cannot count as committed.


def receipt(directory: Path, names: list[str]) -> tuple[dict, str]:
    raw = bounded_bytes(directory/'receipt.csv', 16 << 20)
    rows = {}
    for row in csv.DictReader(io.StringIO(raw.decode('utf-8-sig'))):
        name = row.get('Name', '')
        require(name not in rows and (name == 'manifest.bin' or PATTERN.fullmatch(name)),
                'duplicate or malformed receipt name')
        require(SHA.fullmatch(row.get('SHA256', '')) is not None, 'invalid receipt SHA')
        require(re.fullmatch(r'[0-9]{1,12}', row.get('Bytes', '')) is not None, 'invalid receipt byte count')
        rows[name] = dict(name=name, bytes=int(row['Bytes']), sha256=row['SHA256'])
    require(sorted(rows) == names, 'receipt inventory mismatch: '+str(directory))
    return rows, digest(raw)


def predecessor(path: Path, expected_sha: str) -> tuple[dict, dict]:
    raw = bounded_bytes(path, 32 << 20)
    require(digest(raw) == expected_sha, 'previous audit report SHA mismatch')
    previous = json.loads(raw)
    require(previous.get('status') == 'PASS', 'previous report is not a production PASS')
    require(previous.get('prefix_snapshot', False) is False and
            previous.get('backup_snapshot_test', False) is False and
            previous.get('qualification_replay', False) is False and
            previous.get('protocol_test', False) is False,
            'a production audit chain cannot inherit a snapshot/test report')
    if 'audit_type' in previous:
        require(previous['audit_type'] == AUDIT_TYPE and
                previous.get('prefix_snapshot') is False and
                previous.get('backup_snapshot_test') is False and
                previous.get('qualification_replay') is False and
                previous.get('protocol_test') is False and
                previous.get('global_alias_closure_checked') is True,
                'unrecognized or incomplete explicit production audit type')
    else:
        require(previous.get('scope') in LEGACY_SCOPES, 'unrecognized legacy audit scope')
    require(previous.get('source_sha256_lineage', PINNED_SHA) == PINNED_SHA,
            'previous source lineage differs')
    rows = previous['files_audited']
    require(isinstance(rows, list) and 1 < len(rows) <= MAX_FILES, 'invalid previous file list')
    mapped = {}
    for row in rows:
        name = row['name']
        require(name not in mapped and (name == 'manifest.bin' or PATTERN.fullmatch(name)) and
                isinstance(row['bytes'], int) and not isinstance(row['bytes'], bool) and
                0 < row['bytes'] <= 300256 and SHA.fullmatch(row['sha256']),
                'invalid or duplicate previous file entry')
        mapped[name] = row
    return previous, mapped


def write_json(path: Path, value: object) -> None:
    with path.open('x', encoding='utf-8', newline='\n') as stream:
        json.dump(value, stream, indent=2)
        stream.write('\n')


def audit(args: argparse.Namespace, guard: HardBounds) -> dict:
    previous, old = predecessor(args.previous_report, args.previous_sha256)
    prior = int(previous['closed_prefix'])
    prefix = args.expected_prefix
    require(0 < prior <= prefix <= N and (prior % CHUNK == 0 or prior == N), 'invalid predecessor prefix')
    require(prior == prefix if args.qualification_replay else prior < prefix,
            'production requires a strict extension; qualification requires the same prefix')
    names = [f'chunk-{i:016x}.bin' for i in range(0,prefix,CHUNK)]+['manifest.bin']
    old_names = [f'chunk-{i:016x}.bin' for i in range(0,prior,CHUNK)]+['manifest.bin']
    require(sorted(old) == old_names, 'previous report is not an exact contiguous prefix')
    require(inventory(args.namespace) == names and inventory(args.backup_after) == names,
            'current/after inventory is not the exact required prefix')
    require(args.namespace.resolve().drive.casefold() != args.backup_after.resolve().drive.casefold(),
            'current and physical after backup must be on separate Windows volumes')
    after, after_sha = receipt(args.backup_after, names)
    before_sha = None
    if not args.qualification_replay:
        require(args.backup_before is not None, 'production requires a before backup')
        require(args.backup_before.resolve() != args.backup_after.resolve() and
                args.namespace.resolve().drive.casefold() != args.backup_before.resolve().drive.casefold(),
                'before backup must be distinct and on a separate volume')
        require(inventory(args.backup_before) == old_names, 'before inventory differs from predecessor')
        before, before_sha = receipt(args.backup_before, old_names)
        require(all(before[name] == {k:old[name][k] for k in ('name','bytes','sha256')} for name in old_names),
                'before receipt differs from SHA-pinned predecessor')
    for name in old_names:
        require(after[name] == {k:old[name][k] for k in ('name','bytes','sha256')},
                'old after receipt differs from SHA-pinned predecessor')
    total_bytes = sum(row['bytes'] for row in after.values())
    require(total_bytes <= args.max_bytes, 'declared committed-byte budget exceeded')
    raw_manifest = bounded_bytes(args.namespace/'manifest.bin', 256)
    manifest = header(raw_manifest)
    require(manifest['magic'] == 'SFR4MT01' and manifest['entries'] == N and manifest['chunk_size'] == CHUNK and
            manifest['source_header_hash'] == 10599038447932118899,
            'unexpected production manifest/domain/source header')
    key, stab = repair_key()
    repair_hash = checksum(struct.pack('<I',stab),checksum(struct.pack('<12H',*key),
                           checksum(b'L4-stab12-order3-witness-v1')))
    require(manifest['repair_hash'] == repair_hash, 'independent repair hash differs')
    mode = 'replay' if args.qualification_replay else 'resume'
    plan_text = f'SHARED_F4_BITSET_PLAN_V1\n{prefix} {prior} {len(names)} {args.max_bytes} {mode}\n'
    for name in names:
        row = after[name]
        plan_text += f"{name}\t{row['bytes']}\t{row['sha256']}\t{int(not args.qualification_replay and name in old)}\n"
    plan_path = args.output.with_suffix('.plan.txt')
    with plan_path.open('x',encoding='ascii',newline='\n') as stream:
        stream.write(plan_text)
    plan_sha = digest(plan_text.encode('ascii'))
    worker_sha = digest(bounded_bytes(args.worker, 16 << 20))
    seconds = int(args.maxseconds-(time.monotonic()-guard.started))-1
    require(seconds > 0, 'no bounded worker time remains')
    command = [str(args.worker.resolve()), 'plan='+str(plan_path.resolve()), 'plan_sha256='+plan_sha,
               'current='+str(args.namespace.resolve()), 'after='+str(args.backup_after.resolve()),
               'before='+str((args.backup_before or args.backup_after).resolve()),
               'maxseconds='+str(seconds), 'maxrssmib=1024']
    worker_stdout = args.output.with_suffix('.worker.jsonl')
    worker_stderr = args.output.with_suffix('.worker.stderr.txt')
    with worker_stdout.open('xb') as out, worker_stderr.open('xb') as err:
        completed = subprocess.run(command,stdout=out,stderr=err,timeout=seconds+0.5,check=False)
    require(completed.returncode == 0, 'native audit worker failed; see '+str(worker_stderr))
    require(digest(bounded_bytes(args.worker,16<<20)) == worker_sha, 'worker executable changed during audit')
    require(digest(bounded_bytes(plan_path,16<<20)) == plan_sha, 'worker plan changed during audit')
    require(bounded_bytes(worker_stderr,1<<20) == b'', 'native worker stderr is not empty')
    parsed = [json.loads(line) for line in bounded_bytes(worker_stdout,32<<20).decode('ascii').splitlines()]
    require(len(parsed) == len(names)+1 and parsed[-1].get('worker_status') == 'PASS', 'incomplete worker certificate')
    reports, totals = parsed[:-1], parsed[-1]
    require([r['name'] for r in reports] == names, 'worker file inventory differs')
    for row in reports:
        require(all(row[k] == after[row['name']][k] for k in ('name','bytes','sha256')), 'worker file digest differs')
        if row['name'] in old:
            require(all(row[k] == old[row['name']][k] for k in row if k in old[row['name']]),
                    'previous independently decoded file record differs')
    require(totals['closed_prefix'] == prefix and totals['total_bytes'] == total_bytes and
            totals['prior_live_records'] == previous['live_records'] and
            totals['prior_closed_representatives'] == previous['closed_representatives'] and
            totals['prior_value_checksum'] == previous.get('value_checksum',previous.get('F4_checksum_mod2_64')),
            'worker/previous counters disagree')
    new_rows = [r for r in reports if r.get('begin',-1) >= prior]
    summary = previous.get('engine_summary',{})
    stdout_sha = None
    if args.qualification_replay:
        for key_name in ('live_records','closed_representatives','aliases_to_closed_representatives',
                         'aliases_to_future_uncomputed_representatives'):
            require(totals[key_name] == previous[key_name], 'qualification differential counter differs: '+key_name)
        require(totals['holes'] == previous.get('holes',prefix-previous['live_records']), 'qualification holes differ')
    else:
        require(args.stdout is not None, 'production audit needs the terminal stdout log')
        raw = bounded_bytes(args.stdout,32<<20)
        stdout_sha = digest(raw)
        lines = raw.decode('utf-8').splitlines()
        summaries = [fields(line) for line in lines if line.startswith('SUMMARY ')]
        resumed = [fields(line) for line in lines if line.startswith('RESUMED ')]
        fresh = [fields(line) for line in lines if line.startswith('CLOSED_CHUNK ')]
        require(len(summaries) == len(resumed) == 1 and len(fresh) == len(new_rows), 'terminal log inventory differs')
        summary, resume = summaries[0], resumed[0]
        require((int(resume['chunks']),int(resume['closed_prefix']),int(resume['closed_representatives'])) ==
                (len(old_names)-1,prior,previous['closed_representatives']), 'logged resumed state differs')
        for row, logged in zip(new_rows,fresh):
            require(all(row[k] == int(logged[k]) for k in ('begin','count','live','representatives')) and
                    logged['closed_prefix'] == f"{row['begin']+row['count']}/{N}", 'new chunk log differs')
        expected = dict(new_chunks=len(new_rows),new_indices=prefix-prior,closed_prefix=prefix,total_entries=N,
                        closed_representatives=totals['closed_representatives'],F4_checksum_mod2_64=totals['value_checksum'])
        require(all(int(summary[k]) == value for k,value in expected.items()), 'terminal summary differs')
        require(summary.get('N6') == 'NOT_COMPUTED' and
                summary.get('status') == ('CLOSED_F4_CATALOGUE' if prefix == N else 'INCOMPLETE_RESUMABLE'),
                'terminal F4 scope marker differs')
        require(bounded_bytes(args.stdout.parent/'stderr.log',1<<20) == b'', 'producer stderr is not empty')
    require(inventory(args.namespace) == names and inventory(args.backup_after) == names,
            'committed current/after inventory changed during audit')
    require(receipt(args.backup_after,names)[1] == after_sha, 'after receipt changed during audit')
    if not args.qualification_replay:
        require(inventory(args.backup_before) == old_names and receipt(args.backup_before,old_names)[1] == before_sha,
                'before backup inventory/receipt changed during audit')
    return dict(status='TEST_ONLY_PASS' if args.qualification_replay or args.protocol_test else 'PASS',audit_type=AUDIT_TYPE,
        scope='independent native two-pass byte/backup/alias audit; not independent F4 recomputation',
        storage_integrity_scope='content/provenance at the verified reads, not a simultaneous or permanently immutable namespace snapshot',
        qualification_replay=args.qualification_replay,protocol_test=args.protocol_test,
        prefix_snapshot=False,backup_snapshot_test=False,
        namespace=str(args.namespace.resolve()),backup_after=str(args.backup_after.resolve()),
        backup_before=str(args.backup_before.resolve()) if args.backup_before else None,
        previous_report=str(args.previous_report.resolve()),previous_report_sha256=args.previous_sha256,
        previous_prefix=prior,before_receipt_sha256=before_sha,after_receipt_sha256=after_sha,
        closed_prefix=prefix,total_entries=N,chunk_count=len(names)-1,file_count=len(names),total_bytes=total_bytes,
        live_records=totals['live_records'],holes=totals['holes'],closed_representatives=totals['closed_representatives'],
        value_checksum=totals['value_checksum'],new_chunks=len(new_rows),new_indices=prefix-prior,
        new_live_records=totals['live_records']-totals['prior_live_records'],
        new_closed_representatives=totals['closed_representatives']-totals['prior_closed_representatives'],
        new_value_checksum=(totals['value_checksum']-totals['prior_value_checksum'])&MASK,
        aliases_to_closed_representatives=totals['aliases_to_closed_representatives'],
        aliases_to_future_uncomputed_representatives=totals['aliases_to_future_uncomputed_representatives'],
        global_alias_closure_checked=True,complete_domain_closed=prefix==N and totals['aliases_to_future_uncomputed_representatives']==0,
        prior_prefix_files_unchanged=True,before_backup_matches_prior=not args.qualification_replay,
        after_backup_matches_current=True,representative_bitset_bytes=totals['representative_bitset_bytes'],
        representative_bitset_sha256=totals['representative_bitset_sha256'],repair_native_key=key,repair_stabilizer=stab,
        repair_hash=repair_hash,source_header_hash=manifest['source_header_hash'],source_sha256_lineage=PINNED_SHA,
        source_full_catalogue_read=False,global_runtime_extrapolation=False,full_domain_runtime_measured=False,
        worker_sha256=worker_sha,worker_command=command,plan_sha256=plan_sha,stdout_sha256=stdout_sha,
        worker_elapsed_seconds=totals['elapsed_seconds'],worker_peak_rss_bytes=totals['peak_rss_bytes'],
        elapsed_seconds=time.monotonic()-guard.started,parent_peak_rss_bytes=guard.peak,
        engine_summary=summary,files_audited=reports)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('namespace','backup-after','previous-report','worker','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    for name in ('backup-before','stdout'):
        parser.add_argument('--'+name,type=Path)
    parser.add_argument('--previous-sha256',required=True)
    parser.add_argument('--expected-prefix',type=int,required=True)
    parser.add_argument('--max-bytes',type=int,required=True)
    parser.add_argument('--maxseconds',type=int,default=180)
    parser.add_argument('--qualification-replay',action='store_true')
    parser.add_argument('--protocol-test',action='store_true',
                        help='execute all production checks, but emit only TEST_ONLY_PASS')
    args = parser.parse_args()
    args.previous_sha256 = args.previous_sha256.upper()
    require(SHA.fullmatch(args.previous_sha256) and 0 < args.expected_prefix <= N and
            (args.expected_prefix % CHUNK == 0 or args.expected_prefix == N) and
            0 < args.max_bytes <= MAX_BYTES and 0 < args.maxseconds <= 180, 'invalid explicit audit bounds')
    require(args.output.suffix == '.json' and args.output.parent.is_dir() and
            args.output.resolve().is_relative_to(LOG_ROOT.resolve()), 'fresh output must be JSON under data/logs')
    for suffix in ('.json','.plan.txt','.worker.jsonl','.worker.stderr.txt'):
        require(not args.output.with_suffix(suffix).exists(), 'audit artifact must be fresh')
    guard = HardBounds(args.maxseconds,1<<30)  # Parent + worker each capped at1GiB; normally far smaller.
    try:
        report = audit(args,guard)
        write_json(args.output,report)
        print(json.dumps({k:v for k,v in report.items() if k not in ('files_audited','engine_summary','worker_command')},indent=2))
        print('[OK] independent two-pass bitset audit; source checkpoints untouched')
    finally:
        guard.close()


if __name__ == '__main__':
    main()
