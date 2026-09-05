#!/usr/bin/env python3
"""Read-only bounded-memory geometry checks for closed shared-F4 export.

Chunk payloads/record semantics remain checked by the released native engine.
The controller retains deny-write/delete handles before invoking this reader.
Native serialization is independently checked by layer_support_probe, not by
this header reader. No numerical C6 F4 reevaluation is claimed here.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import struct

from layer_dp_progress import CK_SEED, ck_hash64, parse_header


def native(path):
    with path.open("rb") as handle:
        return parse_header(handle.read(128), str(path), path.stat().st_size)


def shared(path):
    with path.open("rb") as handle:
        raw = handle.read(256)
    if len(raw) != 256:
        raise ValueError("truncated shared header")
    h = struct.unpack("<QII64s22Q", raw)
    if h[1] != 1 or h[4] != 0x31304E494D344653 or any(h[16:]):
        raise ValueError("wrong shared version, semantics or reserved fields")
    zero = raw[:144] + bytes(8) + raw[152:]
    if ck_hash64(zero, CK_SEED) != h[12]:
        raise ValueError("shared header checksum mismatch")
    return h


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--c", type=int, choices=(5, 6), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--source-sha256", required=True)
    parser.add_argument("--namespace", type=Path)
    parser.add_argument("--exported", type=Path)
    parser.add_argument("--chunk", type=int, required=True)
    args = parser.parse_args()
    source = native(args.source)
    expected_live = {5: 17120, 6: 903398603}[args.c]
    expected_mass = {5: 62185328, 6: 41602261536160}[args.c]
    entries = source.n_entries + (args.c == 6)
    if (source.c != args.c or source.layer != 4 or source.n_wide or
            entries - source.holes != expected_live):
        raise ValueError("source has wrong dimension, support count or format")
    if args.c == 6 and (entries != 903398621 or source.holes != 18):
        raise ValueError("C6 source is not the pinned repaired-ID geometry")
    result = dict(c=args.c, layer=4, entries=entries, holes=source.holes,
                  live=expected_live, mass=expected_mass, chunk=args.chunk,
                  config_hash=source.config_hash)
    if args.namespace:
        h = shared(args.namespace / "manifest.bin")
        if (h[0] != 0x3130544D34524653 or h[2] != args.c or
                h[3].decode("ascii").upper() != args.source_sha256.upper() or
                h[5] != source.header_hash or h[7] != entries or
                h[8] != args.chunk or not 1 <= args.chunk <= 1000000 or
                any((h[9], h[10], h[11], h[13], h[14], h[15])) or
                bool(h[6]) != (args.c == 6) or
                (args.namespace / "manifest.bin").stat().st_size != 256):
            raise ValueError("manifest lineage or geometry mismatch")
        expected = set()
        live = 0
        for begin in range(0, entries, args.chunk):
            name = f"chunk-{begin:016x}.bin"
            expected.add(name)
            path = args.namespace / name
            if not path.is_file():
                raise ValueError(f"incomplete namespace: missing {name}")
            ch = shared(path)
            count = min(args.chunk, entries - begin)
            if (ch[0] != 0x31304B4334524653 or ch[1:9] != h[1:9] or
                    ch[9] != begin or ch[10] != count or
                    path.stat().st_size != 256 + 12 * count or
                    not 0 <= ch[13] <= ch[14] <= count):
                raise ValueError(f"chunk lineage, range or length mismatch: {name}")
            live += ch[14]
        found = {p.name for p in args.namespace.glob("chunk-*.bin")}
        if found != expected or live != expected_live:
            raise ValueError("namespace has extra/missing chunks or wrong live total")
        result["chunks"] = len(expected)
        result["complete_header_coverage"] = True
    if args.exported:
        output = native(args.exported)
        if (output.c != args.c or output.layer != 4 or output.parent_layer or
                output.n_wide or output.n_entries != entries or
                output.holes != source.holes or output.config_hash != source.config_hash):
            raise ValueError("export is not a plain production native L4 snapshot")
        result["plain_native_export_header"] = True
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
