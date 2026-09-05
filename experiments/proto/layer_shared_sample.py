"""Build a bounded, exactly fiber-closed C6 native F4 test catalogue.

Every emitted expected value is a freshly closed retained-kernel result. No
checkpoint weights are read. This is a finite implementation test, not N(6).
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import statistics
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
NATIVE_L4_SIZE = 903_398_603  # Independently closed orbit census, not sample size.


def execute(command: list[str], log: Path, seconds: float = 185) -> str:
    started = time.monotonic()
    with log.open("x", encoding="utf-8") as output:
        result = subprocess.run(command, cwd=ROOT, stdout=output,
                                stderr=subprocess.STDOUT, timeout=seconds, check=False)
    text = log.read_text(encoding="utf-8")
    if result.returncode or "[OK]" not in text:
        raise RuntimeError(f"bounded child failed ({result.returncode}): {log}")
    print(f"RUN {log.name} wall={time.monotonic()-started:.6f}s", flush=True)
    return text


def decode_native(key: tuple[int, ...]) -> tuple[int, ...]:
    if len(key) != 12:
        raise ValueError("C6 native key needs 12 masks")
    graph = []
    for word in key:
        if word >> 12:
            raise ValueError("native mask exceeds six boxes")
        mask = 0
        for box in range(6):
            field = (word >> (2*box)) & 3
            if field == 1:
                raise ValueError("invalid native field 1")
            if field:
                mask |= 1 << (2*box + field - 2)
        if mask.bit_count() != 4:
            raise ValueError("non-four-row native key")
        graph.append(mask)
    if any(sum((mask >> slot) & 1 for mask in graph) != 4 for slot in range(12)):
        raise ValueError("unbalanced slot degrees")
    return tuple(graph)


def write_graphs(path: Path, keys: list[tuple[int, ...]], values: list[int] | None = None) -> None:
    with path.open("x", encoding="ascii", newline="\n") as output:
        output.write("# C=6 L=4 bounded exact native fiber closure; NOT a complete C6 layer\n")
        for index, key in enumerate(keys):
            output.write(" ".join(map(str, decode_native(key))))
            if values is not None:
                output.write(f" expected={values[index]}")
            output.write("\n")


def parse_summary(text: str) -> dict[str, str]:
    line, = [row for row in text.splitlines() if row.startswith("summary ")]
    return dict(token.split("=", 1) for token in line.split()[1:])


def mean_se(values: list[float]) -> tuple[float, float]:
    return statistics.mean(values), statistics.stdev(values)/math.sqrt(len(values)) if len(values) > 1 else 0.0


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--limit", type=int, default=1024)
    args = parser.parse_args()
    if not 1 <= args.limit <= 1024:
        parser.error("bounded source limit must be in 1..1024")
    source = args.source.resolve()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    source_sha = hashlib.sha256(source.read_bytes()).hexdigest().upper()
    fiber_log = execute([
        str(ROOT / "build/layer_pairing_fiber_bench.exe"), "6", "4", f"input={source}",
        f"limit={args.limit}", "maxpairings=5000000", "maxseconds=180", "emitfibers",
    ], out / "fiber-expansion.log")
    samples = [obj for line in fiber_log.splitlines() if line.startswith("{")
               and "sample" in (obj := json.loads(line))]
    if len(samples) != args.limit:
        raise ValueError("fiber expansion did not cover every bounded source")
    groups: dict[tuple[int, ...], tuple[tuple[int, ...], ...]] = {}
    closure: set[tuple[int, ...]] = set()
    source_representatives = []
    for sample in samples:
        representative = tuple(sample["native_key"])
        fiber = tuple(tuple(key) for key in sample["fiber"])
        if not fiber or len(set(fiber)) != len(fiber) or tuple(sorted(fiber)) != fiber:
            raise ValueError("fiber keys are not unique and sorted")
        if representative != fiber[0] or len(fiber) != sample["native_fiber_with_transpose"]:
            raise ValueError("fiber minimum/size mismatch")
        for key in fiber:
            decode_native(key)
        if representative in groups and groups[representative] != fiber:
            raise ValueError("one graph key has inconsistent exact fiber")
        groups[representative] = fiber
        closure.update(fiber)
        source_representatives.append(representative)
    keys = sorted(closure)
    if len(keys) > 100_000:
        raise ValueError("bounded expanded catalogue exceeds 100000 states")
    if sum(map(len, groups.values())) != len(keys):
        raise ValueError("distinct complete graph fibers overlap")
    key_index = {key: index for index, key in enumerate(keys)}
    values: list[int] = []
    batch_summaries = []
    for begin in range(0, len(keys), 20_000):
        batch = keys[begin:begin+20_000]
        raw = out / f"direct-batch-{begin:06d}.txt"
        write_graphs(raw, batch)
        text = execute([
            str(ROOT / "build/layer_direct_bench.exe"), "6", "4", f"input={raw}",
            f"limit={len(batch)}", "mode=direct", "threads=24", "maxseconds=180",
            "maxrecords=1000000000", "maxstates=1000000",
        ], out / f"direct-batch-{begin:06d}.log")
        rows = [tuple(map(int, line.split(','))) for line in text.splitlines()
                if re.fullmatch(r"\d+,\d+,\d+,\d+", line)]
        summary = parse_summary(text)
        if len(rows) != len(batch) or int(summary["rows"]) != len(batch):
            raise ValueError("direct F4 batch did not close every native key")
        for index, value, leaves, nodes in rows:
            if index != len(values)-begin+1 or not 0 < value < 2**64 or not 0 < leaves <= nodes:
                raise ValueError("invalid closed F4 record")
            values.append(value)
        batch_summaries.append(summary)
    for representative, fiber in groups.items():
        expected = values[key_index[representative]]
        if any(values[key_index[key]] != expected for key in fiber):
            raise ValueError("cold F4 values differ inside an exact graph fiber")
    expected_path = out / "closure-expected.txt"
    write_graphs(expected_path, keys, values)
    representative_values = [values[key_index[key]] for key in source_representatives]
    timing_input = out / "uniform-source-minimum-representatives.txt"
    write_graphs(timing_input, source_representatives, representative_values)
    timing = execute([
        str(ROOT / "build/layer_direct_bench.exe"), "6", "4", f"input={timing_input}",
        f"limit={len(samples)}", "mode=direct", "maxseconds=120",
        "maxrecords=100000000", "maxstates=1000000",
    ], out / "uniform-source-minimum-representatives-timing.log", 125)
    timing_rows = [line.split(',') for line in timing.splitlines()
                   if re.match(r"^\d+,\d+,", line)]
    timing_summary = parse_summary(timing)
    if len(timing_rows) != len(samples) or int(timing_summary["expected_checked"]) != len(samples):
        raise ValueError("representative timing did not reproduce every exact value")
    inverse_fibers, weighted_times, representative_times = [], [], []
    per_source = []
    for sample, key, columns, expected in zip(samples, source_representatives, timing_rows, representative_values):
        if int(columns[0]) != sample["sample"] or int(columns[1]) != expected:
            raise ValueError("representative timing source/value mismatch")
        # Sequential direct mode: column 8 is closed F4 kernel elapsed time;
        # source canonicalization and leaf pre-count are separate columns.
        seconds = float(columns[8])
        size = sample["native_fiber_with_transpose"]
        inverse_fibers.append(1/size)
        representative_times.append(seconds)
        weighted_times.append(seconds/size)
        per_source.append(dict(source=sample["sample"], fiber_size=size,
                               representative_native_key=key,
                               representative_catalogue_id=key_index[key],
                               representative_F4=expected, kernel_seconds=seconds,
                               kernel_seconds_divided_by_fiber=seconds/size))
    mean_inverse, se_inverse = mean_se(inverse_fibers)
    mean_work, se_work = mean_se(weighted_times)
    report = dict(
        source=str(source), source_sha256=source_sha, uniform_native_sources=len(samples),
        fiber_closed_native_states=len(keys), graph_representatives=len(groups),
        every_direct_F4_within_fiber_equal=True, input_checkpoint_weights_used=False,
        expected_file=str(expected_path),
        expected_sha256=hashlib.sha256(expected_path.read_bytes()).hexdigest().upper(),
        direct_batches=batch_summaries, mean_inverse_fiber=mean_inverse,
        se_inverse_fiber=se_inverse, mean_representative_kernel_seconds=statistics.mean(representative_times),
        mean_kernel_seconds_divided_by_fiber=mean_work, se_kernel_seconds_divided_by_fiber=se_work,
        native_L4_population=NATIVE_L4_SIZE,
        projected_single_thread_kernel_hours=NATIVE_L4_SIZE*mean_work/3600,
        projection_scope="uniform-native sample assumption; measured sequential kernel time; not multicore production wall time",
        per_source=per_source,
    )
    with (out / "summary.json").open("x", encoding="utf-8") as output:
        json.dump(report, output, indent=2)
    print(json.dumps({key: value for key, value in report.items()
                      if key not in ("per_source", "direct_batches")}, indent=2), flush=True)
    print("[OK] exact bounded fiber closure and per-native cold F4 fixture", flush=True)


if __name__ == "__main__":
    main()
