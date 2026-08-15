#!/usr/bin/env python3
"""Read-only layer-DP checkpoint inspection and append-only progress sidecar.

The helper reads the version-2 128-byte checkpoint header, validates its
header checksum and exact file length, hashes the image with SHA-256, and can
append one durable CSV record.  It never opens a checkpoint or parent snapshot
for writing.  Delta fan statistics use the immutable parent snapshot's stab
array so abandoned parallel-insertion holes are excluded exactly.
"""

from __future__ import annotations

import argparse
import csv
import datetime as dt
import hashlib
import json
import math
import os
import struct
import sys
import time
from dataclasses import asdict, dataclass
from decimal import Decimal, localcontext
from typing import Sequence


CK_MAGIC = 0x314B434C4A464453
CK_VERSION = 2
CK_SEED = 0x5344464A434B3031
MASK64 = (1 << 64) - 1
HEADER = struct.Struct("<QIIII13Q")
HEADER_BYTES = 128
STATE_BYTES = 24

CSV_FIELDS = [
    "schema_version",
    "event",
    "observed_at",
    "session",
    "git_head",
    "checkpoint_path",
    "generation",
    "c",
    "parent_layer",
    "layer",
    "cursor_chunk",
    "n_chunks",
    "parent_chunks_delta",
    "chunk_parents",
    "parents_processed_delta",
    "claimed_entries",
    "holes",
    "real_states",
    "claimed_entries_delta",
    "real_states_delta",
    "emissions_total",
    "emissions_delta",
    "emissions_per_parent",
    "cache_hits_total",
    "cache_hits_delta",
    "checkpoint_write_seconds",
    "elapsed_seconds",
    "elapsed_delta_seconds",
    "rss_bytes",
    "rss_gib",
    "available_gib",
    "file_bytes",
    "sha256",
    "hash_seconds",
    "config_hash64",
    "parent_keys_hash64",
    "payload_hash64",
    "header_hash64",
    "n_wide",
]


class ProgressError(Exception):
    pass


@dataclass(frozen=True)
class Header:
    magic: int
    version: int
    c: int
    layer: int
    parent_layer: int
    config_hash: int
    generation: int
    n_entries: int
    holes: int
    cursor_chunk: int
    n_chunks: int
    chunk_parents: int
    emissions: int
    cache_hits: int
    parent_keys_hash: int
    n_wide: int
    payload_hash: int
    header_hash: int

    @property
    def real_states(self) -> int:
        return self.n_entries - self.holes

    @property
    def expected_bytes(self) -> int:
        return (
            HEADER_BYTES
            + self.n_entries * (STATE_BYTES + 8 + 4)
            + self.n_wide * 16
        )


def ck_hash64(data: bytes, seed: int) -> int:
    h = seed ^ ((0x9E3779B97F4A7C15 + len(data)) & MASK64)
    offset = 0
    while offset + 8 <= len(data):
        word = int.from_bytes(data[offset : offset + 8], "little")
        h ^= word
        h = (h * 0xFF51AFD7ED558CCD) & MASK64
        h ^= h >> 33
        offset += 8
    tail = int.from_bytes(data[offset:], "little")
    h ^= tail
    h = (h * 0xC4CEB9FE1A85EC53) & MASK64
    h ^= h >> 33
    return h & MASK64


