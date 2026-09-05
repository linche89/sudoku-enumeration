#!/usr/bin/env python3
"""Immutable S3 source/stage binding and recoverable final-CSV handoff.

This controller helper does not compute any Sudoku value or edit a native
checkpoint. Legacy artifacts without a binding are deliberately refused.
The result receipt is committed before a nonreplacing CSV rename; recovery
may complete only that exact, already-hashed tuple, never adopt another CSV.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import stat
import subprocess
import sys
import uuid

from layer_dp_progress import CK_SEED, ck_hash64, parse_header

ROOT = Path(__file__).resolve().parents[1]
CODE = ("experiments/proto/layer_dp_gate.cpp", "scripts/build_layer_dp.ps1",
        "scripts/layer_s3_lineage.py", "scripts/layer_dp_progress.py")


def sha(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as handle:
        for block in iter(lambda: handle.read(8 << 20), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def normalized(path):
    return os.path.normcase(str(Path(path).resolve()))


def encoded(value):
    return (json.dumps(value, sort_keys=True, indent=2)+"\n").encode("ascii")


def read_record(path):
    path = Path(path)
    if path.stat().st_size > 65536:
        raise ValueError("oversized S3 lineage record")
    with path.open("rb") as handle:
        raw = handle.read(65537)
    if len(raw) > 65536:
        raise ValueError("oversized S3 lineage record")
    result = json.loads(raw)
    if encoded(result) != raw:
        raise ValueError("noncanonical or damaged S3 lineage record")
    return result


def exclusive_record(path, value):
    path = Path(path)
    if path.exists():
        raise ValueError("refusing to overwrite immutable S3 lineage")
    private = Path(str(path)+".tmp-"+uuid.uuid4().hex)
    with private.open("xb") as handle:
        handle.write(encoded(value)); handle.flush(); os.fsync(handle.fileno())
    # Windows rename is nonreplacing. POSIX link gives the same exclusive
    # commit property; unlink removes only our own freshly linked temporary.
    if os.name == "nt":
        os.rename(private, path)
    else:
        os.link(private, path); private.unlink()


def native_header(path):
    path = Path(path)
    with path.open("rb") as handle:
        return parse_header(handle.read(128), str(path), path.stat().st_size)


def implementation():
    compiler = shutil.which("g++")
    if not compiler:
        raise ValueError("g++ identity is required for S3 lineage")
    return dict(source_sha256={name: sha(ROOT/name) for name in CODE},
                compiler_sha256=sha(compiler),
                compiler_version=subprocess.check_output(
                    [compiler, "-dumpfullversion"], text=True, timeout=10).strip(),
                compiler_target=subprocess.check_output(
                    [compiler, "-dumpmachine"], text=True, timeout=10).strip())


def expected_binding(args):
    for item in (args.source, args.base, args.peer, args.dump):
        reject_reparse(item)
    source = Path(args.source).resolve()
    h = native_header(source)
    expected_config = ck_hash64(struct.pack("<6Q", 0x4C445043414E3031,
                                          args.c, 2*args.c, 24, 0, 0), CK_SEED)
    if (h.c != args.c or h.layer != args.c-1 or h.parent_layer or h.n_wide or
            h.config_hash != expected_config or h.real_states != {5: 17120, 6: 96452755}[args.c]):
        raise ValueError("S3 source must be a complete plain production penultimate layer")
    digest = sha(source)
    if digest != args.source_sha256.upper():
        raise ValueError("supplied source full SHA-256 mismatch")
    caps = tuple(int(item) for item in args.caps.split(","))
    if len(caps) != args.c-1 or any(item <= 0 for item in caps) or args.chunk <= 0:
        raise ValueError("invalid exact stage capacity/chunk configuration")
    base, peer, dump = map(normalized, (args.base, args.peer, args.dump))
    def overlap(a, b):
        # Namespace basenames reserve every suffix and descendant.
        return a == b or a.startswith(b+".") or b.startswith(a+".") or \
            a.startswith(b+os.sep) or b.startswith(a+os.sep)
    if overlap(base, peer) or any(overlap(normalized(source), x) or overlap(dump, x)
                                  for x in (base, peer)):
        raise ValueError("source/dump/stage namespace paths overlap")
    return dict(schema="S3NSB001", source_path=normalized(source), source_sha256=digest,
                source_bytes=source.stat().st_size, source_header_hash=h.header_hash,
                source_config_hash=h.config_hash, c=args.c, parent_layer=args.c-1,
                role=args.role, base=base, peer=peer, dump=dump, caps=list(caps),
                chunk_parents=args.chunk, expected_n=str(args.expected_n),
                native_semantics="LDPCAN01-production-unseeded-unforced-wide",
                implementation=implementation())


def reject_reparse(path):
    current = Path(os.path.abspath(path))
    for part in (current, *current.parents):
        try:
            info = part.lstat()
        except FileNotFoundError:
            continue
        if stat.S_ISLNK(info.st_mode) or getattr(info, "st_file_attributes", 0) & 0x400:
            raise ValueError("reparse/symlink paths are not accepted for S3 lineage")


def paths(args):
    base = str(Path(args.base).resolve())
    return (Path(base+".s3-binding.json"), Path(base+".s3-result.json"),
            Path(base+f".L{args.c-1}.snap"), Path(base+f".L{args.c}.snap"))


def validate(args, create=False):
    binding, result, parent, final = paths(args)
    expected = expected_binding(args)
    artifacts = [parent, final, Path(str(Path(args.base).resolve())+".a"),
                 Path(str(Path(args.base).resolve())+".b"), Path(args.dump), result]
    for path in [binding, *artifacts]:
        reject_reparse(path)
    if not binding.exists():
        base_path = Path(args.base).resolve()
        if any(path.exists() for path in artifacts) or any(base_path.parent.glob(base_path.name+".*")):
            raise ValueError("legacy/unbound S3 artifacts: use a fresh namespace; no inferred migration")
        if create:
            exclusive_record(binding, expected)
        return expected, "BOUND_FRESH" if create else "UNBOUND_FRESH_READONLY"
    if read_record(binding) != expected:
        raise ValueError("S3 source/stage/implementation binding mismatch")
    if any(path.exists() for path in artifacts):
        if not parent.is_file() or sha(parent) != expected["source_sha256"]:
            raise ValueError("resume-parent full SHA differs from pinned source")
    if Path(args.dump).exists() and not result.exists():
        raise ValueError("completed CSV lacks its immutable source-bound result receipt")
    if result.exists():
        validate_result(args, expected)
    return expected, "BOUND_VERIFIED"


def validate_result(args, expected):
    binding, result_path, _, final = paths(args)
    receipt = read_record(result_path)
    if (receipt.get("schema") != "S3RSP001" or receipt.get("binding_sha256") != sha(binding) or
            receipt.get("source_sha256") != expected["source_sha256"] or
            receipt.get("role") != args.role or receipt.get("final_csv") != normalized(args.dump) or
            receipt.get("final_snapshot") != normalized(final)):
        raise ValueError("prepared result has different source/stage lineage")
    if not final.is_file() or sha(final) != receipt["snapshot_sha256"]:
        raise ValueError("prepared result final snapshot SHA mismatch")
    command = Path(receipt["command_record"])
    candidate, target = Path(receipt["prepared_csv"]), Path(args.dump).resolve()
    for path in (command, candidate):
        reject_reparse(path)
    if (candidate == target or target.parent not in candidate.parents or
            target.parent not in command.parents):
        raise ValueError("prepared result paths escaped the dedicated final-CSV log tree")
    if not command.is_file() or sha(command) != receipt["command_sha256"]:
        raise ValueError("prepared result command provenance mismatch")
    if candidate.exists() and target.exists():
        raise ValueError("both prepared and final CSV exist; refusing ambiguous handoff")
    present = target if target.exists() else candidate
    if not present.is_file() or sha(present) != receipt["csv_sha256"]:
        raise ValueError("prepared/final CSV SHA mismatch")
    return receipt


def prepare_result(args, expected):
    binding, result, parent, final = paths(args)
    if not binding.is_file() or not parent.is_file() or sha(parent) != expected["source_sha256"]:
        raise ValueError("result preparation requires the pinned immutable parent")
    if result.exists() or Path(args.dump).exists():
        raise ValueError("result/CSV already exists; use exact prepared-result recovery")
    if not args.prepared_csv or not args.command_record:
        raise ValueError("prepared CSV and command record required")
    prepared, command = Path(args.prepared_csv).resolve(), Path(args.command_record).resolve()
    target = Path(args.dump).resolve()
    # Only private attempt artifacts below this CSV's log directory can be moved.
    if (prepared == target or target.parent not in prepared.parents or
            target.parent not in command.parents):
        raise ValueError("prepared artifacts must stay inside the dedicated final-CSV log tree")
    h = native_header(final)
    if (h.c != args.c or h.layer != args.c or h.config_hash != expected["source_config_hash"] or
            h.real_states != {5: 355, 6: 63199}[args.c] or
            (h.parent_layer and h.cursor_chunk != h.n_chunks) or
            (args.c == 6 and h.n_wide != h.n_entries)):
        raise ValueError("result requires a closed complete native final snapshot")
    lines = command.read_text(encoding="ascii").splitlines()
    if ("layer5_sha256="+expected["source_sha256"]) not in lines or \
            ("binding_sha256="+sha(binding)) not in lines or ("attempt="+args.role) not in lines:
        raise ValueError("command record does not bind this source, stage and role")
    exclusive_record(result, dict(schema="S3RSP001", binding_sha256=sha(binding),
        source_sha256=expected["source_sha256"], role=args.role,
        prepared_csv=normalized(prepared), final_csv=normalized(target),
        csv_sha256=sha(prepared), final_snapshot=normalized(final), snapshot_sha256=sha(final),
        command_record=normalized(command), command_sha256=sha(command)))
    validate_result(args, expected)


def finalize_result(args, expected):
    receipt = validate_result(args, expected)
    target, prepared = Path(args.dump), Path(receipt["prepared_csv"])
    if target.exists():
        return "REUSED_EXACT_CLOSED_RESULT"
    # Only the exact previously committed receipt authorizes this single move.
    if os.name == "nt":
        os.rename(prepared, target)
    else:
        os.link(prepared, target); prepared.unlink()
    validate_result(args, expected)
    return "FINALIZED_EXACT_PREPARED_RESULT"


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("mode", choices=("validate", "bind", "prepare-result", "finalize-result"))
    p.add_argument("--c", type=int, choices=(5, 6), default=6)
    for name in ("source", "source-sha256", "base", "peer", "dump", "caps", "expected-n"):
        p.add_argument("--"+name, required=True)
    p.add_argument("--role", choices=("primary", "replay"), required=True)
    p.add_argument("--chunk", type=int, required=True)
    p.add_argument("--prepared-csv"); p.add_argument("--command-record")
    args = p.parse_args(argv)
    expected, status = validate(args, create=args.mode == "bind")
    if args.mode == "prepare-result":
        prepare_result(args, expected); status = "IMMUTABLE_RESULT_PREPARED"
    elif args.mode == "finalize-result":
        status = finalize_result(args, expected)
    print("S3_LINEAGE "+status)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ValueError, OSError, subprocess.SubprocessError, KeyError) as error:
        print("S3_LINEAGE_REFUSED "+str(error), file=sys.stderr)
        raise SystemExit(2)
