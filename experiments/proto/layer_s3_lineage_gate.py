#!/usr/bin/env python3
"""Small disposable C5 gate for S3 controller lineage and CSV publication.

Uses an already verified closed C5 L4 oracle; never builds a binary, opens
production C6 state, or invokes the real C6 controller. All altered fixtures
and committed native outputs live in a fresh data/logs directory.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/"scripts"))
import layer_s3_lineage as lineage
from layer_dp_progress import CK_SEED, ck_hash64

N5 = "1903816047972624930994913280000"
SOURCE_SHA = "E60114E228A328CE6734B1F68E0136017193D7DD20181F3AD97BE3186127D04A"


def exclusive(path, data):
    with path.open("xb") as handle:
        handle.write(data)


def poisoned(source, target):
    raw = bytearray(source.read_bytes()); n, = struct.unpack_from("<Q", raw, 40)
    first, = struct.unpack_from("<Q", raw, 128+24*n)
    struct.pack_into("<Q", raw, 128+24*n, first+1)
    payload = ck_hash64(raw[128:128+24*n], CK_SEED)
    payload = ck_hash64(raw[128+24*n:128+32*n], payload)
    payload = ck_hash64(raw[128+32*n:], payload)
    struct.pack_into("<Q", raw, 112, payload); struct.pack_into("<Q", raw, 120, 0)
    struct.pack_into("<Q", raw, 120, ck_hash64(raw[:128], CK_SEED))
    exclusive(target, raw)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--source", type=Path, required=True)
    args = p.parse_args()
    if lineage.sha(args.source) != SOURCE_SHA:
        raise ValueError("independent complete C5 source SHA mismatch")
    output = Path(tempfile.mkdtemp(prefix="s3-lineage-gate-", dir=ROOT/"data/logs"))
    print("ARTIFACTS "+str(output), flush=True)
    start = time.monotonic(); checks = []
    source = output/"closed source with spaces.L4.snap"; shutil.copyfile(args.source, source)
    engine = ROOT/"build/layer_dp_gate.exe"
    protected = [ROOT/"build/layer_shared_f4.exe", ROOT/"build/layer_reverse_f5.exe", engine]
    before = {str(path): (lineage.sha(path), path.stat().st_mtime_ns) for path in protected}

    def run(label, command, reason=None):
        with (output/(label+".log")).open("x", encoding="utf-8") as log:
            result = subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT,
                                    timeout=min(120, max(1, 330-(time.monotonic()-start))))
        text = (output/(label+".log")).read_text()
        if reason is None:
            if result.returncode:
                raise ValueError(label+" failed: "+text[-3000:])
        elif not result.returncode or reason not in text:
            raise ValueError(label+" did not reject correctly: "+text[-3000:])
        checks.append(dict(test=label, exit_code=result.returncode))
        print(("PASS " if reason is None else "REJECT ")+label, flush=True)
        return text

    logtree = output/"final logs with spaces"; logtree.mkdir()
    bases = {role: output/("stage "+role) for role in ("primary", "replay")}
    dumps = {role: logtree/(role+".csv") for role in bases}
    caps = "200,40000,40000,1000"

    def command(mode, role="primary", *, source_file=source, digest=None, chunk=1000,
                base=None, peer=None, dump=None, extra=()):
        return [sys.executable, str(ROOT/"scripts/layer_s3_lineage.py"), mode, "--c", "5",
                "--source", str(source_file), "--source-sha256", digest or lineage.sha(source_file),
                "--base", str(base or bases[role]), "--peer", str(peer or bases["replay" if role == "primary" else "primary"]),
                "--dump", str(dump or dumps[role]), "--role", role, "--caps", caps,
                "--chunk", str(chunk), "--expected-n", N5, *extra]

    inventory = set(output.rglob("*"))
    run("prepare-only-no-binding", command("validate"))
    assert not Path(str(bases["primary"])+".s3-binding.json").exists()
    run("overlapping-namespaces", command("validate", peer=bases["primary"]), "paths overlap")
    legacy = output/"legacy"; exclusive(Path(str(legacy)+".a"), b"old artifact")
    run("legacy-no-binding", command("bind", base=legacy), "legacy/unbound")
    orphan = output/"orphan"; exclusive(Path(str(orphan)+".a.tmp"), b"uncommitted old artifact")
    run("legacy-temporary", command("bind", base=orphan), "legacy/unbound")

    for role in bases:
        run(role+"-bind", command("bind", role))
    binding = Path(str(bases["primary"])+".s3-binding.json")
    binding_sha = lineage.sha(binding)
    binding_bytes = binding.read_bytes(); binding.write_bytes(b" "*65537)
    run("oversized-binding-record", command("validate"), "oversized")
    binding.write_bytes(binding_bytes)
    changed_binding = json.loads(binding_bytes)
    changed_binding["implementation"]["source_sha256"]["scripts/layer_dp_progress.py"] = "0"*64
    binding.write_bytes(lineage.encoded(changed_binding))
    run("changed-imported-header-semantics", command("validate"), "binding mismatch")
    binding.write_bytes(binding_bytes)
    wrong = output/"hash-valid-different-source.L4.snap"; poisoned(source, wrong)
    run("different-weight-source-with-same-keys", command("validate", source_file=wrong), "binding mismatch")
    run("wrong-declared-source-sha", command("validate", digest="0"*64), "full SHA-256 mismatch")
    run("changed-chunk", command("validate", chunk=500), "binding mismatch")
    assert lineage.sha(binding) == binding_sha

    # Actual native C5 contraction in two distinct namespaces, including paths
    # with spaces. Each parent image is emitted by the unchanged engine itself.
    for role in bases:
        attempt = logtree/(role+" attempt"); attempt.mkdir()
        candidate = attempt/"final.csv"; record = attempt/"command.txt"
        engine_cmd = [str(engine), "5", "--threads", "2", "--caps", caps,
                      "--load-layer", "4", str(source), "--checkpoint", str(bases[role]),
                      "0.0001", "--ckpt-chunk", "1000", "--dump", str(candidate)]
        exclusive(record, ("attempt="+role+"\nlayer5_sha256="+SOURCE_SHA+
            "\nbinding_sha256="+lineage.sha(Path(str(bases[role])+".s3-binding.json"))+
            "\nexe_sha256="+lineage.sha(engine)+"\ncommand="+repr(engine_cmd)+"\n").encode("ascii"))
        text = run(role+"-actual-C5-final", engine_cmd)
        assert N5 in text and "complete classes = 355" in text
        run(role+"-resume-source-full-sha", command("validate", role))
        run(role+"-prepare-result", command("prepare-result", role, extra=(
            "--prepared-csv", str(candidate), "--command-record", str(record))))
        assert candidate.exists() and not dumps[role].exists()
        # Separate process return here simulates interruption after the flushed
        # receipt and before CSV publication. Resume performs only its rename.
        run(role+"-readonly-prepared-validation", command("validate", role))
        assert candidate.exists() and not dumps[role].exists()
        run(role+"-recover-prepared-rename", command("finalize-result", role))
        assert dumps[role].exists() and not candidate.exists()
        run(role+"-reuse-closed-result", command("finalize-result", role))

    assert dumps["primary"].read_bytes() == dumps["replay"].read_bytes()
    csv_hash = lineage.sha(dumps["primary"])
    run("independent-semantic-certificate", [sys.executable, str(ROOT/"experiments/proto/s4_certificate_verify.py"),
        str(dumps["primary"]), "--c", "5", "--classes", "355", "--expect-n", N5, "--quiet"])
    run("independent-python-sum", [sys.executable, str(ROOT/"experiments/proto/s4_exact_sum.py"),
        str(dumps["primary"]), "--classes", "355", "--expect-n", N5])
    run("independent-powershell-sum", ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
        str(ROOT/"experiments/proto/s4_exact_sum.ps1"), str(dumps["primary"]), "-Classes", "355", "-ExpectN", N5])

    # Every mutation below is in this new disposable tree. Preserve and restore
    # exact bytes after each rejection to check that no repair is inferred.
    parent = Path(str(bases["primary"])+".L4.snap")
    parent_bytes = parent.read_bytes()
    parent.unlink()  # native parent may hard-link our source; isolate mutation
    exclusive(parent, wrong.read_bytes())
    run("wrong-resume-parent-weight-only", command("validate"), "resume-parent full SHA")
    parent.write_bytes(parent_bytes)
    result_path = Path(str(bases["primary"])+".s3-result.json")
    receipt_bytes = result_path.read_bytes()
    receipt = json.loads(receipt_bytes); receipt["source_sha256"] = "0"*64
    result_path.write_bytes(lineage.encoded(receipt))
    run("hash-valid-wrong-result-lineage", command("validate"), "different source/stage")
    result_path.write_bytes(receipt_bytes)
    receipt = json.loads(receipt_bytes); receipt["prepared_csv"] = str(output/"outside-log-tree.csv")
    result_path.write_bytes(lineage.encoded(receipt))
    run("receipt-path-escape", command("validate"), "escaped the dedicated")
    result_path.write_bytes(receipt_bytes)
    original_csv = dumps["primary"].read_bytes(); dumps["primary"].write_bytes(original_csv+b"\n")
    run("stale-or-corrupted-completed-csv", command("validate"), "CSV SHA mismatch")
    dumps["primary"].write_bytes(original_csv)
    hidden = output/"saved-result-receipt.json"; result_path.rename(hidden)
    run("csv-without-completion-receipt", command("validate"), "lacks its immutable")
    hidden.rename(result_path)
    final = Path(str(bases["primary"])+".L5.snap"); old_final = final.read_bytes()
    final.unlink(); exclusive(final, old_final[:-1]+bytes([old_final[-1]^1]))
    run("changed-final-snapshot", command("validate"), "snapshot SHA mismatch")
    final.write_bytes(old_final)
    run("restored-exact-tuple", command("validate"))
    assert lineage.sha(source) == SOURCE_SHA
    assert before == {str(path): (lineage.sha(path), path.stat().st_mtime_ns) for path in protected}
    receipt = dict(checks=checks, elapsed_seconds=time.monotonic()-start, C=5, classes=355, N5=N5,
                   primary_replay_csv_sha256=csv_hash, source_sha256=SOURCE_SHA,
                   protected_binaries_unchanged=True, C6_checkpoint_io=False,
                   source=str(source), bases={k: str(v) for k, v in bases.items()}, logs=str(output))
    exclusive(output/"checks.json", json.dumps(receipt, indent=2).encode())
    print(json.dumps(receipt, indent=2)); print("S3 LINEAGE AND PREPARED-RESULT RECOVERY CHECKS PASSED")


if __name__ == "__main__":
    main()