def parse_header(raw: bytes, path: str, file_bytes: int) -> Header:
    if len(raw) != HEADER_BYTES:
        raise ProgressError("%s: truncated checkpoint header" % path)
    values = HEADER.unpack(raw)
    header = Header(
        magic=values[0],
        version=values[1],
        c=values[2],
        layer=values[3],
        parent_layer=values[4],
        config_hash=values[5],
        generation=values[6],
        n_entries=values[7],
        holes=values[8],
        cursor_chunk=values[9],
        n_chunks=values[10],
        chunk_parents=values[11],
        emissions=values[12],
        cache_hits=values[13],
        parent_keys_hash=values[14],
        n_wide=values[15],
        payload_hash=values[16],
        header_hash=values[17],
    )
    if header.magic != CK_MAGIC:
        raise ProgressError("%s: bad checkpoint magic 0x%016X" %
                            (path, header.magic))
    if header.version != CK_VERSION:
        raise ProgressError("%s: checkpoint version %d != %d" %
                            (path, header.version, CK_VERSION))
    if not 2 <= header.c <= 6:
        raise ProgressError("%s: invalid C=%d" % (path, header.c))
    if not 1 <= header.layer <= header.c:
        raise ProgressError("%s: invalid stored layer %d" %
                            (path, header.layer))
    if header.parent_layer:
        if header.layer != header.parent_layer + 1:
            raise ProgressError("%s: invalid transition %d->%d" %
                                (path, header.parent_layer, header.layer))
        if (header.chunk_parents == 0 or header.n_chunks == 0 or
                header.cursor_chunk > header.n_chunks):
            raise ProgressError("%s: invalid chunk geometry" % path)
    elif any((header.cursor_chunk, header.n_chunks, header.chunk_parents,
              header.parent_keys_hash)):
        raise ProgressError("%s: plain snapshot carries transition fields" % path)
    if header.holes > header.n_entries or header.n_wide > header.n_entries:
        raise ProgressError("%s: impossible entries/holes/wide counts" % path)

    zeroed = raw[:120] + b"\0" * 8
    derived_header_hash = ck_hash64(zeroed, CK_SEED)
    if derived_header_hash != header.header_hash:
        raise ProgressError(
            "%s: header hash 0x%016X != derived 0x%016X" %
            (path, header.header_hash, derived_header_hash)
        )
    if file_bytes != header.expected_bytes:
        raise ProgressError("%s: file length %d != header-derived %d" %
                            (path, file_bytes, header.expected_bytes))
    return header


def inspect_checkpoint(path: str, known_sha256: str | None = None) -> tuple[Header, int, str, float]:
    absolute = os.path.abspath(path)
    started = time.monotonic()
    try:
        with open(absolute, "rb") as handle:
            raw = handle.read(HEADER_BYTES)
            handle.seek(0, os.SEEK_END)
            file_bytes = handle.tell()
            header = parse_header(raw, absolute, file_bytes)
            if known_sha256 is None:
                handle.seek(0)
                digest = hashlib.sha256()
                for block in iter(lambda: handle.read(8 << 20), b""):
                    digest.update(block)
                sha256 = digest.hexdigest().upper()
            else:
                sha256 = known_sha256.upper()
    except OSError as exc:
        raise ProgressError("cannot read %s: %s" % (absolute, exc)) from exc
    if len(sha256) != 64 or any(ch not in "0123456789ABCDEF" for ch in sha256):
        raise ProgressError("--known-sha256 is not a 64-digit hexadecimal hash")
    return header, file_bytes, sha256, time.monotonic() - started


def count_real_parents(parent_path: str, expected_child: Header,
                       cursor_before: int, cursor_after: int) -> int:
    absolute = os.path.abspath(parent_path)
    try:
        with open(absolute, "rb") as handle:
            raw = handle.read(HEADER_BYTES)
            handle.seek(0, os.SEEK_END)
            file_bytes = handle.tell()
            parent = parse_header(raw, absolute, file_bytes)
            if parent.c != expected_child.c or parent.layer != expected_child.parent_layer:
                raise ProgressError(
                    "%s: parent C/layer %d/%d does not match child transition %d/%d" %
                    (absolute, parent.c, parent.layer,
                     expected_child.c, expected_child.parent_layer)
                )
            if parent.config_hash != expected_child.config_hash:
                raise ProgressError("%s: parent/child configuration hashes differ" % absolute)
            lo = min(cursor_before * expected_child.chunk_parents, parent.n_entries)
            hi = min(cursor_after * expected_child.chunk_parents, parent.n_entries)
            if hi < lo:
                raise ProgressError("checkpoint cursor moved backwards")
            stab_offset = HEADER_BYTES + parent.n_entries * (STATE_BYTES + 8)
            handle.seek(stab_offset + lo * 4)
            remaining = hi - lo
            real = 0
            while remaining:
                take = min(remaining, 1 << 20)
                block = handle.read(take * 4)
                if len(block) != take * 4:
                    raise ProgressError("%s: truncated parent stab interval" % absolute)
                real += sum(value != 0 for (value,) in struct.iter_unpack("<I", block))
                remaining -= take
            return real
    except OSError as exc:
        raise ProgressError("cannot inspect parent %s: %s" %
                            (absolute, exc)) from exc


