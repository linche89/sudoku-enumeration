#!/usr/bin/env python3
"""Fresh shared-F4 -> actual reverse-F5 -> native final certificate gate.

All computation is complete C5 on fresh disposable fixtures. This runner does
not discover, read, write, or resume any production C6 checkpoint. Corruption
probes edit only newly copied artifacts inside this runner's own directory.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import time

from layer_shared_gate import (Gate, altered_source, exclusive, fields, native_image,
                               sha, summary as f4_summary)
from layer_direct_gate import COUNTS, KNOWN, REFERENCE, REFERENCE_SHA256, ROOT, export_closed
from layer_dp_progress import CK_SEED, ck_hash64

OFF = dict(total=176, chunk=184, begin=192, count=200, payload=208,
           header=216, live=224, checksum=232)


def summary(text, closed=None):
    rows = [line for line in text.splitlines() if line.startswith("SUMMARY ")]
    if len(rows) != 1:
        raise ValueError("missing or ambiguous production reverse summary")
    result = fields(rows[0])
    if result.get("N6") != "NOT_COMPUTED" or result.get("source_checkpointreadonly") != "yes":
        raise ValueError("missing readonly/scope marker")
    if closed is not None:
        expected = dict(closed_prefix=closed, total_entries=closed, live_closed=355)
        if result.get("status") != "CLOSED_F5_CATALOGUE" or any(
                int(result.get(key, -1)) != value for key, value in expected.items()):
            raise ValueError("production reverse catalogue is not fully closed")
    return result


def chunk_data(path):
    raw = bytearray(path.read_bytes())
    if len(raw) < 256 or raw[:8] not in (b"RVF5CK01", b"RVF5MT01"):
        raise ValueError("unexpected distinct F5 chunk format")
    count, = struct.unpack_from("<Q", raw, OFF["count"])
    if len(raw) != 256+8*count:
        raise ValueError("F5 chunk length mismatch")
    wanted, = struct.unpack_from("<Q", raw, OFF["header"])
    test = bytearray(raw[:256])
    struct.pack_into("<Q", test, OFF["header"], 0)
    if ck_hash64(test, CK_SEED) != wanted:
        raise ValueError("independent F5 header checksum mismatch")
    if count:
        wanted, = struct.unpack_from("<Q", raw, OFF["payload"])
        if ck_hash64(raw[256:], CK_SEED) != wanted:
            raise ValueError("independent F5 payload checksum mismatch")
    return raw


def reseal_chunk(raw):
    count, = struct.unpack_from("<Q", raw, OFF["count"])
    if count:
        struct.pack_into("<Q", raw, OFF["payload"], ck_hash64(raw[256:], CK_SEED))
    struct.pack_into("<Q", raw, OFF["header"], 0)
    struct.pack_into("<Q", raw, OFF["header"], ck_hash64(raw[:256], CK_SEED))


def reseal_native(raw):
    n, = struct.unpack_from("<Q", raw, 40)
    if len(raw) != 128+36*n:
        raise ValueError("mutation requires a narrow native image")
    keys, weights, stabs = raw[128:128+24*n], raw[128+24*n:128+32*n], raw[128+32*n:]
    payload = ck_hash64(stabs, ck_hash64(weights, ck_hash64(keys, CK_SEED)))
    struct.pack_into("<Q", raw, 112, payload)
    struct.pack_into("<Q", raw, 120, 0)
    struct.pack_into("<Q", raw, 120, ck_hash64(raw[:128], CK_SEED))


class ReverseGate(Gate):
    def cmd(self, l4, support, namespace, *, chunk=50, limit=355,
            threads=None, verify=None, export=None, digest=None):
        seconds = self.args.seconds_per_process
        command = [str(self.args.reverse_exe.resolve()), "5", f"l4={l4}",
                   f"l4sha256={digest or sha(l4)}", f"l5support={support}",
                   f"output={namespace}", f"chunk={chunk}", f"limit={limit}",
                   f"threads={threads or self.args.threads}", "maxrssgib=2",
                   f"workseconds={seconds-15}", f"maxseconds={seconds-5}", "checkpointreadonly"]
        if verify is not None:
            command.append(f"verify={verify}")
        if export is not None:
            command.append(f"export={export}")
        return command

    def readback5(self, original, exported, label):
        source_header, source_bytes, source_values = native_image(original)
        target_header, target_bytes, target_values = native_image(exported)
        if (source_values != target_values or source_header.holes != target_header.holes or
                source_bytes != target_bytes):
            raise ValueError("new native L5 export is not per-key AND byte-identical to independent reference")
        info = export_closed(exported, self.out/(label+".txt"), 5, 5)
        if info["states"] != 355 or info["n"] != KNOWN[5]:
            raise ValueError("new reverse export did not independently reproduce N5")
        self.checks.append(dict(test=label, byte_identical=True, **info))
        print(f"PASS {label} all355_values=1 byte_identical=1 N5={info['n']}", flush=True)

    def negatives(self, l4, support, reference, good, exported):
        first = sorted(good.glob("f5-chunk-*.bin"))[0].name
        for label, reason, filename, offset, seal in (
            ("bad-f5-header", "header/domain/range checksum", first, 240, False),
            ("bad-f5-payload", "payload checksum", first, 256, False),
            ("bad-l4-lineage", "different inputs", "manifest.bin", 16, True),
            ("bad-l5-lineage", "different inputs", "manifest.bin", 80, True),
        ):
            def mutate(directory, filename=filename, offset=offset, seal=seal):
                path = directory/filename
                raw = chunk_data(path)
                if seal:
                    raw[offset] = ord("0") if raw[offset] != ord("0") else ord("1")
                    reseal_chunk(raw)
                else:
                    raw[offset] ^= 1
                self.write_mutation(path, raw)
            target = self.corrupt_copy(good, label, mutate)
            before = self.committed(target)
            self.run(self.cmd(l4, support, target), label, reason)
            self.preserved(target, before)

        def zero(directory):
            path = directory/first
            raw = chunk_data(path)
            struct.pack_into("<Q", raw, 256, 0)
            reseal_chunk(raw)
            self.write_mutation(path, raw)
        target = self.corrupt_copy(good, "zero-live-f5", zero)
        self.run(self.cmd(l4, support, target), "zero-live-f5", "positive CLOSED live values")
        def nonfactorial(directory):
            path = directory/first
            raw = chunk_data(path)
            value, = struct.unpack_from("<Q", raw, 256)
            struct.pack_into("<Q", raw, 256, value+1)
            reseal_chunk(raw)
            self.write_mutation(path, raw)
        target = self.corrupt_copy(good, "nonfactorial-f5", nonfactorial)
        self.run(self.cmd(l4, support, target), "nonfactorial-f5", "closed F5 must be divisible")
        self.run(self.cmd(l4, support, good, chunk=51), "changed-chunk-geometry", "different inputs")
        before_export = sha(exported)
        self.run(self.cmd(l4, support, good, export=exported), "export-overwrite", "export target already exists")
        if sha(exported) != before_export:
            raise ValueError("refused export overwrite modified the snapshot")
        self.run(self.cmd(l4, support, self.out/"wrong-l4-sha", digest="0"*64),
                 "wrong-l4-sha", "full SHA-256 mismatch")
        command = self.cmd(l4, support, self.out/"missing-readonly")
        self.run([arg for arg in command if arg != "checkpointreadonly"],
                 "missing-readonly", "require C5/C6, readonly inputs")
        self.run(self.cmd(l4, support, self.out), "input-ancestor-overlap",
                 "readonly input cannot reside inside writable F5 namespace")

        for label, options in (
            ("bad-target-stabilizer", dict(wrong_stabilizer=True)),
            ("bad-target-canonical-key", dict(noncanonical=True)),
        ):
            bad = self.out/(label+".snap")
            altered_source(support, bad, **options)
            namespace = self.out/(label+"-namespace")
            digest = sha(bad)
            self.run(self.cmd(l4, bad, namespace), label, "semantic audit failed")
            if namespace.exists() or sha(bad) != digest:
                raise ValueError("invalid target created persistent state or changed its input")

        bad = self.out/"bad-target-hole-count.snap"
        raw = bytearray(support.read_bytes())
        struct.pack_into("<Q", raw, 48, 1)
        reseal_native(raw)
        exclusive(bad, raw)
        self.run(self.cmd(l4, bad, self.out/"bad-target-hole-count"),
                 "bad-target-hole-count", "full designated L5 support")

        bad = self.out/"zero-closed-l4.snap"
        raw = bytearray(l4.read_bytes())
        n, = struct.unpack_from("<Q", raw, 40)
        struct.pack_into("<Q", raw, 128+24*n, 0)
        reseal_native(raw)
        exclusive(bad, raw)
        self.run(self.cmd(bad, support, self.out/"zero-closed-l4"),
                 "zero-closed-l4", "L4 weighted T has zero live values")

        bad = self.out/"nonfactorial-closed-l4.snap"
        raw = bytearray(l4.read_bytes())
        n, = struct.unpack_from("<Q", raw, 40)
        value, = struct.unpack_from("<Q", raw, 128+24*n)
        stab, = struct.unpack_from("<I", raw, 128+32*n)
        struct.pack_into("<Q", raw, 128+24*n, value+3840//stab)
        reseal_native(raw)
        exclusive(bad, raw)
        self.run(self.cmd(bad, support, self.out/"nonfactorial-closed-l4"),
                 "nonfactorial-closed-l4", "nonfactorial F4")

    def poison_and_holes(self, l4, support, reference):
        poison = self.out/"poisoned-old-l5.snap"
        altered_source(support, poison, poison=True)
        digest = sha(poison)
        exported = self.out/"poison-recomputed.L5.snap"
        text = self.run(self.cmd(l4, poison, self.out/"poison-recomputed", verify=reference,
                                 export=exported), "old-target-weights-discarded")
        summary(text, 355)
        if "old_T=erased" not in text or sha(poison) != digest:
            raise ValueError("old target weights were not demonstrably discarded without input mutation")
        self.readback5(support, exported, "poison-recomputed-readback")

        hole_source = self.out/"l5-support-with-hole.snap"
        altered_source(support, hole_source, append_hole=True)
        hole_namespace = self.out/"l5-with-hole"
        hole_export = self.out/"l5-with-hole.snap"
        text = self.run(self.cmd(l4, hole_source, hole_namespace, chunk=356, limit=356,
                                 verify=reference, export=hole_export), "legal-target-hole")
        summary(text, 356)
        self.readback5(hole_source, hole_export, "hole-export-readback")

        def mutate(directory):
            path = next(directory.glob("f5-chunk-*.bin"))
            raw = chunk_data(path)
            struct.pack_into("<Q", raw, 256+8*355, 1)
            reseal_chunk(raw)
            self.write_mutation(path, raw)
        target = self.corrupt_copy(hole_namespace, "nonzero-f5-hole", mutate)
        self.run(self.cmd(l4, hole_source, target, chunk=356, limit=356),
                 "nonzero-f5-hole", "positive CLOSED live values and zero holes")

    def kill_resume(self, l4, support, reference):
        namespace = self.out/"killed-f5"
        log = self.out/"killed-f5.log"
        start = time.monotonic()
        with log.open("x", encoding="utf-8") as output:
            process = subprocess.Popen(self.cmd(l4, support, namespace, chunk=1, threads=1),
                                       cwd=ROOT, stdout=output, stderr=subprocess.STDOUT)
            try:
                first = namespace/"f5-chunk-0000000000000000.bin"
                while not first.exists():
                    if process.poll() is not None or time.monotonic()-start > 30:
                        raise RuntimeError("F5 kill test did not reach the first committed chunk")
                    time.sleep(0.002)
                if process.poll() is not None:
                    raise RuntimeError("F5 process completed before deliberate interruption")
                process.kill()
                process.wait(timeout=10)
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait(timeout=10)
        chunks = sorted(namespace.glob("f5-chunk-*.bin"))
        counts = [struct.unpack_from("<Q", chunk_data(path), OFF["count"])[0] for path in chunks]
        if not 0 < sum(counts) < 355:
            raise ValueError("kill test lacks a strict nonempty durable F5 prefix")
        before = self.committed(namespace)
        exclusive(namespace/"f5-chunk-orphan.tmp-gate", b"uncommitted values")
        exported = self.out/"kill-resumed.L5.snap"
        result = self.run(self.cmd(l4, support, namespace, chunk=1, verify=reference,
                                   export=exported), "kill-resume-complete")
        summary(result, 355)
        self.preserved(namespace, before)
        self.readback5(support, exported, "kill-resume-readback")
        self.checks.append(dict(test="actual-killed-F5-prefix", closed_prefix=sum(counts),
                                chunks=len(chunks), prior_chunk_sha256_preserved=True))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reverse-exe", type=Path, default=ROOT/"build/layer_reverse_f5.exe")
    parser.add_argument("--shared-exe", type=Path, default=ROOT/"build/layer_shared_f4.exe")
    parser.add_argument("--layer-exe", type=Path, default=ROOT/"build/layer_dp_gate.exe")
    parser.add_argument("--threads", type=int, choices=range(2, 25), default=4)
    parser.add_argument("--seconds-per-process", type=int, default=180)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if not 30 <= args.seconds_per_process <= 600:
        parser.error("per-process limit must be30..600 seconds")
    for exe in (args.reverse_exe, args.shared_exe, args.layer_exe):
        if not exe.is_file():
            parser.error(f"missing executable: {exe}")
    if sha(REFERENCE) != REFERENCE_SHA256:
        raise ValueError("independent C5 reference hash changed")
    if args.output is None:
        (ROOT/"data/logs").mkdir(exist_ok=True)
        out = Path(tempfile.mkdtemp(prefix="layer-reverse-gate-", dir=ROOT/"data/logs"))
    else:
        out = args.output.resolve()
        out.mkdir(parents=True, exist_ok=False)
    print(f"ARTIFACTS {out}", flush=True)
    start = time.monotonic()
    gate = ReverseGate(args, out)
    stem = gate.fixture(5)
    old_l4, support = stem.with_suffix(".L4.snap"), stem.with_suffix(".L5.snap")
    reference = stem.with_suffix(".L5.txt")
    protected = {path:sha(path) for path in (old_l4, support)}
    l4 = out/"new-shared.L4.snap"
    text = gate.run(Gate.cmd(gate, 5, old_l4, out/"new-shared-f4", chunk=500,
                            verify=stem.with_suffix(".L4.txt"), export=l4), "fresh-shared-F4")
    f4_summary(text, 17120)
    gate.readback(old_l4, l4, 5, "fresh-shared-F4-readback")
    protected[l4] = sha(l4)

    namespace = out/"closed-reverse-f5"
    text = gate.run(gate.cmd(l4, support, namespace, limit=100, threads=1), "bounded-F5-prefix")
    state = summary(text)
    if state.get("status") != "INCOMPLETE_RESUMABLE" or int(state["closed_prefix"]) != 100:
        raise ValueError("F5 bounded prefix did not stop exactly at100")
    previous = gate.committed(namespace)
    exported = out/"new-reverse.L5.snap"
    text = gate.run(gate.cmd(l4, support, namespace, verify=reference, export=exported), "resume-F5-complete")
    summary(text, 355)
    if "EXACT_VERIFIED_F5_NATIVE_VALUES=355" not in text:
        raise ValueError("production reverse did not compare every final native value")
    gate.preserved(namespace, previous)
    gate.readback5(support, exported, "complete-F5-independent-square")
    protected[exported] = sha(exported)

    caps = ",".join(str(max(100, n*2)) for n in COUNTS[5][1:])
    command = [str(args.layer_exe.resolve()), "5", "--threads", str(args.threads),
               "--caps", caps, "--load-layer", "5", str(exported), "--ref", str(REFERENCE),
               "--dump", str(out/"native-final-stage.csv")]
    result = gate.run(command, "native-final-stage-load-compatibility")
    if str(KNOWN[5]) not in result or "355 rows, 355 matched" not in result:
        raise ValueError("existing native final stage did not accept the fresh F5 export exactly")
    gate.negatives(l4, support, reference, namespace, exported)
    gate.poison_and_holes(l4, support, reference)
    gate.kill_resume(l4, support, reference)
    if any(sha(path) != digest for path, digest in protected.items()):
        raise ValueError("a readonly source or earlier closed export was modified")
    with (out/"checks.json").open("x", encoding="utf-8") as handle:
        json.dump(dict(checks=gate.checks, source_sha256={str(p):v for p,v in protected.items()},
                       source_unchanged=True, production_c6_checkpoint_io=False,
                       elapsed_seconds=time.monotonic()-start), handle, indent=2)
    print(f"ACTUAL REVERSE F5 END-TO-END AND RECOVERY GATES PASSED seconds={time.monotonic()-start:.3f}", flush=True)


if __name__ == "__main__":
    main()
