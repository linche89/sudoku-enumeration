#!/usr/bin/env python3
"""Small, disposable end-to-end gates for the closed-only export controller.

Uses an already qualified complete C5 namespace and constructs a fresh
independent C5 row-incremental oracle. No C6 input is discovered or opened.
"""
from __future__ import annotations

import argparse
import ctypes
from ctypes import wintypes
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
from types import SimpleNamespace
import uuid

from layer_shared_gate import Gate, ROOT, native_image, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("fixtures", "full-gate", "shared-gate", "direct-gate"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    out = Path(tempfile.mkdtemp(prefix="shared-export-gate-", dir=ROOT / "data/logs"))
    external = Path("D:/sudoku_FJ_checkpoint_backups") / ("shared-export-gate-" + uuid.uuid4().hex)
    external.mkdir(exist_ok=False)
    print(f"ARTIFACTS={out}\nEXTERNAL_FIXTURES={external}", flush=True)
    gate = Gate(SimpleNamespace(layer_exe=ROOT/"build/layer_dp_gate.exe", seconds_per_process=120), out)
    fresh = gate.fixture(5).with_suffix(".L4.snap")
    source = args.fixtures.resolve() / "reference-c5.L4.snap"
    source_image = native_image(source)
    if source_image[0].c != 5 or source_image[2] != native_image(fresh)[2]:
        raise ValueError("qualified C5 fixture disagrees with fresh independent oracle")
    source_copy = external / "source.L4.snap"
    shutil.copy2(source, source_copy)
    complete = out / "complete"
    backup = external / "complete"
    shutil.copytree(args.fixtures.resolve()/"closed-c5", complete)
    shutil.copytree(complete, backup)
    source_pin = sha(source)
    original_chunks = Gate.committed(complete)
    binaries = {p: sha(p) for p in (ROOT/"build/layer_shared_f4.exe", ROOT/"build/layer_reverse_f5.exe")}
    results = []

    def command(ns, ns_copy, exported, exported_copy):
        return ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
                str(ROOT/"scripts/export_layer_shared.ps1"),
                "-FullGateEvidence", str(args.full_gate.resolve()),
                "-SharedGateEvidence", str(args.shared_gate.resolve()),
                "-DirectGateEvidence", str(args.direct_gate.resolve()),
                "-C", "5", "-C5Source", str(source), "-C5SourceSha256", source_pin,
                "-SourceBackup", str(source_copy), "-NamespacePath", str(ns),
                "-NamespaceBackup", str(ns_copy), "-ManifestSha256", sha(ns/"manifest.bin"),
                "-OutputPath", str(exported), "-OutputBackup", str(exported_copy),
                "-Chunk", "500", "-Threads", "1", "-MaxMinutes", "1",
                "-AuditMaxSeconds", "30", "-LimitGiB", "2"]

    def invoke(name, cmd, reject=None):
        log = out/(name+".log")
        with log.open("x", encoding="utf-8") as handle:
            result = subprocess.run(cmd, cwd=ROOT, stdout=handle, stderr=subprocess.STDOUT,
                                    timeout=90, check=False)
        text = log.read_text(encoding="utf-8")
        if reject is None:
            if result.returncode or "CLOSED SHARED F4 EXPORT VERIFIED" not in text:
                raise ValueError(f"{name} failed: {log}")
        elif result.returncode == 0 or reject.lower() not in text.lower():
            raise ValueError(f"{name} did not reject for {reject!r}: {log}")
        results.append(dict(test=name, exit=result.returncode, expected_rejection=reject))
        print(("REJECT " if reject else "PASS ")+name, flush=True)

    output = out/"new-closed.L4.snap"
    copy = external/"new-closed.L4.snap"
    invoke("complete-export", command(complete, backup, output, copy))
    if native_image(output)[2] != native_image(fresh)[2] or sha(output) != sha(copy):
        raise ValueError("all-key/stabilizer/T comparison or new physical backup differs")
    receipt = json.loads(Path(str(output)+".receipt.json").read_text(encoding="utf-8"))
    if receipt["new_chunks"] or receipt["new_indices"] or receipt["live"] != 17120:
        raise ValueError("export receipt is not a zero-computation complete native layer")
    print(f"PASS fresh_oracle_all_native_values=17120 output_sha256={sha(output)}", flush=True)
    invoke("overwrite-refused", command(complete, backup, output, copy), "refusing to overwrite")

    for label, excluded in (("incomplete", set(sorted(original_chunks)[-1:])),
                            ("internal-gap", {"chunk-00000000000001f4.bin"})):
        # Skip one chunk while constructing a fresh copy; never delete inputs.
        if label == "incomplete":
            excluded = {max(name for name in original_chunks if name.startswith("chunk-"))}
        ns, cp = out/label, external/label
        ns.mkdir(); cp.mkdir()
        for name in original_chunks:
            if name not in excluded:
                shutil.copy2(complete/name, ns/name)
                shutil.copy2(complete/name, cp/name)
        target = out/(label+".snap")
        invoke(label+"-refused", command(ns, cp, target, external/(label+".snap")), "incomplete namespace")
        if target.exists() or Gate.committed(ns).keys() != (original_chunks.keys()-excluded):
            raise ValueError("incomplete export invocation computed or modified chunks")

    # A pre-existing writer makes acquisition of retained FileShare.Read
    # handles fail, before producer launch. This is the real Windows primitive.
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    create = kernel.CreateFileW
    create.argtypes = [wintypes.LPCWSTR,wintypes.DWORD,wintypes.DWORD,ctypes.c_void_p,
                       wintypes.DWORD,wintypes.DWORD,wintypes.HANDLE]
    create.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    chunk = complete/"chunk-0000000000000000.bin"
    handle = create(str(chunk),0x40000000,1,None,3,0,None)
    if handle == wintypes.HANDLE(-1).value:
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        target = out/"existing-writer.snap"
        invoke("existing-writer-refused", command(complete,backup,target,external/target.name),
               "cannot access the file")
        if target.exists():
            raise ValueError("writer-conflicting invocation created export")
    finally:
        kernel.CloseHandle(handle)
    # Conversely, retained read-only handles deny new writers and deletion.
    handle = create(str(chunk),0x80000000,1,None,3,0,None)
    if handle == wintypes.HANDLE(-1).value:
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        for access in (0x40000000,0x10000):
            denied = create(str(chunk),access,7,None,3,0,None)
            if denied != wintypes.HANDLE(-1).value:
                kernel.CloseHandle(denied)
                raise ValueError("retained read handle did not deny modification/deletion")
    finally:
        kernel.CloseHandle(handle)
    print("PASS Windows_retained_read_handles_deny_write_and_delete", flush=True)
    if (Gate.committed(complete) != original_chunks or Gate.committed(backup) != original_chunks or
            sha(source) != source_pin or sha(source_copy) != source_pin or
            any(sha(path) != pin for path,pin in binaries.items())):
        raise ValueError("protected source, namespace or released executable changed")
    with (out/"checks.json").open("x",encoding="utf-8") as handle:
        json.dump(results,handle,indent=2)
    print("CLOSED SHARED EXPORT CONTROLLER C5 GATES PASSED; no C6 input opened", flush=True)


if __name__ == "__main__":
    main()