def _parse_nonnegative(value: str, field: str) -> int:
    try:
        parsed = int(value)
    except ValueError as exc:
        raise ProgressError("existing progress row has bad %s=%r" %
                            (field, value)) from exc
    if parsed < 0:
        raise ProgressError("existing progress row has negative %s" % field)
    return parsed


def read_progress(path: str) -> list[dict[str, str]]:
    if not os.path.exists(path):
        return []
    try:
        with open(path, "r", newline="", encoding="ascii") as handle:
            reader = csv.DictReader(handle, strict=True)
            if reader.fieldnames != CSV_FIELDS:
                raise ProgressError("%s: unexpected progress.csv header" % path)
            rows = list(reader)
    except (OSError, UnicodeError, csv.Error) as exc:
        raise ProgressError("cannot read progress sidecar %s: %s" %
                            (path, exc)) from exc
    for row in rows:
        if None in row or any(value is None for value in row.values()):
            raise ProgressError("%s: malformed progress row" % path)
    return rows


def decimal_ratio(numerator: int, denominator: int) -> str:
    if denominator == 0:
        return ""
    with localcontext() as context:
        context.prec = 30
        return format(Decimal(numerator) / Decimal(denominator), ".9f")


def append_progress(args: argparse.Namespace, header: Header, file_bytes: int,
                    sha256: str, hash_seconds: float) -> tuple[dict[str, str], bool]:
    sidecar = os.path.abspath(args.progress)
    rows = read_progress(sidecar)
    previous = rows[-1] if rows else None

    identity = (args.event, str(args.session), str(header.generation), sha256)
    if previous is not None:
        previous_identity = (
            previous["event"], previous["session"],
            previous["generation"], previous["sha256"],
        )
        if identity == previous_identity:
            return previous, False

    is_baseline = args.event in ("initial_baseline", "resume_baseline",
                                 "recovered_baseline")
    deltas: dict[str, str] = {
        "parent_chunks_delta": "",
        "parents_processed_delta": "",
        "claimed_entries_delta": "",
        "real_states_delta": "",
        "emissions_delta": "",
        "emissions_per_parent": "",
        "cache_hits_delta": "",
        "elapsed_delta_seconds": "",
    }
    if is_baseline and previous is not None:
        for field, value in (
            ("c", header.c),
            ("parent_layer", header.parent_layer),
            ("layer", header.layer),
            ("n_chunks", header.n_chunks),
            ("chunk_parents", header.chunk_parents),
        ):
            if _parse_nonnegative(previous[field], field) != value:
                raise ProgressError("baseline identity changed at %s" % field)
        if previous["config_hash64"] != "0x%016X" % header.config_hash:
            raise ProgressError("baseline configuration hash changed")
        previous_generation = _parse_nonnegative(
            previous["generation"], "generation"
        )
        previous_cursor = _parse_nonnegative(
            previous["cursor_chunk"], "cursor_chunk"
        )
        if (header.generation < previous_generation or
                header.cursor_chunk < previous_cursor):
            raise ProgressError("baseline checkpoint moved backwards")
        if (header.generation == previous_generation and
                previous["sha256"] != sha256):
            raise ProgressError("same baseline generation has a different SHA-256")
    if not is_baseline:
        if previous is None:
            previous_cursor = 0
            previous_entries = 0
            previous_real = 0
            previous_emissions = 0
            previous_hits = 0
        else:
            for field, value in (
                ("c", header.c),
                ("parent_layer", header.parent_layer),
                ("layer", header.layer),
                ("n_chunks", header.n_chunks),
                ("chunk_parents", header.chunk_parents),
            ):
                if _parse_nonnegative(previous[field], field) != value:
                    raise ProgressError("progress identity changed at %s" % field)
            if previous["config_hash64"] != "0x%016X" % header.config_hash:
                raise ProgressError("progress configuration hash changed")
            previous_generation = _parse_nonnegative(
                previous["generation"], "generation"
            )
            if header.generation != previous_generation + 1:
                raise ProgressError(
                    "generation %d does not follow progress generation %d" %
                    (header.generation, previous_generation)
                )
            previous_cursor = _parse_nonnegative(
                previous["cursor_chunk"], "cursor_chunk"
            )
            previous_entries = _parse_nonnegative(
                previous["claimed_entries"], "claimed_entries"
            )
            previous_real = _parse_nonnegative(
                previous["real_states"], "real_states"
            )
            previous_emissions = _parse_nonnegative(
                previous["emissions_total"], "emissions_total"
            )
            previous_hits = _parse_nonnegative(
                previous["cache_hits_total"], "cache_hits_total"
            )
        if (header.cursor_chunk < previous_cursor or
                header.n_entries < previous_entries or
                header.real_states < previous_real or
                header.emissions < previous_emissions or
                header.cache_hits < previous_hits):
            raise ProgressError("checkpoint cumulative counter moved backwards")
        chunks_delta = header.cursor_chunk - previous_cursor
        parents_delta = count_real_parents(
            args.parent, header, previous_cursor, header.cursor_chunk
        )
        emissions_delta = header.emissions - previous_emissions
        deltas.update({
            "parent_chunks_delta": str(chunks_delta),
            "parents_processed_delta": str(parents_delta),
            "claimed_entries_delta": str(header.n_entries - previous_entries),
            "real_states_delta": str(header.real_states - previous_real),
            "emissions_delta": str(emissions_delta),
            "emissions_per_parent": decimal_ratio(emissions_delta, parents_delta),
            "cache_hits_delta": str(header.cache_hits - previous_hits),
        })
        if (previous is not None and
                previous["session"] == str(args.session) and
                previous["elapsed_seconds"]):
            prior_elapsed = Decimal(previous["elapsed_seconds"])
            elapsed_delta = Decimal(args.elapsed_seconds) - prior_elapsed
            if elapsed_delta < 0:
                raise ProgressError("session elapsed time moved backwards")
            deltas["elapsed_delta_seconds"] = format(elapsed_delta, "f")

    row = {
        "schema_version": "1",
        "event": args.event,
        "observed_at": args.observed_at,
        "session": str(args.session),
        "git_head": args.git_head,
        "checkpoint_path": os.path.abspath(args.checkpoint),
        "generation": str(header.generation),
        "c": str(header.c),
        "parent_layer": str(header.parent_layer),
        "layer": str(header.layer),
        "cursor_chunk": str(header.cursor_chunk),
        "n_chunks": str(header.n_chunks),
        "chunk_parents": str(header.chunk_parents),
        "claimed_entries": str(header.n_entries),
        "holes": str(header.holes),
        "real_states": str(header.real_states),
        "emissions_total": str(header.emissions),
        "cache_hits_total": str(header.cache_hits),
        "checkpoint_write_seconds": args.checkpoint_write_seconds,
        "elapsed_seconds": args.elapsed_seconds,
        "rss_bytes": str(args.rss_bytes),
        "rss_gib": args.rss_gib,
        "available_gib": args.available_gib,
        "file_bytes": str(file_bytes),
        "sha256": sha256,
        "hash_seconds": "%.3f" % hash_seconds,
        "config_hash64": "0x%016X" % header.config_hash,
        "parent_keys_hash64": "0x%016X" % header.parent_keys_hash,
        "payload_hash64": "0x%016X" % header.payload_hash,
        "header_hash64": "0x%016X" % header.header_hash,
        "n_wide": str(header.n_wide),
        **deltas,
    }

    parent_directory = os.path.dirname(sidecar)
    if parent_directory and not os.path.isdir(parent_directory):
        raise ProgressError("progress sidecar directory does not exist: %s" %
                            parent_directory)
    try:
        new_file = not os.path.exists(sidecar)
        mode = "x" if new_file else "a"
        with open(sidecar, mode, newline="", encoding="ascii") as handle:
            writer = csv.DictWriter(handle, fieldnames=CSV_FIELDS,
                                    lineterminator="\n")
            if new_file:
                writer.writeheader()
            writer.writerow(row)
            handle.flush()
            os.fsync(handle.fileno())
    except OSError as exc:
        raise ProgressError("cannot append progress sidecar %s: %s" %
                            (sidecar, exc)) from exc
    return row, True


