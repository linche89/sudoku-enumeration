#!/usr/bin/env python3
"""Bounded streaming audit of one resumed shared-F4 window and its backups.

No producer/controller changes or checkpoint writes. Only committed .bin files
are opened, never temporaries. The prior report is SHA-pinned; every prior file
must remain unchanged. A compact u32 alias array is the only per-state storage.
Use --prefix-snapshot to inspect an OLD immutable prefix while a writer appends:
newer chunks are not opened and the report explicitly states this limited scope.
"""
from __future__ import annotations

import argparse
from array import array
import csv
import ctypes
from ctypes import wintypes
import hashlib
import json
import math
import os
from pathlib import Path
import re
import struct
import sys
import threading
import time

from layer_shared_checkpoint_audit import (MASK, PINNED_SHA, checksum, fields,
                                          header, repair_key)

CHUNK_PATTERN = re.compile(r'chunk-([0-9a-f]{16})\.bin\Z')
NONE = 0xFFFFFFFF


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


class HardBounds:
    def __init__(self, seconds: float, rss: int):
        self.done = threading.Event()
        self.started = time.monotonic()
        self.peak = 0
        if os.name != 'nt':
            raise RuntimeError('this audit RSS guard currently requires Windows')

        class Counters(ctypes.Structure):
            _fields_ = [('cb', wintypes.DWORD), ('PageFaultCount', wintypes.DWORD)] + [
                (name, ctypes.c_size_t) for name in ('PeakWorkingSetSize', 'WorkingSetSize',
                'QuotaPeakPagedPoolUsage', 'QuotaPagedPoolUsage', 'QuotaPeakNonPagedPoolUsage',
                'QuotaNonPagedPoolUsage', 'PagefileUsage', 'PeakPagefileUsage')]

        kernel = ctypes.WinDLL('kernel32', use_last_error=True)
        kernel.GetCurrentProcess.restype = wintypes.HANDLE
        psapi = ctypes.WinDLL('psapi', use_last_error=True)
        psapi.GetProcessMemoryInfo.argtypes = [wintypes.HANDLE, ctypes.POINTER(Counters), wintypes.DWORD]
        psapi.GetProcessMemoryInfo.restype = wintypes.BOOL
        process = kernel.GetCurrentProcess()

        def watch() -> None:
            while not self.done.wait(0.05):
                counter = Counters()
                counter.cb = ctypes.sizeof(counter)
                if not psapi.GetProcessMemoryInfo(process, ctypes.byref(counter), counter.cb):
                    print('BOUND cannot inspect independent audit RSS', file=sys.stderr, flush=True)
                    os._exit(124)
                self.peak = max(self.peak, int(counter.PeakWorkingSetSize))
                if counter.WorkingSetSize > rss or time.monotonic()-self.started > seconds:
                    print('BOUND independent audit time/RSS; no completed report accepted', file=sys.stderr, flush=True)
                    os._exit(124)

        self.thread = threading.Thread(target=watch, daemon=True)
        self.thread.start()

    def close(self) -> None:
        self.done.set()
        self.thread.join()


def bounded_bytes(path: Path, maximum: int) -> bytes:
    size = path.stat().st_size
    require(0 <= size <= maximum, 'file exceeds bounded read: '+str(path))
    with path.open('rb') as stream:
        data = stream.read(maximum+1)
    require(len(data) == size and len(data) <= maximum, 'file changed/exceeded bounded read: '+str(path))
    return data


def inventory(directory: Path, prefix: int | None = None) -> tuple[list[str], int]:
    names = []
    ignored = 0
    for path in directory.iterdir():
        match = CHUNK_PATTERN.fullmatch(path.name)
        if path.name != 'manifest.bin' and match is None:
            continue  # Never open a .tmp or receipt as checkpoint data.
        if match is not None and prefix is not None and int(match[1], 16) >= prefix:
            ignored += 1
            continue
        names.append(path.name)
        require(len(names) <= 10001, 'more than 10000 bounded committed chunks')
    return sorted(names), ignored


