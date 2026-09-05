#!/usr/bin/env python3
"""Complete C=2..5 tests for direct graph values and rooted reverse layers.

Creates only disposable small-C fixtures under a fresh output directory.  No
C=6 checkpoint is read or written.  Each native coefficient is checked, not
merely the final square sum.  The existing layer engine supplies an independent
row-incremental reference; the new executable computes graph factorizations.
"""

from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from math import factorial
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from layer_dp_progress import inspect_checkpoint  # noqa: E402

KNOWN = {
    2: 288,
    3: 28200960,
    4: 29136487207403520,
    5: 1903816047972624930994913280000,
}
COUNTS = {
    2: (1, 2),
    3: (1, 5, 4),
    4: (1, 23, 54, 26),
    5: (1, 107, 16150, 17120, 355),
}
REFERENCE = ROOT / "docs/expert/2026-07-21/native_c5_response_quotient_triples.csv"
REFERENCE_SHA256 = "D2FDEB354ED4C1443E9870B5727CE35C88BA6B392C6DAA32CD92D2601BCDB1F5"


def run(command: list[str], log: Path, seconds: float) -> str:
    started = time.monotonic()
    with log.open("x", encoding="utf-8") as output:
        result = subprocess.run(
            command, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT,
            timeout=seconds, check=False,
        )
    text = log.read_text(encoding="utf-8")
    if result.returncode:
        raise RuntimeError(f"exit {result.returncode}: {command!r}; see {log}")
    print(f"RUN {log.name} seconds={time.monotonic()-started:.3f}", flush=True)
    return text


def expect_failure(command: list[str], log: Path, seconds: float) -> str:
    with log.open("x", encoding="utf-8") as output:
        result = subprocess.run(command, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT,
                                timeout=seconds, check=False)
    text = log.read_text(encoding="utf-8")
    if result.returncode == 0 or "[OK]" in text:
        raise ValueError(f"expected fail-closed rejection: {command!r}; see {log}")
    print(f"EXPECTED REJECTION {log.name}", flush=True)
    return text


def decode_state(words: tuple[int, ...], c: int, layer: int) -> tuple[int, ...]:
    if any(words[2*c:]):
        raise ValueError("nonzero state padding")
    masks = []
    for word in words[:2*c]:
        if word >> (2*c):
            raise ValueError("bits outside boxes")
        mask = 0
        for box in range(c):
            field = (word >> (2*box)) & 3
            if field == 1:
                raise ValueError("invalid native field 1")
            if field:
                mask |= 1 << (2*box + field - 2)
        if mask.bit_count() != layer:
            raise ValueError("wrong row degree")
        masks.append(mask)
    if any(sum((mask >> slot) & 1 for mask in masks) != layer
           for slot in range(2*c)):
        raise ValueError("unbalanced slot degree")
    return tuple(masks)


def export_closed(path: Path, target: Path, c: int, layer: int) -> dict:
    header, _, digest, _ = inspect_checkpoint(str(path))
    if (header.c, header.layer) != (c, layer):
        raise ValueError("fixture identity mismatch")
    if header.parent_layer and header.cursor_chunk != header.n_chunks:
        raise ValueError("partial accumulator is not a reference")
    if header.n_wide:
        raise ValueError("unexpected wide small-C fixture")
    raw = path.read_bytes()
    n = header.n_entries
    group = (1 << c) * factorial(c)
    weight_offset = 128 + 24*n
    stab_offset = weight_offset + 8*n
    real = holes = mass = total = 0
    seen = set()
    with target.open("x", encoding="ascii", newline="\n") as output:
        output.write(f"# C={c} L={layer}; complete exact native reference\n")
        for i in range(n):
            stab, = struct.unpack_from("<I", raw, stab_offset + 4*i)
            if not stab:
                holes += 1
                continue
            if group % stab:
                raise ValueError("invalid stabilizer divisor")
            native = struct.unpack_from("<12H", raw, 128 + 24*i)
            if native in seen:
                raise ValueError("duplicate fixture key")
            seen.add(native)
            masks = decode_state(native, c, layer)
            weight, = struct.unpack_from("<Q", raw, weight_offset + 8*i)
            orbit = group // stab
            value, remainder = divmod(weight, orbit)
            if remainder:
                raise ValueError("T/orbit is not an integer")
            output.write(" ".join(map(str, masks)) + f" expected={value}\n")
            real += 1
            mass += orbit
            if layer == c:
                a = 1
                for multiplicity in Counter(masks).values():
                    a *= factorial(multiplicity)
                ell = factorial(2*c) // a
                total += orbit * ell * value * value
    if holes != header.holes or real != COUNTS[c][layer-1]:
        raise ValueError("wrong complete layer size")
    if layer == c and total != KNOWN[c]:
        raise ValueError("incorrect independent square sum")
    return dict(c=c, layer=layer, states=real, orbit_mass=mass,
                n=total if layer == c else None, snapshot_sha256=digest)


