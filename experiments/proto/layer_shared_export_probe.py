#!/usr/bin/env python3
"""Bounded C5 qualification of the released shared-F4 export path.

This is NOT a production export controller. It constructs only disposable C5
copies, demonstrates that checkpointreadonly is not export-only, and checks
that a fully closed namespace resumes and exports without computing a chunk.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import tempfile

from layer_shared_gate import ROOT, native_image, summary


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fixtures", type=Path, required=True)
    args = parser.parse_args()
    fixture = args.fixtures.resolve()
    source = fixture / "reference-c5.L4.snap"
    namespace = fixture / "closed-c5"
    if native_image(source)[0].c != 5:
        raise ValueError("only C5 fixtures are permitted")
    exe = ROOT / "build/layer_shared_f4.exe"
    pins = {exe: digest(exe), source: digest(source)}
    original = {p.name: digest(p) for p in namespace.glob("*.bin")}
    out = Path(tempfile.mkdtemp(prefix="shared-export-probe-", dir=ROOT / "data/logs"))
    print(f"ARTIFACTS={out}", flush=True)

    def run(directory, output, name):
        command = [str(exe), "5", f"catalog={source}", f"output={directory}",
                   "chunk=500", "limit=500", "threads=1", "workseconds=20",
                   "maxseconds=30", "maxrssgib=2", "checkpointreadonly",
                   f"export={output}"]
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True,
                                timeout=40, check=False)
        with (out / (name + ".log")).open("x", encoding="utf-8") as log:
            log.write("COMMAND " + subprocess.list2cmdline(command) + "\n")
            log.write(result.stdout + result.stderr)
        if result.returncode:
            raise RuntimeError(f"{name} failed: {result.returncode}")
        return summary(result.stdout)

    complete = out / "complete"
    shutil.copytree(namespace, complete)
    before = {p.name: digest(p) for p in complete.glob("*.bin")}
    exported = out / "new-closed.L4.snap"
    result = run(complete, exported, "closed-resume-export")
    if (result["status"] != "CLOSED_F4_CATALOGUE" or
            int(result["new_chunks"]) or int(result["new_indices"]) or
            int(result["closed_prefix"]) != 17120):
        raise ValueError("closed namespace did not export with zero new work")
    if before != {p.name: digest(p) for p in complete.glob("*.bin")}:
        raise ValueError("committed chunks changed during closed resume")
    if native_image(source)[2] != native_image(exported)[2]:
        raise ValueError("independent all-key native readback differs")
    print(f"PASS closed_resume_export native_values=17120 new_chunks=0 new_indices=0 sha256={digest(exported)}",
          flush=True)

    # Construct a prefix without removing any file from the original/copy.
    prefix = out / "incomplete"
    prefix.mkdir()
    for name in ("manifest.bin", "chunk-0000000000000000.bin"):
        shutil.copy2(namespace / name, prefix / name)
    absent = out / "must-not-be-a-closed-export.snap"
    result = run(prefix, absent, "readonly-is-not-export-only")
    if (result["status"] != "INCOMPLETE_RESUMABLE" or
            int(result["new_chunks"]) != 1 or int(result["new_indices"]) != 500 or
            absent.exists()):
        raise ValueError("unexpected incomplete-namespace behavior")
    print("PASS scope_counterexample checkpointreadonly=yes new_chunks=1 new_indices=500 export_created=no",
          flush=True)
    if any(digest(p) != pin for p, pin in pins.items()):
        raise ValueError("read-only source or released executable changed")
    if original != {p.name: digest(p) for p in namespace.glob("*.bin")}:
        raise ValueError("original complete C5 namespace changed")
    print("SHARED EXPORT SCOPE PROBE PASSED; no C6 input opened", flush=True)


if __name__ == "__main__":
    main()
