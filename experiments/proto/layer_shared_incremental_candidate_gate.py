"""Candidate-only shared-F4 integration; released files never rebuilt or written.

One 360s / 6GiB sampled aggregate process-tree bound includes compilation,
the existing actual-engine gate, and old-version chunk resume compatibility.
Timings are concurrent with production and are NOT speedup measurements.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time
import uuid

import psutil
from layer_shared_gate import native_image, summary

ROOT = Path(__file__).resolve().parents[2]


def require(ok, message):
    if not ok:
        raise ValueError(message)


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest().upper()


class TreeBound:
    def __init__(self):
        self.started = time.monotonic()
        self.peak = 0
        self.parent = psutil.Process()
        self.runs = []

    def run(self, command, log):
        require(time.monotonic()-self.started < 360, 'aggregate 360s bound exhausted')
        started = time.monotonic()
        tracked = {}
        process = None
        failure = None
        peak = 0
        try:
            with log.open('x', encoding='utf-8') as stream:
                process = subprocess.Popen(command, cwd=ROOT, stdout=stream,
                    stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW)
                root = psutil.Process(process.pid)
                tracked[root.pid] = root.create_time()
                while True:
                    try:
                        for child in root.children(recursive=True):
                            tracked[child.pid] = child.create_time()
                    except psutil.NoSuchProcess:
                        pass
                    rss = self.parent.memory_info().rss
                    for pid, created in list(tracked.items()):
                        try:
                            p = psutil.Process(pid)
                            if p.create_time() == created:
                                rss += p.memory_info().rss
                        except psutil.NoSuchProcess:
                            pass
                    peak = max(peak, rss)
                    self.peak = max(self.peak, rss)
                    require(rss <= 6 << 30, 'aggregate subtree RSS exceeds 6GiB')
                    require(time.monotonic()-self.started <= 360, 'aggregate 360s deadline')
                    if process.poll() is not None:
                        break
                    time.sleep(0.075)
                require(process.returncode == 0, 'child failed; see '+str(log))
        except BaseException as error:
            failure = repr(error)
            raise
        finally:
            survivors = []
            for pid, created in list(tracked.items()):
                try:
                    p = psutil.Process(pid)
                    if p.create_time() == created and p.is_running():
                        survivors.append(pid)
                        p.kill()
                except psutil.NoSuchProcess:
                    pass
            if process is not None:
                process.wait(timeout=5)
            self.runs.append(dict(command=command, log=str(log),
                seconds=time.monotonic()-started, sampled_peak_rss_bytes=peak,
                tracked_process_count=len(tracked), survivors_killed=survivors,
                error=failure, exit=None if process is None else process.returncode))
            if failure is None:
                require(not survivors, 'unexpected surviving descendant after successful child')
        print('PASS '+log.stem, flush=True)
        return log.read_text(encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--old-fixtures', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    args = parser.parse_args()
    out = args.output_dir.resolve()
    require(out.is_relative_to(ROOT/'data/logs') and not out.exists(),
            'fresh dedicated data/logs output directory required')
    old = args.old_fixtures.resolve()
    require(old.is_relative_to(ROOT/'data/logs') and old.is_dir(),
            'old compatibility fixture must be an explicit small data/logs directory')
    old_ns = old/'closed-c5'
    old_source = old/'reference-c5.L4.snap'
    old_verify = old/'reference-c5.L4.txt'
    old_export = old/'shared-c5.L4.snap'
    old_files = sorted(old_ns.glob('*.bin'))+[old_source, old_verify, old_export]
    require(len(old_files) == 39 and sum(p.stat().st_size for p in old_files) < 16 << 20,
            'old-version fixture inventory exceeded the fixed small C5 domain')
    sample = ROOT/'data/logs/c6-direct-route-20260905/shared-sample-1024/closure-expected.txt'
    require(sample.is_file() and sample.stat().st_size < 4 << 20,
            'missing bounded complete-fiber C6 text oracle')
    out.mkdir()
    binary_dir = ROOT/'build'/('shared-incremental-candidate-'+uuid.uuid4().hex)
    binary_dir.mkdir()
    candidate = binary_dir/'layer_shared_f4_incremental.exe'
    protected = [ROOT/p for p in (
        'build/layer_shared_f4.exe', 'experiments/proto/layer_shared_f4.cpp',
        'experiments/proto/layer_shared_f4_bridge.cpp', 'experiments/proto/layer_shared_f4_bridge.h',
        'experiments/proto/layer_shared_f4_core.h', 'experiments/proto/layer_shared_chunks.h',
        'experiments/proto/layer_shared_catalog.h', 'experiments/proto/layer_dp_gate.cpp',
        'experiments/proto/layer_pairing_fiber_bench.cpp', 'src/factorization_orbit.cpp',
        'experiments/proto/layer_cpu_f4_incremental_core.h',
        'experiments/proto/layer_gpu_f4_nodp_core.h',
        'experiments/proto/layer_shared_f4_incremental_bridge.cpp',
        'experiments/proto/layer_shared_gate.py',
        'experiments/proto/layer_shared_incremental_candidate_gate.py')]
    guard = TreeBound()
    pins = {str(p): digest(p) for p in protected+old_files+[sample]}
    result = dict(status='INCOMPLETE', production_checkpoint_io=False,
                  production_publication=False, concurrent_with_production=True,
                  performance_evidence=False, guard_seconds=360,
                  aggregate_rss_limit_bytes=6 << 30, before_sha256=pins,
                  candidate_binary=str(candidate))
    try:
        guard.run(['g++', '-O3', '-mpopcnt', '-std=c++20', '-fopenmp',
                   'experiments/proto/layer_shared_f4.cpp',
                   'experiments/proto/layer_shared_f4_incremental_bridge.cpp',
                   '-o', str(candidate), '-lbcrypt', '-lpsapi'], out/'compile.log')
        candidate_sha = digest(candidate)
        text = guard.run([sys.executable, 'experiments/proto/layer_shared_gate.py',
            '--shared-exe', str(candidate), '--threads', '4', '--seconds-per-process', '120',
            '--output', str(out/'fixtures'), '--c6-sample', str(sample)], out/'actual-shared-gate.log')
        require('SHARED F4 EXACTNESS AND RECOVERY GATES PASSED' in text,
                'missing actual shared gate marker')
        copied = out/'old-version-closed-c5-copy'
        shutil.copytree(old_ns, copied)
        before_copy = {p.name: digest(p) for p in copied.glob('*.bin')}
        exported = out/'old-version-resumed-c5.L4.snap'
        resumed = guard.run([str(candidate), '5', 'catalog='+str(old_source),
            'output='+str(copied), 'chunk=500', 'limit=500', 'threads=4',
            'workseconds=90', 'maxseconds=110', 'maxrssgib=2', 'checkpointreadonly',
            'verify='+str(old_verify), 'export='+str(exported)], out/'old-version-resume.log')
        state = summary(resumed, 17120)
        require(state.get('new_chunks') == '0' and state.get('new_indices') == '0',
                'old-version resume unexpectedly performed new counting')
        require('exact_verified_native_values=17120' in resumed,
                'old-version resume did not compare every exact value')
        require(before_copy == {p.name: digest(p) for p in copied.glob('*.bin')},
                'old-version immutable copy changed')
        a, _, av = native_image(old_export)
        b, _, bv = native_image(exported)
        require(av == bv and a.holes == b.holes and len(av) == 17120,
                'old-version independently read back exports differ')
        after = {path: digest(path) for path in pins}
        require(after == pins, 'a protected released/source/fixture file changed')
        require(digest(candidate) == candidate_sha, 'candidate binary changed during qualification')
        result.update(status='CANDIDATE_ONLY_PASS', after_sha256=after,
            candidate_sha256=candidate_sha,
            actual_gate_checks_sha256=digest(out/'fixtures/checks.json'),
            old_version_resume=dict(new_chunks=0, new_indices=0, values_checked=17120,
                immutable_chunks_unchanged=True, exports_equal=True,
                original_export_sha256=digest(old_export), new_export_sha256=digest(exported)))
    finally:
        result.update(elapsed_seconds=time.monotonic()-guard.started,
                      sampled_peak_subtree_rss_bytes=guard.peak, commands=guard.runs)
        with (out/'receipt.json').open('x', encoding='utf-8') as stream:
            json.dump(result, stream, indent=2)
    print('INCREMENTAL SHARED F4 CANDIDATE ONLY GATES PASSED', flush=True)


if __name__ == '__main__':
    main()
