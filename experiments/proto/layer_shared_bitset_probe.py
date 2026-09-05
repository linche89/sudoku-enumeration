#!/usr/bin/env python3
"""Bounded independent two-pass alias-closure reference; never a producer.

Tests one old immutable prefix (<=2m IDs), comparing its exact resolved/future
counts with a SHA-pinned prior audit. Also tests full-domain bitset closure
against a direct array definition on finite synthetic fixtures. Neither result
claims that a partial production prefix closes the full F4 catalogue.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import time

from layer_shared_checkpoint_audit import checksum, header
from layer_shared_resume_audit import HardBounds, bounded_bytes, require

NONE = 0xFFFFFFFF


def mark(bits: bytearray, identity: int) -> None:
    bits[identity >> 3] |= 1 << (identity&7)


def marked(bits: bytearray, identity: int) -> bool:
    return bool(bits[identity >> 3] & (1 << (identity&7)))


def local_valid(aliases: list[int], values: list[int]) -> bool:
    n = len(aliases)
    return len(values) == n and all(
        (value == 0 if alias == NONE else
         0 <= alias < n and (value > 0 if alias == i else value == 0))
        for i,(alias,value) in enumerate(zip(aliases,values)))


def direct_closed(aliases: list[int], values: list[int]) -> bool:
    return local_valid(aliases,values) and all(
        alias == NONE or (aliases[alias] == alias and values[alias] > 0)
        for alias in aliases)


def bitset_closed(aliases: list[int], values: list[int]) -> bool:
    if not local_valid(aliases,values):
        return False
    bits = bytearray((len(aliases)+7)//8)
    for i,(alias,value) in enumerate(zip(aliases,values)):
        if alias == i and value > 0:
            mark(bits,i)
    return all(alias == NONE or marked(bits,alias) for alias in aliases)


def synthetic_tests() -> dict:
    rng = random.Random(20260905)
    fixtures = []
    for n in range(2,65):
        for repetition in range(4):
            reps = sorted(rng.sample(range(n),rng.randint(1,n)))
            aliases = [rng.choice(reps) for _ in range(n)]
            values = [0]*n
            for rep in reps:
                aliases[rep] = rep
                values[rep] = 24*rng.randint(1,100)
            for i in range(n):
                if i not in reps and rng.randrange(5) == 0:
                    aliases[i] = NONE
            fixtures.append((aliases,values,True,'valid'))
            aa,vv = aliases.copy(),values.copy()
            vv[reps[0]] = 0
            fixtures.append((aa,vv,False,'missing_representative_value'))
            aa,vv = aliases.copy(),values.copy()
            aa[reps[0]] = n
            fixtures.append((aa,vv,False,'outside_domain'))
            aa,vv = aliases.copy(),values.copy()
            aa[0],aa[1],vv[0],vv[1] = 1,0,0,0
            fixtures.append((aa,vv,False,'two_cycle'))
    kinds = {}
    for aliases,values,expected,kind in fixtures:
        require(direct_closed(aliases,values) == expected == bitset_closed(aliases,values),
                'finite bitset/direct closure differential failed: '+kind)
        kinds[kind] = kinds.get(kind,0)+1
    return dict(seed=20260905,fixtures=len(fixtures),classes=kinds,all_exact_equal=True)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--backup',type=Path,required=True)
    parser.add_argument('--prefix',type=int,required=True)
    parser.add_argument('--reference-report',type=Path,required=True)
    parser.add_argument('--reference-sha256',required=True)
    parser.add_argument('--output',type=Path,required=True)
    args = parser.parse_args()
    require(0 < args.prefix <= 2_000_000 and args.prefix%25000 == 0,'bounded old-prefix geometry required')
    require(not args.output.exists() and args.output.suffix == '.json','fresh JSON output required')
    log_root = Path(__file__).resolve().parents[2]/'data'/'logs'
    require(args.output.resolve().is_relative_to(log_root),'probe reports must remain under data/logs')
    guard = HardBounds(120,1 << 30)
    try:
        raw_reference = bounded_bytes(args.reference_report,16 << 20)
        require(hashlib.sha256(raw_reference).hexdigest().upper() == args.reference_sha256.upper(),
                'prior reference SHA mismatch')
        reference = json.loads(raw_reference)
        require(reference['status'] == 'PASS' and reference['closed_prefix'] == args.prefix and
                not reference.get('prefix_snapshot',False) and not reference.get('backup_snapshot_test',False),
                'only a non-test audited prefix may be the probe reference')
        files = {entry['name']:entry for entry in reference['files_audited']}
        bits = bytearray((args.prefix+7)//8)
        representatives = 0
        sha_seen = {}
        for start in range(0,args.prefix,25000):
            name = f'chunk-{start:016x}.bin'
            raw = bounded_bytes(args.backup/name,300256)
            require(hashlib.sha256(raw).hexdigest().upper() == files[name]['sha256'],
                    'immutable backup differs from prior audit')
            sha_seen[name] = files[name]['sha256']
            h = header(raw)
            require(h['begin'] == start and h['count'] == 25000 and len(raw) == 300256,
                    'wrong prefix chunk geometry')
            ab,vb = raw[256:100256],raw[100256:]
            require(checksum(vb,checksum(ab)) == h['payload_hash'],'payload checksum mismatch')
            local_representatives = 0
            for offset,((alias,),(value,)) in enumerate(zip(struct.iter_unpack('<I',ab),struct.iter_unpack('<Q',vb))):
                identity = start+offset
                if alias == NONE:
                    require(value == 0,'hole stores F4')
                    continue
                require(alias < h['entries'],'alias outside complete catalogue')
                if alias == identity:
                    require(value > 0 and value%24 == 0,'self representative lacks closed F4')
                    require(not marked(bits,identity),'representative ID marked twice')
                    mark(bits,identity)
                    representatives += 1
                    local_representatives += 1
                else:
                    require(value == 0,'nonrepresentative stores F4')
            require(local_representatives == h['representatives'],'representative count differs')
        require(sum(byte.bit_count() for byte in bits) == representatives == reference['closed_representatives'],
                'representative bitset population differs from audited count')
        resolved = future = holes = 0
        for start in range(0,args.prefix,25000):
            name = f'chunk-{start:016x}.bin'
            raw = bounded_bytes(args.backup/name,300256)
            require(hashlib.sha256(raw).hexdigest().upper() == sha_seen[name],
                    'source chunk changed between bitset passes')
            for (alias,) in struct.iter_unpack('<I',raw[256:100256]):
                if alias == NONE:
                    holes += 1
                elif alias < args.prefix:
                    require(marked(bits,alias),'alias points to an unmarked/non-self ID inside committed prefix')
                    resolved += 1
                else:
                    future += 1
        require(resolved == reference['aliases_to_closed_representatives'] and
                future == reference['aliases_to_future_uncomputed_representatives'],
                'bitset resolved/future counts differ from independent array audit')
        tests = synthetic_tests()
        report = dict(status='TEST_ONLY_PASS',scope='bounded independent bitset design probe; not a production audit chain node',
            prefix=args.prefix,representative_bitset_bytes=len(bits),closed_representatives=representatives,
            resolved_aliases=resolved,future_aliases=future,holes=holes,
            full_C6_bitset_payload_bytes=(903398621+7)//8,
            synthetic_differential=tests,elapsed_seconds=time.monotonic()-guard.started,
            peak_rss_bytes=guard.peak,source_full_catalogue_read=False,source_checkpoint_writes=False,
            file_sha256_values_checked_twice=sha_seen)
        with args.output.open('x',encoding='utf-8') as stream:
            json.dump(report,stream,indent=2)
            stream.write('\n')
        print(json.dumps({k:v for k,v in report.items() if k != 'file_sha256_values_checked_twice'},indent=2))
        print('[OK] bounded two-pass bitset reference; no full-C6 closure claim')
    finally:
        guard.close()


if __name__ == '__main__':
    main()