def checked_summary(text: str, expected_rows: int, *, closed: bool = False) -> dict[str, str]:
    if "[OK]" not in text:
        raise ValueError("benchmark did not report successful exact checks")
    summaries = [line for line in text.splitlines() if line.startswith("summary ")]
    if len(summaries) != 1:
        raise ValueError("missing or ambiguous benchmark summary")
    fields = dict(re.findall(r"([a-z_]+)=([0-9.]+)", summaries[0]))
    if (int(fields.get("rows", -1)) != expected_rows or
            int(fields.get("expected_checked", -1)) != expected_rows):
        raise ValueError("not every native coefficient was checked")
    if closed and int(fields.get("closed_misses", -1)) != 0:
        raise ValueError("complete predecessor cache has misses")
    return fields


def native_gather_gate(args: argparse.Namespace, stem: Path, c: int, layer: int,
                       states: int, checks: list[dict]) -> None:
    seconds = min(120.0, args.seconds_per_process)
    totals = Counter()
    for start in range(0, states, 1000):
        count = min(1000, states - start)
        log = stem.with_suffix(f".L{layer}.native.{start:06d}.log")
        command = [str(args.native_exe.resolve()), str(c), str(layer),
                   f"input={stem.with_suffix(f'.L{layer}.txt')}",
                   f"preload={stem.with_suffix(f'.L{layer-1}.txt')}",
                   f"start={start}", f"limit={count}",
                   "maxrecords=5000000", "maxstates=2000000",
                   f"maxseconds={seconds}"]
        fields = checked_summary(run(command, log, seconds + 5), count, closed=True)
        if int(fields.get("closed_hits", -1)) != int(fields.get("weak_residuals", -2)):
            raise ValueError("native residual lookup inventory is not fully closed")
        for key in ("rows", "expected_checked", "matches", "weak_residuals", "closed_misses"):
            totals[key] += int(fields[key])
        checks.append(dict(kind="native-gather", c=c, layer=layer,
                           start=start, rows=count, summary=fields, log=log.name))
    if totals["rows"] != states or totals["expected_checked"] != states:
        raise ValueError("native chunks do not cover the complete source layer")
    expected_work = {(5, 4): (3375557, 2935081), (5, 5): (425652, 270870)}
    if (c, layer) in expected_work:
        if (totals["matches"], totals["weak_residuals"]) != expected_work[c, layer]:
            raise ValueError("native rooted work inventory changed")
    print("NATIVE COMPLETE " + json.dumps(dict(c=c, layer=layer, **totals), sort_keys=True),
          flush=True)


