#!/usr/bin/env python3
"""Disposable exactness, export, and crash-recovery gates for shared F4.

No production checkpoint is discovered or opened. Fresh C4/C5 references come
from the independent row-incremental engine. Deliberately corrupt files are
new copies inside this runner's new artifact directory; nothing is removed.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from math import factorial
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import time

from layer_direct_gate import (COUNTS, KNOWN, REFERENCE, REFERENCE_SHA256,
                               ROOT, checked_summary, decode_state, export_closed)
from layer_dp_progress import CK_SEED, ck_hash64, inspect_checkpoint

MASK64 = (1 << 64) - 1
CHUNK_BYTES = 256
# SFR4CK01 version 1 with the explicit supportRepairHash lineage field.
CHUNK_OFF = dict(total=104, chunk=112, begin=120, count=128, payload=136,
                 header=144, representatives=152, live=160, checksum=168)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def exclusive(path: Path, data: bytes) -> None:
    with path.open("xb") as handle:
        handle.write(data)


def fields(line: str) -> dict[str, str]:
    return dict(re.findall(r"([A-Za-z0-9_]+)=([^\s]+)", line))


def summary(text: str, expected: int | None = None) -> dict[str, str]:
    rows = [line for line in text.splitlines() if line.startswith("SUMMARY ")]
    if len(rows) != 1:
        raise ValueError("missing or ambiguous shared summary")
    result = fields(rows[0])
    if result.get("N6") != "NOT_COMPUTED":
        raise ValueError("shared engine scope marker missing")
    if expected is not None and (result.get("status") != "CLOSED_F4_CATALOGUE" or
            int(result.get("closed_prefix", -1)) != expected or
            int(result.get("total_entries", -1)) != expected):
        raise ValueError("shared catalogue is not fully closed")
    return result


def native_image(path: Path) -> tuple[object, bytes, dict]:
    """Independent header/SHA/payload, legality and weighted-value reader."""
    h, _, _, _ = inspect_checkpoint(str(path))
    raw = path.read_bytes()
    if h.parent_layer or h.n_wide:
        raise ValueError("gate expects a narrow closed native snapshot")
    n = h.n_entries
    keys, weights, stabs = raw[128:128+24*n], raw[128+24*n:128+32*n], raw[128+32*n:]
    calculated = ck_hash64(stabs, ck_hash64(weights, ck_hash64(keys, CK_SEED)))
    if calculated != h.payload_hash:
        raise ValueError("independent native payload checksum mismatch")
    entries = {}
    holes = 0
    for i in range(n):
        stab, = struct.unpack_from("<I", stabs, 4*i)
        value, = struct.unpack_from("<Q", weights, 8*i)
        if not stab:
            holes += 1
            continue
        key = struct.unpack_from("<12H", keys, 24*i)
        decode_state(key, h.c, h.layer)
        if key in entries or ((1 << h.c)*factorial(h.c)) % stab:
            raise ValueError("duplicate key or invalid stabilizer")
        entries[key] = (stab, value)
    if holes != h.holes:
        raise ValueError("native hole count mismatch")
    return h, raw, entries


def altered_source(source: Path, target: Path, *, append_hole=False, poison=False,
                   wrong_stabilizer=False, noncanonical=False) -> None:
    h, raw, _ = native_image(source)
    n = h.n_entries
    header = bytearray(raw[:128])
    keys = raw[128:128+24*n]
    weights = raw[128+24*n:128+32*n]
    stabs = raw[128+32*n:]
    if wrong_stabilizer:
        changed = bytearray(stabs)
        for i in range(n):
            old, = struct.unpack_from("<I", changed, 4*i)
            if old:
                struct.pack_into("<I", changed, 4*i, 1 if old != 1 else 2)
                break
        stabs = bytes(changed)
    if noncanonical:
        changed = bytearray(keys)
        found = False
        for i in range(n):
            stab, = struct.unpack_from("<I", stabs, 4*i)
            if not stab:
                continue
            original = struct.unpack_from("<12H", keys, 24*i)
            swapped = tuple(sorted(word ^ (1 if (word & 3) else 0)
                                   for word in original[:2*h.c])) + original[2*h.c:]
            if swapped != original:
                decode_state(swapped, h.c, h.layer)
                struct.pack_into("<12H", changed, 24*i, *swapped)
                found = True
                break
        if not found:
            raise ValueError("fixture unexpectedly has no noncanonical coordinate image")
        keys = bytes(changed)
    if poison:
        weights = struct.pack("<Q", 1)*n
    if append_hole:
        keys += bytes(24)
        weights += bytes(8)
        stabs += bytes(4)
        struct.pack_into("<Q", header, 40, n+1)
        struct.pack_into("<Q", header, 48, h.holes+1)
    payload = ck_hash64(stabs, ck_hash64(weights, ck_hash64(keys, CK_SEED)))
    struct.pack_into("<Q", header, 112, payload)
    struct.pack_into("<Q", header, 120, 0)
    struct.pack_into("<Q", header, 120, ck_hash64(header, CK_SEED))
    exclusive(target, header+keys+weights+stabs)
    native_image(target)


def chunk_data(path: Path) -> bytearray:
    raw = bytearray(path.read_bytes())
    if len(raw) < CHUNK_BYTES or raw[:8] not in (b"SFR4CK01", b"SFR4MT01"):
        raise ValueError("unexpected shared chunk format")
    count, = struct.unpack_from("<Q", raw, CHUNK_OFF["count"])
    if len(raw) != CHUNK_BYTES+12*count:
        raise ValueError("unexpected shared chunk length")
    wanted, = struct.unpack_from("<Q", raw, CHUNK_OFF["header"])
    test = bytearray(raw[:CHUNK_BYTES])
    struct.pack_into("<Q", test, CHUNK_OFF["header"], 0)
    if ck_hash64(test, CK_SEED) != wanted:
        raise ValueError("independent shared header checksum mismatch")
    if count:
        a = raw[CHUNK_BYTES:CHUNK_BYTES+4*count]
        v = raw[CHUNK_BYTES+4*count:]
        wanted, = struct.unpack_from("<Q", raw, CHUNK_OFF["payload"])
        if ck_hash64(v, ck_hash64(a, CK_SEED)) != wanted:
            raise ValueError("independent shared payload checksum mismatch")
    return raw


def reseal(raw: bytearray) -> None:
    count, = struct.unpack_from("<Q", raw, CHUNK_OFF["count"])
    if count:
        a, v = raw[256:256+4*count], raw[256+4*count:]
        struct.pack_into("<Q", raw, CHUNK_OFF["payload"], ck_hash64(v, ck_hash64(a, CK_SEED)))
    struct.pack_into("<Q", raw, CHUNK_OFF["header"], 0)
    struct.pack_into("<Q", raw, CHUNK_OFF["header"], ck_hash64(raw[:256], CK_SEED))


class Gate:
    def __init__(self, args, out):
        self.args, self.out = args, out
        self.checks = []

    def run(self, cmd, name, reject=None):
        log = self.out / (name+".log")
        start = time.monotonic()
        with log.open("x", encoding="utf-8") as output:
            result = subprocess.run(cmd, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT,
                                    timeout=self.args.seconds_per_process, check=False)
        text = log.read_text(encoding="utf-8")
        if reject is None:
            if result.returncode:
                raise RuntimeError(f"{name} failed: exit {result.returncode}; {log}")
        elif result.returncode == 0 or reject.lower() not in text.lower():
            raise RuntimeError(f"{name} did not reject for {reject!r}; {log}")
        record = dict(test=name, exit=result.returncode, seconds=time.monotonic()-start,
                      log=log.name, expected_rejection=reject)
        self.checks.append(record)
        print(("REJECT " if reject else "PASS ")+name+f" seconds={record['seconds']:.3f}", flush=True)
        return text

    def cmd(self, c, source, namespace, *, chunk=500, limit=200000, threads=None,
            verify=None, export=None):
        seconds = self.args.seconds_per_process
        cmd = [str(self.args.shared_exe.resolve()), str(c), f"catalog={source}",
               f"output={namespace}", f"chunk={chunk}", f"limit={limit}",
               f"threads={threads or self.args.threads}", "maxrssgib=2",
               f"workseconds={seconds-15}", f"maxseconds={seconds-5}", "checkpointreadonly"]
        if verify is not None:
            cmd.append(f"verify={verify}")
        if export is not None:
            cmd.append(f"export={export}")
        return cmd

    def fixture(self, c):
        stem = self.out / f"reference-c{c}"
        cmd = [str(self.args.layer_exe.resolve()), str(c), "--threads", "1",
               "--caps", ",".join(str(max(100, n*2)) for n in COUNTS[c][1:]),
               "--invariance", "50", "--scan-check", "50"]
        if c == 5:
            cmd += ["--ref", str(REFERENCE)]
        for layer in (range(4, 6) if c == 5 else (4,)):
            cmd += ["--save-layer", str(layer), str(stem.with_suffix(f".L{layer}.snap"))]
        result = self.run(cmd, f"reference-c{c}")
        if str(KNOWN[c]) not in result or "[OK]" not in result:
            raise ValueError("independent reference did not reproduce known N")
        for layer in (range(4, 6) if c == 5 else (4,)):
            export_closed(stem.with_suffix(f".L{layer}.snap"),
                          stem.with_suffix(f".L{layer}.txt"), c, layer)
        return stem

    def readback(self, reference, exported, c, label):
        original, _, original_values = native_image(reference)
        closed, _, values = native_image(exported)
        if original_values != values or original.holes != closed.holes:
            raise ValueError(f"{label}: independent exported weighted values differ")
        text = self.out / (label+".txt")
        info = export_closed(exported, text, c, 4)
        self.checks.append(dict(test=label, values_checked=len(values), **info))
        print(f"PASS {label} native_values={len(values)} independent_payload_hash=1", flush=True)
        return text

    @staticmethod
    def committed(namespace):
        return {p.name: sha(p) for p in namespace.glob("*.bin")}

    @staticmethod
    def preserved(namespace, previous):
        if any(not (namespace/name).is_file() or sha(namespace/name) != digest
               for name, digest in previous.items()):
            raise ValueError("previously committed namespace bytes changed")

    def corrupt_copy(self, original, label, mutate):
        target = self.out / label
        shutil.copytree(original, target)
        mutate(target)
        return target

    def write_mutation(self, path, raw):
        # Only a freshly copied, disposable gate namespace may be modified.
        if self.out not in path.resolve().parents:
            raise ValueError("mutation escaped the disposable gate directory")
        with path.open("r+b") as handle:
            handle.write(raw)
            handle.truncate()

    def corruption(self, c4, good, holes, hole_source):
        source, reference = c4.with_suffix(".L4.snap"), c4.with_suffix(".L4.txt")
        first = sorted(good.glob("chunk-*.bin"))[0].name
        for label, error, offset, seal in (
            ("bad-header", "header/version checksum", 176, False),
            ("bad-payload", "payload checksum", 256, False),
            ("bad-lineage", "different source", 16, True),
        ):
            def mutate(directory, offset=offset, seal=seal, label=label):
                path = directory / ("manifest.bin" if label == "bad-lineage" else first)
                raw = chunk_data(path)
                raw[offset] ^= 1
                if seal:
                    reseal(raw)
                self.write_mutation(path, raw)
            target = self.corrupt_copy(good, label, mutate)
            before = self.committed(target)
            self.run(self.cmd(4, source, target, chunk=7, verify=reference), label, error)
            self.preserved(target, before)

        def bad_alias(directory):
            path = directory / first
            raw = chunk_data(path)
            struct.pack_into("<I", raw, 256, 26)
            reseal(raw)
            self.write_mutation(path, raw)
        target = self.corrupt_copy(good, "bad-alias-range", bad_alias)
        self.run(self.cmd(4, source, target, chunk=7), "bad-alias-range", "live catalogue ID")
        self.run(self.cmd(4, source, good, chunk=8), "bad-chunk-geometry", "different source")

        hole_chunk = next(holes.glob("chunk-*.bin")).name
        for label, record, alias, error in (
            ("bad-hole-record", 26, 0, "hole has"),
            ("bad-alias-to-hole", 0, 26, "live catalogue ID"),
        ):
            def mutate(directory, record=record, alias=alias):
                path = directory / hole_chunk
                raw = chunk_data(path)
                struct.pack_into("<I", raw, 256+4*record, alias)
                reseal(raw)
                self.write_mutation(path, raw)
            target = self.corrupt_copy(holes, label, mutate)
            self.run(self.cmd(4, hole_source, target, chunk=27), label, error)

    def kill_resume(self, c5):
        namespace = self.out / "c5-killed"
        source, reference = c5.with_suffix(".L4.snap"), c5.with_suffix(".L4.txt")
        command = self.cmd(5, source, namespace, chunk=100, threads=1)
        log = self.out / "c5-killed.log"
        start = time.monotonic()
        with log.open("x", encoding="utf-8") as output:
            process = subprocess.Popen(command, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT)
            try:
                first = namespace / "chunk-0000000000000000.bin"
                while not first.exists():
                    if process.poll() is not None or time.monotonic()-start > 30:
                        raise RuntimeError("kill test did not reach a committed first chunk")
                    time.sleep(0.005)
                self.run(command, "concurrent-namespace-refused", "another process owns")
                if process.poll() is not None:
                    raise RuntimeError("kill test completed before deliberate interruption")
                process.kill()
                process.wait(timeout=10)
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait(timeout=10)
        chunks = sorted(namespace.glob("chunk-*.bin"))
        counts = [struct.unpack_from("<Q", chunk_data(p), 128)[0] for p in chunks]
        if not 0 < sum(counts) < 17120:
            raise ValueError("kill test did not preserve a strict nonempty closed prefix")
        before = self.committed(namespace)
        # A killed incomplete temporary must never be mistaken for a chunk.
        exclusive(namespace / "chunk-orphan.bin.tmp-gate", b"uncommitted partial bytes")
        result = self.run(self.cmd(5, source, namespace, chunk=100, verify=reference), "c5-kill-resume")
        summary(result, 17120)
        if "exact_verified_native_values=17120" not in result:
            raise ValueError("kill-resumed run did not check all native values")
        self.preserved(namespace, before)
        self.checks.append(dict(test="killed-prefix-preservation", chunks=len(chunks),
                                closed_prefix=sum(counts), unchanged_sha256=True))

    def alias_cycle(self, c5, namespace):
        candidates = []
        for path in sorted(namespace.glob("chunk-*.bin")):
            raw = chunk_data(path)
            begin, count = struct.unpack_from("<QQ", raw, CHUNK_OFF["begin"])
            for j in range(count):
                alias, = struct.unpack_from("<I", raw, 256+4*j)
                value, = struct.unpack_from("<Q", raw, 256+4*count+8*j)
                if alias != begin+j and value == 0:
                    candidates.append((path.name, j, begin+j))
                    if len(candidates) == 2:
                        break
            if len(candidates) == 2:
                break
        if len(candidates) != 2:
            raise ValueError("complete C5 fixture did not expose two nonrepresentatives")

        def mutate(directory):
            changed = {}
            for position, (name, local, _) in enumerate(candidates):
                raw = changed.setdefault(name, chunk_data(directory/name))
                other_id = candidates[1-position][2]
                struct.pack_into("<I", raw, 256+4*local, other_id)
            for name, raw in changed.items():
                reseal(raw)
                self.write_mutation(directory/name, raw)
        target = self.corrupt_copy(namespace, "bad-alias-cycle", mutate)
        before = self.committed(target)
        self.run(self.cmd(5, c5.with_suffix(".L4.snap"), target),
                 "bad-alias-cycle", "alias/closed-value closure")
        self.preserved(target, before)

    def sample(self, source):
        if source.stat().st_size > 16 << 20:
            raise ValueError("C6 synthetic text gate is capped at 16 MiB")
        before = sha(source)
        copied = self.out/"c6-synthetic-closure.txt"
        shutil.copyfile(source, copied)
        count = sum(bool(line.strip()) and not line.lstrip().startswith("#")
                    for line in copied.read_text(encoding="ascii").splitlines())
        if not 1 <= count <= 200000:
            raise ValueError("C6 synthetic text row count outside explicit small bound")
        cmd = self.cmd(6, copied, self.out/"c6-synthetic-closure", chunk=1000, verify=copied)
        cmd = [f"sampletext={copied}" if arg.startswith("catalog=") else arg for arg in cmd]
        result = self.run(cmd, "c6-synthetic-complete-fibers")
        state = summary(result, count)
        if state.get("domain") != "sample_only" or f"exact_verified_native_values={count}" not in result:
            raise ValueError("C6 synthetic gate confused sample and complete support")
        if sha(source) != before or sha(copied) != before:
            raise ValueError("synthetic source was modified")
        self.checks.append(dict(test="c6-synthetic-scope", rows=count, source_sha256=before,
                                complete_c6_catalogue=False, production_checkpoint_io=False))

    def gather(self, c5, preload):
        source = c5.with_suffix(".L5.txt")
        command = [str(self.args.native_exe.resolve()), "5", "5", f"input={source}",
                   f"preload={preload}", "limit=355", "maxrecords=5000000",
                   "maxstates=200000", "maxseconds=120"]
        result = self.run(command, "c5-gather-from-new-export")
        data = checked_summary(result, 355, closed=True)
        if (int(data["matches"]), int(data["weak_residuals"])) != (425652, 270870):
            raise ValueError("complete final gather workload differs")
        values = [int(match.group(1)) for line in result.splitlines()
                  if (match := re.fullmatch(r"row=\d+ matches=\d+ raw=\d+ F=(\d+)", line))]
        h, raw, _ = native_image(c5.with_suffix(".L5.snap"))
        total = position = 0
        for i in range(h.n_entries):
            stab, = struct.unpack_from("<I", raw, 128+32*h.n_entries+4*i)
            if not stab:
                continue
            masks = decode_state(struct.unpack_from("<12H", raw, 128+24*i), 5, 5)
            if position >= len(values):
                raise ValueError("gather omitted a final value")
            denominator = 1
            for count in Counter(masks).values():
                denominator *= factorial(count)
            total += (3840//stab)*(factorial(10)//denominator)*values[position]**2
            position += 1
        if position != 355 or len(values) != 355 or total != KNOWN[5]:
            raise ValueError("new-export downstream square sum did not reproduce N5")
        self.checks.append(dict(test="new-export-final-square", classes=position, n=total,
                                predecessor_source=str(preload), old_weights_used=False))
        print(f"PASS new-export-final-square classes=355 N5={total}", flush=True)

    def reverse_core(self, c5, exported, preload):
        original = sha(exported)
        for threads in (1, self.args.threads):
            command = [str(self.args.reverse_exe.resolve()), "5", f"catalog={exported}",
                       f"input={c5.with_suffix('.L5.txt')}", f"preload={preload}",
                       "mode=exact", "limit=355", "maxrecords=5000000",
                       f"threads={threads}", "maxseconds=120", "maxrssgib=2", "checkpointreadonly"]
            text = self.run(command, f"c5-reverse-core-{threads}-threads")
            lines = [line for line in text.splitlines() if line.startswith("summary ")]
            if len(lines) != 1 or "[OK] COMPLETE_C5_ALL355_EXACT_F5" not in text:
                raise ValueError("reverse core did not explicitly close its exact complete C5 gate")
            data = fields(lines[0])
            expected = dict(sources=355, threads=threads, labelled_matchings=425652,
                            weak_residuals=270870, hits=270870, misses=0, checked=355)
            if any(int(data.get(key, -1)) != value for key, value in expected.items()):
                raise ValueError("reverse core full exact workload differs")
            if data.get("mode") != "exact" or "zero_predecessor_refused=yes" not in text:
                raise ValueError("reverse core accepted zero/unclosed predecessor values")
            if sha(exported) != original:
                raise ValueError("reverse core modified its newly exported source")
            self.checks.append(dict(test=f"reverse-core-exact-{threads}", summary=data,
                                    predecessor_source=str(preload), old_weights_used=False))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--shared-exe", type=Path, default=ROOT/"build/layer_shared_f4.exe")
    parser.add_argument("--layer-exe", type=Path, default=ROOT/"build/layer_dp_gate.exe")
    parser.add_argument("--native-exe", type=Path, default=ROOT/"build/layer_native_gather_bench.exe")
    parser.add_argument("--reverse-exe", type=Path, default=ROOT/"build/layer_reverse_f5_bench.exe")
    parser.add_argument("--threads", type=int, choices=range(2, 25), default=4)
    parser.add_argument("--seconds-per-process", type=int, default=180)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--c6-sample", type=Path,
                        help="optional independently valued, complete-fiber TEXT sample; never a checkpoint")
    args = parser.parse_args()
    if not 30 <= args.seconds_per_process <= 600:
        parser.error("per-process bound must be 30..600 seconds")
    for exe in (args.shared_exe, args.layer_exe, args.native_exe, args.reverse_exe):
        if not exe.is_file():
            parser.error(f"missing executable: {exe}")
    if sha(REFERENCE) != REFERENCE_SHA256:
        raise ValueError("independent C5 reference hash changed")
    if args.output is None:
        (ROOT/"data/logs").mkdir(exist_ok=True)
        out = Path(tempfile.mkdtemp(prefix="layer-shared-gate-", dir=ROOT/"data/logs"))
    else:
        out = args.output.resolve()
        out.mkdir(parents=True, exist_ok=False)
    print(f"ARTIFACTS {out}", flush=True)
    start = time.monotonic()
    gate = Gate(args, out)
    c4, c5 = gate.fixture(4), gate.fixture(5)
    original_hashes = {p: sha(p) for p in out.glob("reference-*.snap")}
    s4, t4 = c4.with_suffix(".L4.snap"), c4.with_suffix(".L4.txt")
    s5, t5 = c5.with_suffix(".L4.snap"), c5.with_suffix(".L4.txt")
    good4, good5 = out/"closed-c4", out/"closed-c5"
    exported4, exported5 = out/"shared-c4.L4.snap", out/"shared-c5.L4.snap"
    result = gate.run(gate.cmd(4, s4, good4, chunk=7, threads=1,
                               verify=t4, export=exported4), "c4-complete")
    summary(result, 26)
    gate.readback(s4, exported4, 4, "c4-independent-readback")
    partial = gate.run(gate.cmd(5, s5, good5, limit=2000, threads=1), "c5-bounded-prefix")
    state = summary(partial)
    if state.get("status") != "INCOMPLETE_RESUMABLE" or int(state["closed_prefix"]) != 2000:
        raise ValueError("bounded prefix did not stop exactly at 2000")
    prior = gate.committed(good5)
    result = gate.run(gate.cmd(5, s5, good5, verify=t5, export=exported5), "c5-resume-complete")
    summary(result, 17120)
    if "exact_verified_native_values=17120" not in result:
        raise ValueError("complete C5 per-native-value gate missing")
    gate.preserved(good5, prior)
    preload = gate.readback(s5, exported5, 5, "c5-independent-readback")
    gate.gather(c5, preload)
    gate.reverse_core(c5, exported5, preload)
    closed_hash = sha(exported5)
    gate.run(gate.cmd(5, s5, good5, export=exported5), "export-overwrite-refused", "refusing to overwrite")
    if sha(exported5) != closed_hash:
        raise ValueError("overwrite rejection changed exported snapshot")

    poison = out/"poisoned-c4.L4.snap"
    altered_source(s4, poison, poison=True)
    result = gate.run(gate.cmd(4, poison, out/"poisoned-source-recomputed", chunk=26,
                               verify=t4), "old-weights-discarded")
    summary(result, 26)
    if "exact_verified_native_values=26" not in result:
        raise ValueError("poisoned old weights were not independently replaced")
    hole_source, holes = out/"c4-with-hole.snap", out/"closed-c4-with-hole"
    altered_source(s4, hole_source, append_hole=True)
    hole_export = out/"shared-c4-with-hole.snap"
    result = gate.run(gate.cmd(4, hole_source, holes, chunk=27, verify=t4,
                               export=hole_export), "legal-hole-preserved")
    summary(result, 27)
    gate.readback(hole_source, hole_export, 4, "hole-export-independent-readback")
    gate.corruption(c4, good4, holes, hole_source)
    gate.alias_cycle(c5, good5)
    for label, options, reason in (
        ("bad-source-stabilizer", dict(wrong_stabilizer=True), "stabilizer"),
        ("bad-source-canonical-key", dict(noncanonical=True), "canonical"),
    ):
        bad_source = out/(label+".snap")
        altered_source(s4, bad_source, **options)
        gate.run(gate.cmd(4, bad_source, out/(label+"-namespace"), chunk=26), label, reason)
    gate.kill_resume(c5)
    if args.c6_sample is not None:
        gate.sample(args.c6_sample.resolve())
    if any(sha(path) != digest for path, digest in original_hashes.items()):
        raise ValueError("an independent source snapshot was modified")
    with (out/"checks.json").open("x", encoding="utf-8") as handle:
        json.dump(dict(checks=gate.checks, source_sha256={str(p):v for p,v in original_hashes.items()},
                       source_unchanged=True, production_checkpoint_io=False,
                       c6_synthetic_sample_ran=args.c6_sample is not None,
                       elapsed_seconds=time.monotonic()-start), handle, indent=2)
    print(f"SHARED F4 EXACTNESS AND RECOVERY GATES PASSED seconds={time.monotonic()-start:.3f}", flush=True)


if __name__ == "__main__":
    main()
