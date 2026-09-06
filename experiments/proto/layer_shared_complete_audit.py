#!/usr/bin/env python3
"""Independent FULL-domain audit, without inheriting an unaudited resume total.

The previously qualified native reader decodes every current record and reads
both physical copies. A historical audit is checked for file preservation, not
used to supply any counters. Only fresh data/logs artifacts are written.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import re
import subprocess
import time

from layer_shared_bitset_audit import (
    CHUNK, LOG_ROOT, MAX_BYTES, N, digest, inventory, predecessor, receipt, write_json,
)
from layer_shared_checkpoint_audit import MASK, PINNED_SHA, fields
from layer_shared_resume_audit import HardBounds, bounded_bytes, require

WORKER_SHA = 'DD541C9318334DCA8763FEBD66E60B81F51CF35D2887456EEBFD9DD804F7D4C4'


def parse_log(raw: bytes) -> tuple[dict, list[dict], dict]:
    lines = raw.decode('utf-8-sig').splitlines()
    resumed = [fields(s) for s in lines if s.startswith('RESUMED ')]
    chunks = [fields(s) for s in lines if s.startswith('CLOSED_CHUNK ')]
    summaries = [fields(s) for s in lines if s.startswith('SUMMARY ')]
    require(len(resumed) == len(summaries) == 1, 'missing or duplicate terminal/resume log')
    return resumed[0], chunks, summaries[0]


def reconcile(reports: list[dict], totals: dict, log: tuple[dict, list[dict], dict]) -> None:
    resume, fresh, summary = log
    prior = int(resume['closed_prefix'])
    require(0 < prior < N and prior % CHUNK == 0, 'invalid final-window resume boundary')
    require(int(resume['chunks']) == prior // CHUNK and
            int(resume['closed_representatives']) == totals['prior_closed_representatives'],
            'independently decoded resume counters differ')
    chunks = [r for r in reports if 'begin' in r]
    old = [r for r in chunks if r['begin'] < prior]
    new = [r for r in chunks if r['begin'] >= prior]
    require(sum(r['live'] for r in old) == totals['prior_live_records'] and
            sum(r['representatives'] for r in old) == totals['prior_closed_representatives'] and
            sum(r['value_checksum'] for r in old) & MASK == totals['prior_value_checksum'],
            'independent prior chunk totals differ')
    require(len(new) == len(fresh), 'new chunk log inventory differs')
    for row, logged in zip(new, fresh):
        require(all(row[k] == int(logged[k]) for k in ('begin', 'count', 'live', 'representatives')) and
                logged['closed_prefix'] == f"{row['begin'] + row['count']}/{N}",
                'new chunk log differs from decoded data')
    expected = dict(new_chunks=len(new), new_indices=N-prior, closed_prefix=N, total_entries=N,
                    closed_representatives=totals['closed_representatives'],
                    F4_checksum_mod2_64=totals['value_checksum'])
    require(all(int(summary[k]) == v for k, v in expected.items()), 'terminal totals differ')
    require(summary.get('status') == 'CLOSED_F4_CATALOGUE' and
            summary.get('domain') == 'complete_native_L4' and summary.get('N6') == 'NOT_COMPUTED',
            'terminal scope is not the complete F4 catalogue')
    require(totals['closed_prefix'] == N and totals['live_records'] == N-18 and totals['holes'] == 18 and
            totals['aliases_to_closed_representatives'] == N-18 and
            totals['aliases_to_future_uncomputed_representatives'] == 0,
            'full-domain live/hole/alias closure failed')


def controller_check(raw: bytes, log_dir: Path, before: Path, after: Path,
                     before_count: int, after_count: int) -> None:
    text = raw.decode('utf-16' if raw[:2] in (b'\xff\xfe', b'\xfe\xff') else 'utf-8-sig')
    ends = re.findall(r'^WINDOW_END exit=([0-9]+) logs=(.+)$', text, re.M)
    require(len(ends) == 1 and ends[0][0] == '0' and Path(ends[0][1].strip()).resolve() == log_dir.resolve(),
            'final controller did not exit zero for this engine log')
    copies = re.findall(r'^PHYSICAL_BACKUP phase=(before|after) files=([0-9]+) directory=(.+)$', text, re.M)
    require(len(copies) == 2, 'controller backup markers missing or duplicated')
    for (phase, count, directory), expected in zip(copies, [('before', before_count, before), ('after', after_count, after)]):
        require(phase == expected[0] and int(count) == expected[1] and
                Path(directory.strip()).resolve() == expected[2].resolve(), 'controller backup binding differs')


def audit(args: argparse.Namespace, guard: HardBounds) -> dict:
    historical, old = predecessor(args.historical_report, args.historical_sha256.upper())
    log_raw = bounded_bytes(args.stdout, 32 << 20)
    log = parse_log(log_raw)
    prior = int(log[0]['closed_prefix'])
    require(0 < int(historical['closed_prefix']) <= prior < N and prior % CHUNK == 0,
            'historical/final-window boundaries are invalid')
    require(bounded_bytes(args.stdout.parent/'stderr.log', 1 << 20) == b'', 'final engine stderr is not empty')
    names = [f'chunk-{i:016x}.bin' for i in range(0, N, CHUNK)] + ['manifest.bin']
    before_names = [f'chunk-{i:016x}.bin' for i in range(0, prior, CHUNK)] + ['manifest.bin']
    require(inventory(args.namespace) == inventory(args.backup_after) == names and
            inventory(args.backup_before) == before_names, 'not the exact complete/before inventory')
    require(args.backup_before.resolve() != args.backup_after.resolve() and
            all(args.namespace.resolve().drive.casefold() != p.resolve().drive.casefold()
                for p in (args.backup_before, args.backup_after)), 'physical backups must be distinct and external')
    after, after_sha = receipt(args.backup_after, names)
    before, before_sha = receipt(args.backup_before, before_names)
    require(all(before[name] == after[name] for name in before_names), 'before/after receipts differ on old files')
    historical_names = [f'chunk-{i:016x}.bin' for i in range(0, int(historical['closed_prefix']), CHUNK)] + ['manifest.bin']
    require(sorted(old) == historical_names and all(after[name] == {k: old[name][k] for k in ('name', 'bytes', 'sha256')}
            for name in historical_names), 'SHA-pinned historical files differ')
    total_bytes = sum(r['bytes'] for r in after.values())
    require(total_bytes == MAX_BYTES, 'full committed byte count differs')
    controller_raw = bounded_bytes(args.controller_log, 32 << 20)
    controller_check(controller_raw, args.stdout.parent, args.backup_before, args.backup_after, len(before_names), len(names))
    require(digest(bounded_bytes(args.worker, 16 << 20)) == WORKER_SHA, 'worker is not the qualified independent reader')
    plan = f'SHARED_F4_BITSET_PLAN_V1\n{N} {prior} {len(names)} {MAX_BYTES} resume\n'
    plan += ''.join(f"{name}\t{after[name]['bytes']}\t{after[name]['sha256']}\t{int(name in before)}\n" for name in names)
    plan_path = args.output.with_suffix('.plan.txt')
    with plan_path.open('x', encoding='ascii', newline='\n') as stream:
        stream.write(plan)
    plan_sha = digest(plan.encode('ascii'))
    seconds = int(args.maxseconds - (time.monotonic()-guard.started)) - 1
    require(seconds > 0, 'no guarded worker time remains')
    command = [str(args.worker.resolve()), 'plan='+str(plan_path.resolve()), 'plan_sha256='+plan_sha,
               'current='+str(args.namespace.resolve()), 'before='+str(args.backup_before.resolve()),
               'after='+str(args.backup_after.resolve()), 'maxseconds='+str(seconds), 'maxrssmib=1024']
    stdout = args.output.with_suffix('.worker.jsonl')
    stderr = args.output.with_suffix('.worker.stderr.txt')
    with stdout.open('xb') as out, stderr.open('xb') as err:
        result = subprocess.run(command, stdout=out, stderr=err, timeout=seconds+0.5, check=False)
    require(result.returncode == 0 and bounded_bytes(stderr, 1 << 20) == b'', 'independent worker failed; no accepted audit')
    rows = [json.loads(s) for s in bounded_bytes(stdout, 32 << 20).decode('ascii').splitlines()]
    require(len(rows) == len(names)+1 and rows[-1].get('worker_status') == 'PASS', 'incomplete native certificate')
    reports, totals = rows[:-1], rows[-1]
    require([r['name'] for r in reports] == names and totals['total_bytes'] == total_bytes, 'worker inventory differs')
    for row in reports:
        name = row['name']
        require(all(row[k] == after[name][k] for k in ('name', 'bytes', 'sha256')), 'worker digest differs')
        if name in old:
            require(all(row[k] == old[name][k] for k in row if k in old[name]), 'historical decoded file differs')
    reconcile(reports, totals, log)
    require(digest(bounded_bytes(args.worker, 16 << 20)) == WORKER_SHA and
            digest(bounded_bytes(plan_path, 16 << 20)) == plan_sha, 'worker or plan changed')
    require(digest(bounded_bytes(args.stdout, 32 << 20)) == digest(log_raw) and
            digest(bounded_bytes(args.controller_log, 32 << 20)) == digest(controller_raw) and
            bounded_bytes(args.stdout.parent/'stderr.log', 1 << 20) == b'', 'terminal logs changed')
    require(inventory(args.namespace) == inventory(args.backup_after) == names and
            inventory(args.backup_before) == before_names and
            receipt(args.backup_after, names)[1] == after_sha and
            receipt(args.backup_before, before_names)[1] == before_sha, 'inventory or receipts changed')
    return dict(status='PASS', audit_type='shared-f4-independent-complete-v1',
        scope='every raw record, physical before/after copy and alias; not independent F4 recomputation',
        storage_integrity_scope='content at the verified reads, not a simultaneous or permanently immutable snapshot',
        full_scan_no_counter_inheritance=True, complete_domain_closed=True, global_alias_closure_checked=True,
        source_full_catalogue_read=False, source_sha256_lineage=PINNED_SHA, N6='NOT_COMPUTED',
        namespace=str(args.namespace.resolve()), backup_before=str(args.backup_before.resolve()),
        backup_after=str(args.backup_after.resolve()), before_receipt_sha256=before_sha, after_receipt_sha256=after_sha,
        historical_report=str(args.historical_report.resolve()), historical_report_sha256=args.historical_sha256.upper(),
        historical_files_unchanged=len(old), final_window_prior_prefix=prior,
        closed_prefix=N, total_entries=N, chunk_count=len(names)-1, file_count=len(names),
        **{k: totals[k] for k in ('total_bytes', 'live_records', 'holes', 'closed_representatives', 'value_checksum',
             'aliases_to_closed_representatives', 'aliases_to_future_uncomputed_representatives',
             'representative_bitset_bytes', 'representative_bitset_sha256')},
        worker_sha256=WORKER_SHA, worker_command=command, plan_sha256=plan_sha,
        stdout_sha256=digest(log_raw), controller_sha256=digest(controller_raw),
        worker_elapsed_seconds=totals['elapsed_seconds'], worker_peak_rss_bytes=totals['peak_rss_bytes'],
        parent_peak_rss_bytes=guard.peak, elapsed_seconds=time.monotonic()-guard.started,
        guard_seconds=args.maxseconds, rss_limit_bytes_each=1 << 30,
        final_window_summary=log[2], files_audited=reports)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('namespace', 'backup-before', 'backup-after', 'historical-report', 'stdout', 'controller-log', 'worker', 'output'):
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--historical-sha256', required=True)
    parser.add_argument('--maxseconds', type=int, default=180)
    args = parser.parse_args()
    require(0 < args.maxseconds <= 180, 'audit time bound must be in (0,180] seconds')
    require(args.output.suffix == '.json' and args.output.parent.is_dir() and
            args.output.resolve().is_relative_to(LOG_ROOT.resolve()), 'fresh JSON output must be under data/logs')
    for suffix in ('.json', '.plan.txt', '.worker.jsonl', '.worker.stderr.txt'):
        require(not args.output.with_suffix(suffix).exists(), 'audit artifact already exists')
    guard = HardBounds(args.maxseconds, 1 << 30)
    try:
        report = audit(args, guard)
        write_json(args.output, report)
        print(json.dumps({k: v for k, v in report.items() if k not in ('files_audited', 'worker_command')}, indent=2))
        print('[OK] complete independent F4 byte/backup/alias audit; checkpoints untouched')
    finally:
        guard.close()


if __name__ == '__main__':
    main()