def fiber_gate(args: argparse.Namespace, stem: Path, c: int, layer: int,
               states: int, checks: list[dict]) -> None:
    if args.fiber_exe is None:
        return
    seconds = min(180.0, args.seconds_per_process)
    command = [str(args.fiber_exe.resolve()), str(c), str(layer),
               f"input={stem.with_suffix(f'.L{layer}.txt')}", f"limit={states}",
               "maxpairings=5000000", f"maxseconds={seconds}",
               "census", f"invariance={min(16, states)}"]
    if (c, layer) == (4, 3):
        command.append("witnesscheck")
    log = stem.with_suffix(f".L{layer}.fiber.log")
    text = run(command, log, seconds + 5)
    objects = [json.loads(line) for line in text.splitlines() if line.startswith("{")]
    census = [obj for obj in objects if obj.get("census") == "passed"]
    summaries = [obj for obj in objects if obj.get("summary") is True]
    if "[OK]" not in text or len(census) != 1 or len(summaries) != 1:
        raise ValueError("pairing fiber census did not close")
    summary = summaries[0]
    if (summary.get("rows") != states or summary.get("native_rows") != states or
            summary.get("invariance_checked") != min(16, states)):
        raise ValueError("pairing fiber source/invariance coverage mismatch")
    for field in ("native_states", "fiber_sum_no_transpose", "fiber_sum_with_transpose"):
        if census[0].get(field) != states:
            raise ValueError("pairing fiber census lost native classes")
    expected_classes = {(4, 3): (38, 33), (5, 3): (1160, 721), (5, 4): (14237, 12543)}
    if (c, layer) in expected_classes:
        got = (census[0].get("graph_classes_no_transpose"),
               census[0].get("graph_classes_with_transpose"))
        if got != expected_classes[c, layer]:
            raise ValueError("complete pairing fiber graph-class inventory changed")
    if (c, layer) == (4, 3) and not any(
            obj.get("external_witness_check") == "passed" for obj in objects):
        raise ValueError("paired-versus-graph witness check was skipped")
    checks.append(dict(kind="pairing-fiber", c=c, layer=layer,
                       census=census[0], summary=summary, log=log.name))


def support_reader_gate(args: argparse.Namespace, stem: Path, info: dict,
                        checks: list[dict]) -> None:
    if args.support_exe is None:
        return
    snapshot = stem.with_suffix(".L3.snap")
    sample = stem.with_suffix(".L3.reader-sample.txt")
    common = [str(args.support_exe.resolve()), "--input", str(snapshot),
              "--checkpointreadonly", "--max-seconds", "30", "--max-rss-mib", "256"]
    text = run(common + ["--expected-sha256", info["snapshot_sha256"],
                         "--samples", "16", "--sample-output", str(sample),
                         "--closed-small-expected-values"],
               stem.with_suffix(".L3.reader.log"), 35)
    if ("payload_hash_verified=yes header_hash_verified=yes readonly=yes" not in text or
            f"sha256={info['snapshot_sha256']}" not in text or
            "sample_count=16 " not in text or "[OK]" not in text):
        raise ValueError("closed small-C support reader audit failed")
    identity = [line for line in text.splitlines() if line.startswith("C=")]
    if len(identity) != 1:
        raise ValueError("missing support reader inventory")
    fields = dict(re.findall(r"([a-zA-Z_]+)=([0-9]+)", identity[0]))
    if (int(fields.get("C", -1)), int(fields.get("layer", -1)), int(fields.get("real", -1))) != (4, 3, info["states"]):
        raise ValueError("support reader lost a small-C state")
    if f"stored_stabilizer_orbit_mass={info['orbit_mass']}" not in text:
        raise ValueError("support reader mass differs from independent export")
    result = run([str(args.bench_exe.resolve()), "4", "3", f"input={sample}",
                  "limit=16", "mode=direct", "maxseconds=30", "maxrecords=1000000",
                  "maxstates=100000"], stem.with_suffix(".L3.reader-values.log"), 35)
    checked_summary(result, 16)
    rejected = expect_failure(common + ["--expected-sha256", "0" * 64],
                              stem.with_suffix(".L3.reader-bad-sha.log"), 35)
    if "SHA-256 mismatch" not in rejected:
        raise ValueError("wrong reader failure: expected a full SHA-256 rejection")
    if hashlib.sha256(snapshot.read_bytes()).hexdigest().upper() != info["snapshot_sha256"]:
        raise ValueError("read-only support checks modified their disposable source")
    checks.append(dict(kind="small-support-reader", c=4, layer=3, samples=16,
                       states=info["states"], wrong_hash_rejected=True))


