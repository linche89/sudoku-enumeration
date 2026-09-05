#!/usr/bin/env python3
"""Bounded NEW-binary complete-C5 support audit. Never opens C6 sources.

Reuses the frozen production loader/canonicalizer, not its numerical entrypoint.
Only fresh disposable C5 copies are read; malformed probes are in child memory.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import time

import psutil

ROOT = Path(__file__).resolve().parents[2]


def sha(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(8 << 20), b""):
            h.update(block)
    return h.hexdigest().upper()


def pin(path):
    s = path.stat()
    return dict(path=str(path), sha256=sha(path), bytes=s.st_size, mtime_ns=s.st_mtime_ns)


class Guard:
    def __init__(self, out, max_seconds=120, max_bytes=2 << 30):
        self.out, self.max_seconds, self.max_bytes = out, max_seconds, max_bytes
        self.started = time.monotonic()
        self.peak = 0
        self.tracked = {}
        self.runs = []

    def live(self):
        result = []
        for pid, created in list(self.tracked.items()):
            try:
                p = psutil.Process(pid)
                if abs(p.create_time() - created) < .001 and p.is_running():
                    result.append(p)
            except psutil.NoSuchProcess:
                pass
        return result

    def run(self, command, label, expect=0, contains=None):
        before = time.monotonic()
        proc = None
        try:
            with (self.out / (label + ".stdout.log")).open("x", encoding="utf-8") as out, \
                 (self.out / (label + ".stderr.log")).open("x", encoding="utf-8") as err:
                proc = subprocess.Popen(command, cwd=ROOT, stdout=out, stderr=err,
                                        creationflags=subprocess.CREATE_NO_WINDOW)
                parent = psutil.Process(proc.pid)
                self.tracked[parent.pid] = parent.create_time()
                while proc.poll() is None:
                    try:
                        family = [parent] + parent.children(recursive=True)
                    except psutil.NoSuchProcess:
                        family = []
                    for p in family:
                        try:
                            self.tracked[p.pid] = p.create_time()
                        except psutil.NoSuchProcess:
                            pass
                    rss = psutil.Process().memory_info().rss
                    for p in self.live():
                        try:
                            rss += p.memory_info().rss
                        except psutil.NoSuchProcess:
                            pass
                    self.peak = max(self.peak, rss)
                    if time.monotonic() - self.started > self.max_seconds:
                        raise RuntimeError("external whole-suite 120-second guard")
                    if rss > self.max_bytes:
                        raise RuntimeError("external aggregate 2-GiB guard")
                    time.sleep(.05)
                code = proc.wait(timeout=5)
        finally:
            remaining = self.live()
            for p in reversed(remaining):
                try:
                    p.kill()
                except psutil.NoSuchProcess:
                    pass
            psutil.wait_procs(remaining, timeout=5)
            if proc is not None:
                proc.wait(timeout=5)
        stdout = (self.out / (label + ".stdout.log")).read_text(encoding="utf-8", errors="replace")
        stderr = (self.out / (label + ".stderr.log")).read_text(encoding="utf-8", errors="replace")
        self.runs.append(dict(label=label, command=command, exit_code=code,
                              wall_seconds=time.monotonic()-before))
        if code != expect or (contains and contains not in stdout + stderr):
            raise AssertionError(f"{label}: exit={code}; inspect retained stdout/stderr")
        return stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--l4", type=Path, required=True)
    parser.add_argument("--l5", type=Path, required=True)
    parser.add_argument("--cxx", default="g++")
    args = parser.parse_args()
    # Inspect small headers BEFORE reading/hashing/copying either fixture.
    originals = {}
    for degree, path in ((4, args.l4.resolve()), (5, args.l5.resolve())):
        if path.stat().st_size > 8 << 20:
            raise ValueError("this gate refuses a source larger than 8 MiB")
        with path.open("rb") as f:
            header = f.read(128)
        if len(header) != 128 or struct.unpack_from("<I", header, 12)[0] != 5 or \
                struct.unpack_from("<I", header, 16)[0] != degree:
            raise ValueError("complete C5 L4/L5 fixtures only")
        originals[degree] = path
    out = Path(tempfile.mkdtemp(prefix="support-semantic-small-", dir=ROOT / "data/logs"))
    build = Path(tempfile.mkdtemp(prefix="support-semantic-candidate-", dir=ROOT / "build"))
    executable = build / "layer_support_semantic_audit.exe"
    print("LOGDIR=" + str(out), flush=True)
    print("CANDIDATE=" + str(executable), flush=True)
    protected = [ROOT / "build/layer_shared_f4.exe", ROOT / "build/layer_reverse_f5.exe",
                 ROOT / "build/layer_dp_gate.exe", ROOT / "experiments/proto/layer_reverse_f5.cpp",
                 ROOT / "experiments/proto/layer_dp_gate.cpp"] + list(originals.values())
    before = [pin(p) for p in protected]
    guard = Guard(out)
    copies = {}
    for degree, path in originals.items():
        destination = out / f"disposable complete C5 L{degree}.snap"
        with destination.open("xb") as f, path.open("rb") as src:
            shutil.copyfileobj(src, f, 1 << 20)
        copies[degree] = destination
    compiler = shutil.which(args.cxx)
    if not compiler:
        raise ValueError("compiler unavailable")
    command = [compiler, "-O3", "-mpopcnt", "-std=c++20", "-fopenmp", "-Wall", "-Wextra",
               str(ROOT / "experiments/proto/layer_support_semantic_audit.cpp"),
               "-o", str(executable), "-lbcrypt", "-lpsapi"]
    guard.run(command, "build-new-diagnostic")
    summaries = []
    for degree in (4, 5):
        expected_live, expected_mass = (17120, 62185328) if degree == 4 else (355, 589392)
        for threads in (1, 2):
            options = [str(executable), "5", str(degree), "input=" + str(copies[degree]),
                       "sha256=" + sha(copies[degree]), "limit=200000", f"threads={threads}",
                       "maxseconds=110", "maxrssgib=2", "checkpointreadonly"]
            if threads == 2:
                options += ["selftest"]
            text = guard.run(options, f"complete-L{degree}-threads{threads}",
                             contains="ALL RECORD SUPPORT SEMANTICS PASSED")
            row = next(line for line in text.splitlines() if line.startswith("SUPPORT_SEMANTIC_AUDIT "))
            fields = dict(token.split("=", 1) for token in row.split()[1:])
            for key, expected in dict(live=expected_live, mass=expected_mass,
                                      audit_calls=expected_live, exact_ID_hits=expected_live).items():
                if int(fields[key]) != expected:
                    raise AssertionError(f"incomplete per-record coverage: {row}")
            if fields["F5"] != "NOT_COMPUTED" or fields["old_T"] != "ERASED" or \
                    fields["file_writes"] != "none" or int(fields["zero_T_entries"]) != int(fields["entries"]):
                raise AssertionError("support-only boundary missing")
            if threads == 2 and "SMALL_NEGATIVES refused=9 " not in text:
                raise AssertionError("missing all nine malformed cases")
            summaries.append(fields)
        base = options[:-1] if options[-1] == "selftest" else options
        guard.run([a for a in base if a != "checkpointreadonly"], f"L{degree}-missing-readonly", expect=1,
                  contains="require bounded readonly")
        bad_sha = ["sha256=" + "0"*64 if a.startswith("sha256=") else a for a in base]
        guard.run(bad_sha, f"L{degree}-wrong-SHA", expect=1, contains="full source SHA")
        bad_limit = ["limit=1" if a.startswith("limit=") else a for a in base]
        guard.run(bad_limit, f"L{degree}-short-limit", expect=1, contains="positive record bound")
    after = [pin(p) for p in protected]
    if before != after or guard.live():
        raise AssertionError("protected source/release binary changed or child remained")
    receipt = dict(status="PASS", scope="complete C5 support only; no C6 invocation",
                   wall_seconds=time.monotonic()-guard.started, sampled_peak_aggregate_rss_bytes=guard.peak,
                   external_seconds=120, external_bytes=2 << 30, tracked_processes=len(guard.tracked),
                   survivors=[], executable=pin(executable), before=before, after=after,
                   summaries=summaries, runs=guard.runs)
    with (out / "receipt.json").open("x", encoding="utf-8") as f:
        json.dump(receipt, f, indent=2)
    print(json.dumps(receipt, indent=2), flush=True)
    print("COMPLETE C5 SUPPORT SEMANTIC AUDIT GATES PASSED", flush=True)


if __name__ == "__main__":
    main()
