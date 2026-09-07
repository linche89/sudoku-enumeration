"""Run one explicit command with the existing process-tree time/RSS guard.

All logs and the exit/guard receipt use a fresh directory under data/logs.
No stage is implicitly chained and no failed stage is automatically retried.
"""
import argparse
import json
import os
from pathlib import Path
import sys
import time

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'experiments/proto'))
from layer_support_semantic_audit_gate import Guard

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--seconds',type=int,required=True)
p.add_argument('--gib',type=int,required=True)
p.add_argument('--output',type=Path,required=True)
p.add_argument('--expect',type=int,default=0)
p.add_argument('command',nargs=argparse.REMAINDER)
a=p.parse_args()
if not 1<=a.seconds<=10800 or not 1<=a.gib<=64:
    p.error('explicit positive time<=3h and aggregate memory<=64GiB required')
out=a.output.resolve()
if not out.is_relative_to(ROOT/'data/logs') or out.exists():
    p.error('fresh log directory under repository data/logs required')
command=a.command[1:] if a.command[:1]==['--'] else a.command
if not command: p.error('explicit command required')
out.mkdir(parents=True)
# Start-Process from PowerShell 7 otherwise propagates incompatible module paths.
if os.name=='nt':
    os.environ['PSModulePath']=r'C:\Program Files\WindowsPowerShell\Modules;C:\Windows\System32\WindowsPowerShell\v1.0\Modules'
guard=Guard(out,max_seconds=a.seconds,max_bytes=a.gib<<30)
result=dict(status='INCOMPLETE',command=command,guard_seconds=a.seconds,
            aggregate_rss_limit_bytes=a.gib<<30,expected_exit=a.expect)
print('GUARDED_STEP logs='+str(out),flush=True)
try:
    guard.run(command,'step',expect=a.expect)
    result['status']='PASS'
finally:
    result.update(seconds=time.monotonic()-guard.started,sampled_peak_rss_bytes=guard.peak,runs=guard.runs,
                  surviving_tracked_processes=[x.pid for x in guard.live()])
    with (out/'receipt.json').open('x',encoding='utf-8') as f: json.dump(result,f,indent=2)
    print(json.dumps(result),flush=True)