def catalog_lookup_gate(args: argparse.Namespace, stem: Path, checks: list[dict]) -> None:
    if args.catalog_exe is None:
        return
    catalog = stem.with_suffix(".L4.snap")
    before = hashlib.sha256(catalog.read_bytes()).hexdigest()
    seconds = min(120.0, args.seconds_per_process)
    command = [str(args.catalog_exe.resolve()), "5", f"catalog={catalog}",
               f"input={stem.with_suffix('.L5.txt')}", "mode=gather", "limit=355",
               "maxqueries=5000000", f"threads={args.parallel_threads}",
               f"maxseconds={seconds}", "checkpointreadonly"]
    log = stem.with_suffix(".catalog-lookup.log")
    text = run(command, log, seconds + 5)
    summaries = [line for line in text.splitlines() if line.startswith("summary ")]
    if len(summaries) != 1 or "[OK] checkpointreadonly=yes production_T_used_as_F=no" not in text:
        raise ValueError("catalogue key-lookup diagnostic did not close")
    fields = dict(re.findall(r"([a-z_]+)=([0-9.]+)", summaries[0]))
    expected = dict(sources=355, matches=425652, weak=270870,
                    queries=270870, hits=270870, misses=0)
    if any(int(fields.get(key, -1)) != value for key, value in expected.items()):
        raise ValueError("catalogue key-lookup coverage or zero-miss check failed")
    if hashlib.sha256(catalog.read_bytes()).hexdigest() != before:
        raise ValueError("catalogue key-lookup changed its disposable source")
    # This checks the native index, not F values; the native gather gate above
    # independently checks every exact F5 against the reference coefficients.
    checks.append(dict(kind="small-catalog-index", c=5, layer=5,
                       summary=fields, log=log.name, values_checked=False))


