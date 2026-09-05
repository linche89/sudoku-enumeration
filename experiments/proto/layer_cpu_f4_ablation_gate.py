"""Retained bounded three-kernel CPU suite:120s / aggregate<2GiB, no GPU."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import statistics
import subprocess
import time
from layer_shared_resume_audit import HardBounds, require


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--worker',type=Path,required=True)
    parser.add_argument('--output-dir',type=Path,required=True)
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[2];logs=root/'data'/'logs'
    require(not args.output_dir.exists() and args.output_dir.resolve().is_relative_to(logs.resolve()),'fresh data/logs output directory required')
    args.output_dir.mkdir()
    guard=HardBounds(120,128<<20)
    report=dict(status='INCOMPLETE',CPU_only=True,checkpoint_reads=0,checkpoint_writes=0,GPU_launches=0,
                source_sha256={},runs=[])
    source_paths=['src/factorization_orbit.cpp','experiments/proto/layer_shared_f4_bridge.cpp',
                  'experiments/proto/layer_gpu_f4_nodp_core.h','experiments/proto/layer_cpu_f4_incremental_core.h',
                  'experiments/proto/layer_cpu_f4_ablation.cpp']
    for path in source_paths:report['source_sha256'][path]=hashlib.sha256((root/path).read_bytes()).hexdigest().upper()
    report['worker_sha256']=hashlib.sha256(args.worker.read_bytes()).hexdigest().upper()
    try:
        fixtures=logs/'layer-direct-gate-20260905-fixtures-v2';route=logs/'c6-direct-route-20260905'
        for c,count in ((4,26),(5,17120),(6,1024)):
            left=int(120-(time.monotonic()-guard.started))-1
            require(left>0,'suite time exhausted before next domain')
            input_path=route/'l4-sample.txt' if c==6 else fixtures/f'c{c}.L4.txt'
            oracle_path=route/('l4-parallel24-1024.log' if c==6 else f'c{c}-l4-parallel{8 if c==4 else 24}-gate.log')
            command=[str(args.worker.resolve()),f'C={c}','input='+str(input_path),'oracle='+str(oracle_path),
                     f'limit={count}','rounds=3',f'repeats={32 if c==4 else 1}',f'maxseconds={left}','maxrssmib=1900']
            stdout=args.output_dir/f'c{c}.stdout.log';stderr=args.output_dir/f'c{c}.stderr.log'
            with stdout.open('xb') as out,stderr.open('xb') as err:
                result=subprocess.run(command,stdout=out,stderr=err,timeout=left+0.5,check=False,cwd=root)
            require(result.returncode==0,'native CPU ablation failed: '+str(stderr))
            require(stderr.read_bytes()==b'','native stderr is not empty')
            text=stdout.read_text();require(text.endswith('CPU THREE KERNEL EXACT ABLATION PASSED\n'),'missing terminal exact marker')
            phases=[];summaries=[];data_rows=0
            for line in text.splitlines():
                if re.match(r'^\d+,',line):data_rows+=1
                if line.startswith(('TIMING ','SUMMARY ')):
                    fields=dict(re.findall(r'([A-Za-z0-9_]+)=([^\s]+)',line))
                    (phases if line.startswith('TIMING ') else summaries).append(fields)
            require(data_rows==count and len(phases)==18 and len(summaries)==1,'incomplete per-graph/timing inventory')
            median={}
            for threads in ('1','24'):
                median[threads]={}
                for variant in ('RELEASED_TERNARY_DP','FROZEN_LEAF_DSU_NODP','INCREMENTAL_DSU_NODP'):
                    rows=[r for r in phases if r['threads']==threads and r['variant']==variant]
                    require(len(rows)==3 and {int(r['position']) for r in rows}=={0,1,2},'unbalanced cyclic positions')
                    median[threads][variant]=statistics.median(float(r['seconds']) for r in rows)
            run=dict(C=c,rows=count,command=command,summary=summaries[0],phases=phases,median_seconds=median,
                     stdout_sha256=hashlib.sha256(stdout.read_bytes()).hexdigest().upper())
            report['runs'].append(run)
            print(json.dumps(dict(C=c,exact='PASS',median_seconds=median,total_seconds=summaries[0]['total_seconds'])),flush=True)
        require(report['worker_sha256']==hashlib.sha256(args.worker.read_bytes()).hexdigest().upper(),'worker changed during suite')
        for path,pin in report['source_sha256'].items():require(hashlib.sha256((root/path).read_bytes()).hexdigest().upper()==pin,'included core changed during suite')
        report['status']='PASS'
    finally:
        report.update(elapsed_seconds=time.monotonic()-guard.started,parent_peak_rss_bytes=guard.peak)
        with (args.output_dir/'summary.json').open('x',encoding='utf-8') as stream:json.dump(report,stream,indent=2)
        guard.close()
    print('CPU THREE KERNEL COMPLETE BOUNDED SUITE PASSED',flush=True)


if __name__=='__main__':main()
