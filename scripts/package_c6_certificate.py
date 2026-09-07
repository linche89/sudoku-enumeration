"""Package the frozen C6 final CSV and standalone checks; never overwrite.

This is artifact packaging, not a new counting or certificate-verification
algorithm. It does not read or modify production checkpoints.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import zipfile


ROOT = Path(__file__).resolve().parents[1]
CSV_BYTES = 4_766_612
CSV_SHA256 = "84F2720E7BA8296D78604153B934B7291AF93C329C07F5D46538F9D7101127C7"
N6 = "38296278920738107863746324732012492486187417600000"
MEMBERS = {
    "README.md": "docs/releases/c6-certificate-README.md",
    "s4_certificate_verify.py": "experiments/proto/s4_certificate_verify.py",
    "s4_exact_sum.py": "experiments/proto/s4_exact_sum.py",
    "s4_exact_sum.ps1": "experiments/proto/s4_exact_sum.ps1",
}


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--csv", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source = args.csv.resolve(strict=True)
    output = args.output.resolve()
    if output.exists():
        parser.error("output already exists; choose an unused archive path")
    if source.stat().st_size != CSV_BYTES:
        parser.error("source is not the frozen complete C6 CSV (wrong length)")
    csv_data = source.read_bytes()
    if sha256(csv_data) != CSV_SHA256:
        parser.error("source is not the frozen complete C6 CSV (wrong SHA-256)")

    members = {"c6-final.csv": csv_data}
    for name, relative in MEMBERS.items():
        members[name] = (ROOT / relative).read_bytes()
    source_commit = subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True, timeout=10
    ).strip()
    manifest = {
        "format": "sudoku-c6-final-certificate-v1",
        "repository": "https://github.com/linche89/sudoku-enumeration",
        "packaging_head_commit": source_commit,
        "production_source_commit": "4dd86b590a2531993af0d00dbb93cf9a13b4de6c",
        "C": 6,
        "classes": 63199,
        "labelled_mass": "622345892187672576",
        "N6": N6,
        "scope": "Final-table certificate; not reevaluation of every F value.",
        "files": {
            name: {"bytes": len(data), "sha256": sha256(data)}
            for name, data in sorted(members.items())
        },
    }
    members["manifest.json"] = (
        json.dumps(manifest, indent=2, sort_keys=True) + "\n"
    ).encode("utf-8")

    output.parent.mkdir(parents=True, exist_ok=True)
    # Exclusive creation protects an existing or concurrently published file.
    with output.open("xb") as handle:
        with zipfile.ZipFile(handle, "w", compression=zipfile.ZIP_DEFLATED) as archive:
            for name, data in sorted(members.items()):
                info = zipfile.ZipInfo(name, date_time=(2026, 9, 7, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                info.external_attr = 0o100644 << 16
                archive.writestr(info, data, compresslevel=9)
    with zipfile.ZipFile(output) as archive:
        if archive.testzip() is not None:
            raise RuntimeError("archive CRC readback failed")
        if set(archive.namelist()) != set(members):
            raise RuntimeError("archive member inventory differs")
        for name, data in members.items():
            if archive.read(name) != data:
                raise RuntimeError("archive readback differs: " + name)
    print("PACKAGE PASS")
    print("archive =", output)
    print("members =", len(members))
    print("bytes =", output.stat().st_size)
    print("sha256 =", sha256(output.read_bytes()))
    print("CSV unchanged =", CSV_SHA256)


if __name__ == "__main__":
    main()
