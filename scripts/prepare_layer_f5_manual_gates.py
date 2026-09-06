"""Fresh bounded small-case gates for the manual F4-export/F5-pilot launcher.

No C6 production job or export is launched. Each gate has an independent
360-second / 6-GiB aggregate process-tree guard and retains its own log.
"""
import argparse
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'experiments/proto'))
from layer_shared_incremental_candidate_gate import TreeBound, digest, require


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, required=True)
    args = parser.parse_args()
    out = args.output_dir.resolve()
    require(out.is_relative_to(ROOT/'data/logs') and not out.exists(), 'fresh gate directory under data/logs required')
    out.mkdir()
    protected = [ROOT/'build'/name for name in
                 ('layer_shared_f4.exe', 'layer_reverse_f5.exe', 'layer_support_probe.exe')]
    pins = {str(p): digest(p) for p in protected}
    report = dict(status='INCOMPLETE', production_computation=False, before_sha256=pins, gates={})
    tasks = (
        ('full', 'verify_all.ps1', [], 'ALL REPOSITORY CHECKS PASSED'),
        ('direct', 'verify_layer_direct.ps1', ['-SkipBuild', '-Threads', '4', '-SecondsPerProcess', '120'],
         'DIRECT LAYER CHECKS PASSED'),
        ('shared', 'verify_layer_shared.ps1', ['-SkipBuild', '-Threads', '4', '-SecondsPerProcess', '120'],
         'SHARED LAYER CHECKS PASSED'),
        ('reverse', 'verify_layer_reverse.ps1', ['-SkipBuild', '-Threads', '4', '-SecondsPerProcess', '120'],
         'PRODUCTION REVERSE F5 CHECKS PASSED'),
    )
    try:
        for name, script, extra, marker in tasks:
            print('CHECKING '+name+'; log='+str(out/(name+'.log')), flush=True)
            guard = TreeBound()
            log = out/(name+'.log')
            text = guard.run(['powershell', '-NoProfile', '-ExecutionPolicy', 'Bypass',
                              '-File', str(ROOT/'scripts'/script), *extra], log)
            require(marker in text, 'gate marker missing: '+name)
            require({str(p): digest(p) for p in protected} == pins, 'protected production executable changed')
            report['gates'][name] = dict(path=str(log), sha256=digest(log), marker=marker,
                guard_seconds=360, rss_limit_bytes=6 << 30, runs=guard.runs)
        report.update(status='PASS', after_sha256={str(p): digest(p) for p in protected})
    finally:
        with (out/'receipt.json').open('x', encoding='utf-8') as stream:
            json.dump(report, stream, indent=2)
    print('MANUAL F5 GATES PASSED; no production computation launched', flush=True)


if __name__ == '__main__':
    main()