def iso_timestamp(value: str | None) -> str:
    if value is None:
        return dt.datetime.now(dt.timezone.utc).astimezone().isoformat()
    try:
        parsed = dt.datetime.fromisoformat(value)
    except ValueError as exc:
        raise ProgressError("--observed-at must be ISO-8601") from exc
    if parsed.tzinfo is None:
        raise ProgressError("--observed-at must include a timezone offset")
    return value


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("checkpoint")
    parser.add_argument("--json", action="store_true",
                        help="inspect only; print JSON and do not write a sidecar")
    parser.add_argument("--progress")
    parser.add_argument("--parent")
    parser.add_argument("--event", default="checkpoint",
                        choices=("checkpoint", "initial_baseline",
                                 "resume_baseline", "recovered_baseline"))
    parser.add_argument("--session", type=int, default=0)
    parser.add_argument("--git-head", default="")
    parser.add_argument("--observed-at", default=None)
    parser.add_argument("--elapsed-seconds", default="")
    parser.add_argument("--rss-bytes", type=int, default=0)
    parser.add_argument("--rss-gib", default="")
    parser.add_argument("--available-gib", default="")
    parser.add_argument("--checkpoint-write-seconds", default="")
    parser.add_argument("--known-sha256", default=None)
    parser.add_argument("--expect-generation", type=int)
    parser.add_argument("--expect-cursor", type=int)
    parser.add_argument("--expect-nchunks", type=int)
    args = parser.parse_args(argv)
    if args.json and args.progress:
        parser.error("--json and --progress are mutually exclusive")
    if not args.json:
        if not args.progress:
            parser.error("--progress is required unless --json is used")
        if args.session <= 0:
            parser.error("--session must be positive when appending")
        if not args.git_head:
            parser.error("--git-head is required when appending")
        if args.event == "checkpoint" and not args.parent:
            parser.error("--parent is required for checkpoint delta rows")
    if args.rss_bytes < 0:
        parser.error("--rss-bytes cannot be negative")
    return args


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        args.observed_at = iso_timestamp(args.observed_at)
        header, file_bytes, sha256, hash_seconds = inspect_checkpoint(
            args.checkpoint, args.known_sha256
        )
        for label, expected, actual in (
            ("generation", args.expect_generation, header.generation),
            ("cursor", args.expect_cursor, header.cursor_chunk),
            ("nchunks", args.expect_nchunks, header.n_chunks),
        ):
            if expected is not None and expected != actual:
                raise ProgressError("checkpoint %s %d != marker %d" %
                                    (label, actual, expected))
        inspection = {
            **asdict(header),
            "real_states": header.real_states,
            "file_bytes": file_bytes,
            "sha256": sha256,
            "hash_seconds": round(hash_seconds, 3),
            "checkpoint_path": os.path.abspath(args.checkpoint),
        }
        if args.json:
            print(json.dumps(inspection, sort_keys=True))
        else:
            row, appended = append_progress(
                args, header, file_bytes, sha256, hash_seconds
            )
            print(json.dumps({"appended": appended, "row": row}, sort_keys=True))
        return 0
    except ProgressError as exc:
        print("PROGRESS FAIL: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