def order3_canonical_gate(args: argparse.Namespace, out: Path, checks: list[dict]) -> None:
    if args.canonical_exe is None:
        return
    raw = out / "order3-fixed.txt"
    selected = out / "order3-stab12.txt"
    run([sys.executable, str(ROOT / "experiments/proto/layer_order3_witnesses.py"),
         "--time-limit", "180", "--max-states", "50000", "--max-visits", "5000000",
         "--output", str(raw)], out / "order3-generate.log", 185)
    text = run([str(args.canonical_exe.resolve()), "6", "4", str(raw), str(selected),
                "41900", "12", "120"], out / "order3-canonical.log", 125)
    summaries = [line for line in text.splitlines() if line.startswith("[OK] rows=")]
    if len(summaries) != 1:
        raise ValueError("missing complete order-three canonicalization summary")
    fields = dict(re.findall(r"([a-z_]+)=([0-9.]+)", summaries[0]))
    expected = dict(rows=41900, distinct_orbits=1060, selected=66, checkpoint_io=0)
    if any(int(fields.get(key, -1)) != value for key, value in expected.items()):
        raise ValueError("complete order-three canonicalization coverage mismatch")
    records = [line for line in selected.read_text(encoding="ascii").splitlines()
               if line.strip() and not line.startswith("#")]
    if len(records) != 66 or len(set(records)) != 66:
        raise ValueError("canonical witness file is not exactly 66 distinct keys")
    checks.append(dict(kind="order3-canonical-support", c=6, layer=4,
                       summary=fields, production_checkpoint_io=False))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--max-c", type=int, choices=range(2, 6), default=5)
    parser.add_argument("--layer-exe", type=Path, default=ROOT / "build/layer_dp_gate.exe")
    parser.add_argument("--bench-exe", type=Path, default=ROOT / "build/layer_direct_bench.exe")
    parser.add_argument("--native-exe", type=Path, default=ROOT / "build/layer_native_gather_bench.exe")
    parser.add_argument("--fiber-exe", type=Path)
    parser.add_argument("--support-exe", type=Path)
    parser.add_argument("--canonical-exe", type=Path)
    parser.add_argument("--catalog-exe", type=Path)
    parser.add_argument("--parallel-threads", type=int, choices=range(2, 25), default=4)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--seconds-per-process", type=float, default=300)
    parser.add_argument("--fixtures-only", action="store_true")
    args = parser.parse_args()
    if not 0 < args.seconds_per_process <= 600:
        parser.error("process bound must be in (0,600]")
    if args.output is None:
        base = ROOT / "data/logs"
        base.mkdir(exist_ok=True)
        out = Path(tempfile.mkdtemp(prefix="layer-direct-gate-", dir=base))
    else:
        out = args.output.resolve()
        out.mkdir(parents=True, exist_ok=False)
    print(f"ARTIFACTS {out}", flush=True)
    if hashlib.sha256(REFERENCE.read_bytes()).hexdigest().upper() != REFERENCE_SHA256:
        raise ValueError("independent C=5 reference changed")
    summaries = []
    checks = []
    for c in range(2, args.max_c + 1):
        stem = out / f"c{c}"
        caps = ",".join(str(max(100, size*2)) for size in COUNTS[c][1:])
        command = [str(args.layer_exe.resolve()), str(c), "--threads", "4",
                   "--caps", caps, "--invariance", "50", "--scan-check", "50"]
        if c == 5:
            command += ["--ref", str(REFERENCE)]
        for layer in range(2, c+1):
            command += ["--save-layer", str(layer), str(stem.with_suffix(f".L{layer}.snap"))]
        text = run(command, stem.with_suffix(".reference.log"), args.seconds_per_process)
        if str(KNOWN[c]) not in text:
            raise ValueError("reference engine did not report expected N")
        initial = stem.with_suffix(".L1.txt")
        with initial.open("x", encoding="ascii") as handle:
            handle.write(f"# C={c} L=1\n")
            handle.write(" ".join(str(1 << s) for s in range(2*c)) + " expected=1\n")
        for layer in range(2, c+1):
            source = stem.with_suffix(f".L{layer}.txt")
            info = export_closed(stem.with_suffix(f".L{layer}.snap"), source, c, layer)
            summaries.append(info)
            print("FIXTURE " + json.dumps(info, sort_keys=True), flush=True)
            if args.fixtures_only:
                continue
            common = [str(args.bench_exe.resolve()), str(c), str(layer),
                      f"input={source}", f"limit={info['states']}",
                      f"maxseconds={args.seconds_per_process}",
                      "maxrecords=100000000", "maxstates=1000000"]
            for mode in (["direct", "gather"] if layer <= 4 else ["gather"]):
                cmd = common + [f"mode={mode}"]
                if mode == "gather":
                    cmd += [f"preload={stem.with_suffix(f'.L{layer-1}.txt')}"]
                result = run(cmd, stem.with_suffix(f".L{layer}.{mode}.log"),
                             args.seconds_per_process + 5)
                fields = checked_summary(result, info["states"], closed=(mode == "gather"))
                checks.append(dict(kind=mode, c=c, layer=layer, summary=fields))
            if c <= 4 or layer >= 4:
                native_gather_gate(args, stem, c, layer, info["states"], checks)
            if layer == 4:
                command = common + ["mode=direct", f"threads={args.parallel_threads}"]
                log = stem.with_suffix(f".L{layer}.parallel.log")
                fields = checked_summary(run(command, log, args.seconds_per_process + 5),
                                         info["states"], closed=True)
                if int(fields.get("threads", -1)) != args.parallel_threads:
                    raise ValueError("parallel exact check used the wrong thread count")
                checks.append(dict(kind="parallel-direct", c=c, layer=layer,
                                   summary=fields, log=log.name))
            if (c, layer) in ((4, 3), (5, 3), (5, 4)):
                fiber_gate(args, stem, c, layer, info["states"], checks)
            if (c, layer) == (4, 3):
                support_reader_gate(args, stem, info, checks)
        if not args.fixtures_only and c == 5:
            catalog_lookup_gate(args, stem, checks)
    if not args.fixtures_only:
        order3_canonical_gate(args, out, checks)
    with (out / "summary.json").open("x", encoding="utf-8") as handle:
        json.dump(summaries, handle, indent=2)
    with (out / "checks.json").open("x", encoding="utf-8") as handle:
        json.dump(checks, handle, indent=2)
    print("FIXTURES PASSED" if args.fixtures_only else "COMPLETE DIRECT/ROOTED LAYER GATE PASSED",
          flush=True)


if __name__ == "__main__":
    main()
