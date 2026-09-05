#!/usr/bin/env python3
"""Independent bounded reader for committed shared-F4 chunks and backups.

Reads no checkpoint payload other than the small immutable chunk namespace.
The original source contributes its 128-byte header only. Never reads .tmp
files or writes a checkpoint; an optional fresh JSON report is the sole output.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import itertools
import json
from pathlib import Path
import re
import struct
import time

MASK = (1 << 64)-1
SEED = 0x5344464A434B3031
PINNED_SHA = 'ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844'
SEMANTICS = 0x31304E494D344653
OFFSETS = dict(semantics=80, source_header_hash=88, repair_hash=96,
               entries=104, chunk_size=112, begin=120, count=128,
               payload_hash=136, header_hash=144, representatives=152,
               live=160, value_checksum=168)


def checksum(data: bytes, seed: int = SEED) -> int:
    h = seed ^ ((0x9E3779B97F4A7C15+len(data)) & MASK)
    stop = len(data)//8*8
    for (word,) in struct.iter_unpack('<Q', data[:stop]):
        h = ((h ^ word)*0xFF51AFD7ED558CCD) & MASK
        h ^= h >> 33
    h ^= int.from_bytes(data[stop:], 'little')
    h = h*0xC4CEB9FE1A85EC53 & MASK
    return h ^ (h >> 33)


def header(data: bytes) -> dict:
    if len(data) < 256:
        raise ValueError('truncated shared header')
    result = {name: struct.unpack_from('<Q', data, offset)[0]
              for name, offset in OFFSETS.items()}
    result['magic'] = data[:8].decode('ascii')
    result['version'], result['C'] = struct.unpack_from('<II', data, 8)
    result['source_sha256'] = data[16:80].decode('ascii')
    candidate = bytearray(data[:256])
    struct.pack_into('<Q', candidate, 144, 0)
    if (result['version'] != 1 or result['C'] != 6 or
            result['semantics'] != SEMANTICS or any(data[176:256]) or
            checksum(candidate) != result['header_hash']):
        raise ValueError('shared header version/domain/reserved/checksum mismatch')
    if result['source_sha256'] != PINNED_SHA:
        raise ValueError('unrecognized source SHA lineage')
    return result


def repair_key() -> tuple[list[int], int]:
    """Full 6! 2^6 referee of the documented unseeded prefix key convention."""
    masks = [294,294,554,554,1161,1161,1360,1360,2181,2181,2640,2640]
    fields = [tuple(0 if not ((word >> (2*b))&3) else
                    2 if ((word >> (2*b))&3) == 1 else 3 for b in range(6))
              for word in masks]
    best = None
    key = None
    stabilizer = 0
    for permutation in itertools.permutations(range(6)):
        permuted = [tuple(row[b] for b in permutation) for row in fields]
        for flips in itertools.product(range(2), repeat=6):
            rows = sorted(tuple(value ^ flips[b] if value else 0
                                for b, value in enumerate(row)) for row in permuted)
            signature = tuple(tuple(row[b] for row in rows) for b in range(6))
            if best is None or signature < best:
                best = signature
                key = sorted(sum(value << (2*b) for b, value in enumerate(row)) for row in rows)
                stabilizer = 1
            elif signature == best:
                stabilizer += 1
    if key is None or stabilizer != 12:
        raise ValueError('independent repair-key stabilizer differs from 12')
    return key, stabilizer


def fields(line: str) -> dict[str, str]:
    return dict(re.findall(r'([A-Za-z0-9_]+)=([^\s]+)', line))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--namespace', type=Path, required=True)
    parser.add_argument('--backup', type=Path, required=True)
    parser.add_argument('--source-header', type=Path, required=True)
    parser.add_argument('--stdout', type=Path, required=True)
    parser.add_argument('--expected-prefix', type=int, required=True)
    parser.add_argument('--max-bytes', type=int, default=16 << 20)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if not 0 < args.expected_prefix <= 5_000_000 or not 0 < args.max_bytes <= 64 << 20:
        parser.error('positive prefix<=5m and byte bound<=64MiB required')
    started = time.monotonic()
    names = sorted(path.name for path in args.namespace.iterdir()
                   if path.name == 'manifest.bin' or re.fullmatch(r'chunk-[0-9a-f]{16}\.bin', path.name))
    if not 2 <= len(names) <= 201 or 'manifest.bin' not in names:
        raise ValueError('missing manifest/chunks or more than 200 bounded chunks')
    total_bytes = sum((args.namespace/name).stat().st_size for name in names)
    if total_bytes > args.max_bytes:
        raise ValueError('bounded chunk byte budget exceeded')
    backup_names = sorted(path.name for path in args.backup.iterdir()
                          if path.name == 'manifest.bin' or re.fullmatch(r'chunk-[0-9a-f]{16}\.bin', path.name))
    if names != backup_names:
        raise ValueError('backup committed-file inventory differs')
    with (args.backup/'receipt.csv').open(encoding='utf-8-sig', newline='') as stream:
        receipt = {row['Name']: row for row in csv.DictReader(stream)}
    if sorted(receipt) != names:
        raise ValueError('backup receipt inventory differs')
    raw_manifest = (args.namespace/'manifest.bin').read_bytes()
    manifest = header(raw_manifest)
    if (len(raw_manifest) != 256 or manifest['magic'] != 'SFR4MT01' or
            any(manifest[k] for k in ('begin','count','payload_hash','representatives','live','value_checksum'))):
        raise ValueError('invalid manifest geometry')
    if manifest['entries'] != 903398621 or manifest['chunk_size'] != 25000:
        raise ValueError('unexpected current production catalogue geometry')
    with args.source_header.open('rb') as stream:
        source = bytearray(stream.read(128))
    if len(source) != 128:
        raise ValueError('source header truncated')
    stored_source_hash, = struct.unpack_from('<Q', source, 120)
    struct.pack_into('<Q', source, 120, 0)
    if checksum(source) != stored_source_hash or manifest['source_header_hash'] != stored_source_hash:
        raise ValueError('source header hash differs from chunk lineage')
    key, stab = repair_key()
    expected_repair_hash = checksum(struct.pack('<I', stab),
        checksum(struct.pack('<12H', *key), checksum(b'L4-stab12-order3-witness-v1')))
    if manifest['repair_hash'] != expected_repair_hash:
        raise ValueError('repair provenance differs from independent full-group key')
    lineage_keys = ('C','version','source_sha256','semantics','source_header_hash',
                    'repair_hash','entries','chunk_size')
    prefix = 0
    representatives = value_checksum = live = 0
    aliases: list[int] = []
    values: list[int] = []
    reports = []
    for name in names:
        raw = (args.namespace/name).read_bytes()
        backed = (args.backup/name).read_bytes()
        digest = hashlib.sha256(raw).hexdigest().upper()
        if (raw != backed or hashlib.sha256(backed).hexdigest().upper() != digest or
                receipt[name]['SHA256'] != digest or int(receipt[name]['Bytes']) != len(raw)):
            raise ValueError('physical backup/receipt SHA or bytes mismatch: '+name)
        h = header(raw)
        if any(h[k] != manifest[k] for k in lineage_keys):
            raise ValueError('inconsistent chunk lineage: '+name)
        if name == 'manifest.bin':
            reports.append(dict(name=name, bytes=len(raw), sha256=digest))
            continue
        if (h['magic'] != 'SFR4CK01' or h['begin'] != prefix or
                int(name[6:22],16) != prefix or h['count'] != manifest['chunk_size'] or
                len(raw) != 256+12*h['count']):
            raise ValueError('noncontiguous or malformed chunk geometry')
        alias_bytes = raw[256:256+4*h['count']]
        value_bytes = raw[256+4*h['count']:]
        if checksum(value_bytes,checksum(alias_bytes)) != h['payload_hash']:
            raise ValueError('payload hash mismatch')
        aa = [value[0] for value in struct.iter_unpack('<I',alias_bytes)]
        vv = [value[0] for value in struct.iter_unpack('<Q',value_bytes)]
        local_representatives = local_live = local_checksum = 0
        for offset, (alias,value) in enumerate(zip(aa,vv)):
            identity = prefix+offset
            if alias == 0xFFFFFFFF:
                if value:
                    raise ValueError('hole stores F4')
                continue
            if alias >= manifest['entries']:
                raise ValueError('alias outside complete catalogue domain')
            local_live += 1
            if alias == identity:
                if not value or value % 24:
                    raise ValueError('self representative has invalid positive F4/color-permutation divisibility')
                local_representatives += 1
                local_checksum = (local_checksum+value)&MASK
            elif value:
                raise ValueError('nonrepresentative stores a value')
        if (h['representatives'],h['live'],h['value_checksum']) != (local_representatives,local_live,local_checksum):
            raise ValueError('chunk diagnostic counts differ from independent payload decoding')
        prefix += h['count']
        representatives += local_representatives
        live += local_live
        value_checksum = (value_checksum+local_checksum)&MASK
        aliases.extend(aa); values.extend(vv)
        reports.append(dict(name=name, bytes=len(raw), sha256=digest,
                            begin=h['begin'],count=h['count'],live=h['live'],
                            representatives=h['representatives'],value_checksum=h['value_checksum'],
                            payload_sha256=hashlib.sha256(raw[256:]).hexdigest().upper()))
    if prefix != args.expected_prefix:
        raise ValueError('closed prefix differs from required window')
    aliases_to_closed = aliases_to_future = holes = 0
    for alias in aliases:
        if alias == 0xFFFFFFFF:
            holes += 1
        elif alias < prefix:
            if aliases[alias] != alias or not values[alias]:
                raise ValueError('alias into committed prefix is not a closed fixed point')
            aliases_to_closed += 1
        else:
            aliases_to_future += 1
    log = args.stdout.read_text(encoding='utf-8')
    summaries = [fields(line) for line in log.splitlines() if line.startswith('SUMMARY ')]
    chunks = [fields(line) for line in log.splitlines() if line.startswith('CLOSED_CHUNK ')]
    if len(summaries) != 1 or len(chunks) != len(names)-1:
        raise ValueError('stdout summary/chunk inventory differs')
    summary = summaries[0]
    for actual, name in ((prefix,'closed_prefix'), (manifest['entries'],'total_entries'),
                         (representatives,'closed_representatives'), (value_checksum,'F4_checksum_mod2_64')):
        if int(summary[name]) != actual:
            raise ValueError('stdout summary differs from decoded payload: '+name)
    if summary.get('status') != 'INCOMPLETE_RESUMABLE' or summary.get('N6') != 'NOT_COMPUTED':
        raise ValueError('window scope marker incorrect')
    for h, stated in zip(reports[:-1],chunks):
        for k in ('begin','count','live','representatives'):
            if h[k] != int(stated[k]):
                raise ValueError('per-chunk stdout differs from independent decoded header')
    report = dict(status='PASS', scope='committed chunk integrity and physical-backup audit; not independent F4 recomputation',
                  namespace=str(args.namespace.resolve()), backup=str(args.backup.resolve()),
                  chunks=len(names)-1,files=len(names),total_bytes=total_bytes,closed_prefix=prefix,
                  live_records=live,holes=holes,closed_representatives=representatives,
                  F4_checksum_mod2_64=value_checksum,aliases_to_closed_representatives=aliases_to_closed,
                  aliases_to_future_uncomputed_representatives=aliases_to_future,
                  repair_native_key=key,repair_stabilizer=stab,repair_hash=expected_repair_hash,
                  source_header_hash=stored_source_hash,source_payload_read=False,
                  source_sha256_lineage=PINNED_SHA,source_full_sha256_recomputed=False,
                  engine_summary=summary,files_audited=reports,elapsed_seconds=time.monotonic()-started,
                  limitation='Future alias target membership and F4 mathematics are not independently re-evaluated by this file-integrity audit.')
    with args.output.open('x',encoding='utf-8') as stream:
        json.dump(report,stream,indent=2); stream.write('\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('files_audited','engine_summary')},indent=2))
    print('[OK] bounded independent committed-chunk and backup audit')


if __name__ == '__main__':
    main()