def receipt(directory: Path, expected_names: list[str]) -> tuple[dict, str]:
    import io
    raw = bounded_bytes(directory/'receipt.csv', 8 << 20)
    result = {}
    for row in csv.DictReader(io.StringIO(raw.decode('utf-8-sig'))):
        name = row.get('Name', '')
        require(name not in result, 'duplicate backup receipt row')
        require(re.fullmatch(r'[A-F0-9]{64}', row.get('SHA256', '')) is not None,
                'invalid backup receipt SHA')
        result[name] = row
    require(sorted(result) == expected_names, 'backup receipt inventory differs: '+str(directory))
    return result, hashlib.sha256(raw).hexdigest().upper()


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def audit(args: argparse.Namespace, guard: HardBounds) -> dict:
    prior_raw = bounded_bytes(args.previous_report, 16 << 20)
    require(digest(prior_raw) == args.previous_sha256.upper(), 'previous audit report SHA mismatch')
    previous = json.loads(prior_raw)
    require(previous.get('status') == 'PASS', 'previous report did not pass')
    require(previous.get('prefix_snapshot',False) is False and
            previous.get('backup_snapshot_test',False) is False,
            'a production audit chain cannot inherit a snapshot/test report')
    prior_prefix = int(previous['closed_prefix'])
    require(0 < prior_prefix < args.expected_prefix <= args.max_entries,
            'a resumed audit requires a strictly extended, bounded prefix')
    previous_files = {row['name']: row for row in previous['files_audited']}
    require(len(previous_files) == len(previous['files_audited']), 'duplicate previous file records')
    old_names = sorted(previous_files)
    names, ignored = inventory(args.namespace, args.expected_prefix if args.prefix_snapshot else None)
    after_names, _ = inventory(args.backup_after)
    before_names, _ = inventory(args.backup_before)
    require(names == after_names, 'current committed inventory differs from after backup')
    require(old_names == before_names, 'before backup differs from previous audited inventory')
    require(set(old_names) <= set(names), 'previous committed file disappeared')
    if not args.allow_backup_snapshot_test:
        require(args.namespace.resolve().drive.casefold() != args.backup_after.resolve().drive.casefold(),
                'namespace and physical backup must be on separate Windows volumes')
    require(args.backup_before.resolve() != args.backup_after.resolve(), 'before/after backups must be distinct')
    sizes = {name: (args.namespace/name).stat().st_size for name in names}
    total_bytes = sum(sizes.values())
    require(total_bytes <= args.max_bytes, 'total committed-byte budget exceeded')
    before_receipt, before_receipt_sha = receipt(args.backup_before, old_names)
    after_receipt, after_receipt_sha = receipt(args.backup_after, names)
    manifest_raw = bounded_bytes(args.namespace/'manifest.bin', 256)
    manifest = header(manifest_raw)
    require(len(manifest_raw) == 256 and manifest['magic'] == 'SFR4MT01', 'invalid manifest')
    require(not any(manifest[k] for k in ('begin','count','payload_hash','representatives','live','value_checksum')),
            'manifest contains record data')
    require(manifest['entries'] == 903398621 and manifest['chunk_size'] == 25000,
            'unexpected production support/chunk geometry')
    expected_names = [f'chunk-{start:016x}.bin' for start in range(0,args.expected_prefix,25000)] + ['manifest.bin']
    require(args.expected_prefix % 25000 == 0 and names == expected_names, 'committed prefix has a gap or wrong boundary')
    key, stab = repair_key()
    repair_hash = checksum(struct.pack('<I', stab), checksum(struct.pack('<12H', *key),
                           checksum(b'L4-stab12-order3-witness-v1')))
    require(manifest['repair_hash'] == repair_hash, 'independent repair-key/stabilizer hash differs')
    lineage = ('C','version','source_sha256','semantics','source_header_hash','repair_hash','entries','chunk_size')
    aliases = array('I')
    require(aliases.itemsize == 4, 'compact alias array must have 32-bit elements')
    prefix = live = representatives = value_checksum = 0
    prior_reps = prior_live = prior_checksum = 0
    reports = []
    for name in names:
        raw = bounded_bytes(args.namespace/name, 300256)
        require(len(raw) == sizes[name], 'immutable source chunk changed size')
        h = header(raw)
        require(all(h[k] == manifest[k] for k in lineage), 'chunk lineage differs from manifest')
        sha = digest(raw)
        copied = bounded_bytes(args.backup_after/name, 300256)
        require(copied == raw and digest(copied) == sha == after_receipt[name]['SHA256'] and
                len(raw) == int(after_receipt[name]['Bytes']), 'after backup/receipt differs: '+name)
        if name in previous_files:
            copied = bounded_bytes(args.backup_before/name, 300256)
            require(copied == raw and digest(copied) == sha == before_receipt[name]['SHA256'] and
                    len(raw) == int(before_receipt[name]['Bytes']) and
                    sha == previous_files[name]['sha256'] and len(raw) == previous_files[name]['bytes'],
                    'previous immutable file/before backup changed: '+name)
        report = dict(name=name, bytes=len(raw), sha256=sha)
        if name == 'manifest.bin':
            reports.append(report)
            continue
        require(h['magic'] == 'SFR4CK01' and h['begin'] == prefix and h['count'] == 25000 and len(raw) == 300256,
                'chunk range/length mismatch')
        alias_bytes, value_bytes = raw[256:100256], raw[100256:]
        require(checksum(value_bytes,checksum(alias_bytes)) == h['payload_hash'], 'chunk payload hash mismatch')
        local_aliases = array('I')
        local_aliases.frombytes(alias_bytes)
        if sys.byteorder != 'little':
            local_aliases.byteswap()
        local_reps = local_live = local_checksum = 0
        for offset, (alias, (value,)) in enumerate(zip(local_aliases, struct.iter_unpack('<Q', value_bytes))):
            if alias == NONE:
                require(value == 0, 'hole stores F4')
                continue
            require(alias < manifest['entries'], 'alias outside complete native domain')
            local_live += 1
            if alias == prefix+offset:
                require(value > 0 and value % 24 == 0, 'self representative has invalid F4')
                local_reps += 1
                local_checksum = (local_checksum+value)&MASK
            else:
                require(value == 0, 'nonrepresentative stores F4')
        require((local_reps,local_live,local_checksum) == (h['representatives'],h['live'],h['value_checksum']),
                'decoded chunk counters differ from header')
        report.update(begin=h['begin'],count=h['count'],live=local_live,representatives=local_reps,
                      value_checksum=local_checksum,payload_sha256=digest(raw[256:]))
        reports.append(report)
        aliases.extend(local_aliases)
        prefix += 25000
        live += local_live
        representatives += local_reps
        value_checksum = (value_checksum+local_checksum)&MASK
        if prefix == prior_prefix:
            prior_reps, prior_live, prior_checksum = representatives, live, value_checksum
    require(prefix == args.expected_prefix, 'decoded prefix differs from required prefix')
    old_checksum = previous.get('value_checksum',previous.get('F4_checksum_mod2_64'))
    require(prior_reps == previous['closed_representatives'] and prior_checksum == old_checksum,
            'previous report totals differ from its unchanged chunks')
    resolved = future = holes = 0
    for alias in aliases:
        if alias == NONE:
            holes += 1
        elif alias < prefix:
            require(aliases[alias] == alias, 'alias into committed prefix is not a closed fixed point')
            resolved += 1  # Its positive F4 was already checked while streaming.
        else:
            future += 1
    terminal = bounded_bytes(args.stdout, 16 << 20).decode('utf-8')
    summaries = [fields(line) for line in terminal.splitlines() if line.startswith('SUMMARY ')]
    resumes = [fields(line) for line in terminal.splitlines() if line.startswith('RESUMED ')]
    fresh = [fields(line) for line in terminal.splitlines() if line.startswith('CLOSED_CHUNK ')]
    require(len(summaries) == len(resumes) == 1, 'stdout summary/resume count differs')
    summary, resume = summaries[0], resumes[0]
    prior_chunks = len(old_names)-1
    require((int(resume['chunks']),int(resume['closed_prefix']),int(resume['closed_representatives'])) ==
            (prior_chunks,prior_prefix,prior_reps), 'logged resumed prefix differs from prior audit')
    new_rows = [row for row in reports if row.get('begin',-1) >= prior_prefix]
    require(len(fresh) == len(new_rows), 'logged new chunk inventory differs')
    for reported, logged in zip(new_rows,fresh):
        require(all(reported[k] == int(logged[k]) for k in ('begin','count','live','representatives')),
                'logged new chunk counters differ')
        require(logged['closed_prefix'] == f"{reported['begin']+reported['count']}/{manifest['entries']}",
                'logged chunk prefix differs')
    expected_summary = dict(new_chunks=len(new_rows),new_indices=prefix-prior_prefix,closed_prefix=prefix,
                            total_entries=manifest['entries'],closed_representatives=representatives,
                            F4_checksum_mod2_64=value_checksum)
    require(all(int(summary[k]) == value for k,value in expected_summary.items()), 'terminal summary differs')
    require(summary.get('N6') == 'NOT_COMPUTED' and summary.get('status') == 'INCOMPLETE_RESUMABLE',
            'incorrect bounded F4 scope marker')
    require(bounded_bytes(args.stdout.parent/'stderr.log', 1 << 20) == b'', 'completed window stderr is not empty')
    final_names, final_ignored = inventory(args.namespace,args.expected_prefix if args.prefix_snapshot else None)
    require(final_names == names, 'committed inventory changed during audit; retry on a closed window')
    ignored = max(ignored,final_ignored)
    return dict(status='PASS',scope='streaming immutable resume/backup integrity audit; not independent F4 recalculation',
        namespace=str(args.namespace.resolve()),backup_before=str(args.backup_before.resolve()),
        backup_after=str(args.backup_after.resolve()),previous_report=str(args.previous_report.resolve()),
        previous_report_sha256=args.previous_sha256.upper(),previous_prefix=prior_prefix,
        before_receipt_sha256=before_receipt_sha,after_receipt_sha256=after_receipt_sha,
        closed_prefix=prefix,chunk_count=len(names)-1,file_count=len(names),total_bytes=total_bytes,
        live_records=live,holes=holes,closed_representatives=representatives,value_checksum=value_checksum,
        new_chunks=len(new_rows),new_indices=prefix-prior_prefix,new_live_records=live-prior_live,
        new_closed_representatives=representatives-prior_reps,new_value_checksum=(value_checksum-prior_checksum)&MASK,
        aliases_to_closed_representatives=resolved,aliases_to_future_uncomputed_representatives=future,
        prior_prefix_files_unchanged=True,before_backup_matches_prior=True,after_backup_matches_current=True,
        compact_alias_bytes=len(aliases)*aliases.itemsize,repair_native_key=key,repair_stabilizer=stab,
        repair_hash=repair_hash,source_header_hash=manifest['source_header_hash'],source_sha256_lineage=PINNED_SHA,
        source_full_catalogue_read=False,global_runtime_extrapolation=False,prefix_snapshot=args.prefix_snapshot,
        newer_chunk_paths_not_opened=ignored,backup_snapshot_test=args.allow_backup_snapshot_test,
        elapsed_seconds=time.monotonic()-guard.started,peak_rss_bytes=guard.peak,engine_summary=summary,
        files_audited=reports)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('namespace','backup-before','backup-after','previous-report','stdout','output'):
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--previous-sha256', required=True)
    parser.add_argument('--expected-prefix', type=int, required=True)
    parser.add_argument('--max-entries', type=int, default=20_000_000)
    parser.add_argument('--max-bytes', type=int, default=512 << 20)
    parser.add_argument('--maxseconds', type=float, default=180)
    parser.add_argument('--maxrssgib', type=int, default=4)
    parser.add_argument('--prefix-snapshot', action='store_true')
    parser.add_argument('--allow-backup-snapshot-test', action='store_true',
                        help='TEST ONLY: allow the namespace to be an immutable external-backup snapshot')
    args = parser.parse_args()
    require(re.fullmatch(r'[A-Fa-f0-9]{64}',args.previous_sha256) is not None, 'valid prior SHA required')
    require(0 < args.max_entries <= 100_000_000 and 0 < args.max_bytes <= 2 << 30 and
            0 < args.expected_prefix <= args.max_entries and 1 <= args.maxrssgib <= 4 and
            math.isfinite(args.maxseconds) and 0 < args.maxseconds <= 180, 'invalid explicit audit bounds')
    require(not args.output.exists(), 'audit report output must be fresh')
    require(args.output.suffix == '.json' and args.output.parent.is_dir(), 'fresh JSON report needs an existing parent')
    log_root = Path(__file__).resolve().parents[2]/'data'/'logs'
    require(args.output.resolve().is_relative_to(log_root), 'independent reports must stay under data/logs')
    guard = HardBounds(args.maxseconds,args.maxrssgib << 30)
    try:
        report = audit(args,guard)
        with args.output.open('x',encoding='utf-8') as stream:
            json.dump(report,stream,indent=2)
            stream.write('\n')
        print(json.dumps({key:value for key,value in report.items() if key not in ('files_audited','engine_summary')},indent=2))
        print('[OK] bounded streaming resume audit; source checkpoints untouched')
    finally:
        guard.close()


if __name__ == '__main__':
    main()
