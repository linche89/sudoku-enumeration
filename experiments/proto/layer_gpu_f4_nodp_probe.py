"""Tiny exact GPU gates for the independent packed-degree/no-DP F4 core.

Only complete C4(26), bounded C5(4), or one C6 sample are admitted. One computing
launch per invocation; <=700ms common deadline, <=1GiB explicit GPU allocation,
120s/1GiB host guards. No production, full catalogue, or occupancy batch.
"""
import argparse
import ctypes as ct
import hashlib
import json
from pathlib import Path
import random
import re
import subprocess
import sys
import time

from layer_gpu_f4_probe import Cuda, read_inputs
from layer_gpu_f4_occupancy_probe import DEVICE_DEADLINE
from layer_gpu_f4_occupancy_probe_v2 import calibration_pair, require, sha, INPUT_SHA, ORACLE_SHA
from layer_shared_resume_audit import HardBounds

WRAPPER=r'''
struct NoDpGuard {
    unsigned long long start,limit;
    __device__ unsigned operator()() {
        if(deadline_expired())return 6;
        return clock64()-start>limit?3:0;
    }
};
extern "C" __global__ void rooted_f4(const unsigned short*inputs,
    unsigned long long*results,int graphs,int n,
    unsigned long long nodeCap,unsigned long long clockCap) {
    const int id=blockIdx.x*blockDim.x+threadIdx.x;if(id>=graphs)return;
    unsigned long long*output=results+6*id;
    for(int i=0;i<6;++i)output[i]=0;
    const unsigned long long start=clock64();NoDpGuard guard{start,clockCap};
    const nodp::Result value=nodp::solve(inputs+12*id,n,nodeCap,guard);
    output[0]=value.status?0:value.value;output[1]=value.leaves;
    output[2]=value.nodes;output[3]=value.iterations;output[4]=value.status;
    output[5]=clock64()-start;
}
'''


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--C',type=int,choices=(4,5,6),required=True)
    for name in ('input','oracle','output-dir'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--cpu-gate',type=Path)
    parser.add_argument('--limit',type=int,required=True)
    mode=parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--execute-tiny-gate',action='store_true')
    mode.add_argument('--occupancy-one-batch',action='store_true')
    parser.add_argument('--seed',type=int,default=20260905)
    args=parser.parse_args()
    occupancy=args.occupancy_one_batch
    require((args.C == 6 and args.limit == 2688 and args.seed == 20260905) if occupancy else
            (args.limit == {4:26,5:4,6:1}[args.C] and args.cpu_gate is not None),
            'only fixed tiny gates or the one authorized2688 occupancy batch are admitted')
    log_root=Path(__file__).resolve().parents[2]/'data'/'logs'
    require(not args.output_dir.exists() and args.output_dir.resolve().is_relative_to(log_root.resolve()),
            'fresh output directory under data/logs required')
    args.output_dir.mkdir()
    started=time.perf_counter();guard=HardBounds(120,1<<30);cuda=None
    report=dict(status='PREFLIGHT',C=args.C,graphs=args.limit,F4_launches=0,markers=[],calibration=[],
                global_deadline_ms=700,node_cap=2000000,GPU_allocation_cap=1<<30,
                host_time_cap=120,host_RSS_cap=1<<30,original_CPU_oracle_recomputed=False,N6_computed=False,
                occupancy_one_batch=occupancy,command=sys.argv)
    log=(args.output_dir/'events.jsonl').open('x',encoding='utf-8',buffering=1)
    def emit(kind,data):
        log.write(json.dumps(dict(kind=kind,data=data,host_seconds=time.perf_counter()-started))+'\n')
        print(kind,json.dumps(data),flush=True)
    try:
        input_count=1024 if occupancy else args.limit
        inputs=read_inputs(args.input,args.C,input_count)
        require(len(inputs) == input_count,'input has insufficient rows')
        oracle={}
        for line in args.oracle.read_text().splitlines():
            if re.fullmatch(r'\d+,\d+,\d+,\d+',line):
                i,v,l,n=map(int,line.split(','));require(i-1 not in oracle,'duplicate oracle row');oracle[i-1]=(v,l,n)
        require(all(i in oracle for i in range(input_count)),'missing retained oracle rows')
        report.update(input_sha256=sha(args.input.read_bytes()),oracle_sha256=sha(args.oracle.read_bytes()))
        new_cpu={}
        if occupancy:
            require(report['input_sha256'] == INPUT_SHA and report['oracle_sha256'] == ORACLE_SHA,
                    'occupancy workload must use the previously pinned1024 inputs/oracle')
            rng=random.Random(args.seed);draws=[rng.randrange(1024) for _ in range(args.limit)]
            report.update(seed=args.seed,draws_with_replacement=True,independent_input_rows=1024,
                          unique_sample_indices=len(set(draws)),new_CPU_core_evaluations=0)
        else:
            draws=list(range(args.limit))
            report['cpu_gate_sha256']=sha(args.cpu_gate.read_bytes())
            # Only tiny gates evaluate the new algorithm on CPU. Occupancy never does.
            command=[str(args.cpu_gate.resolve()),f'C={args.C}','input='+str(args.input.resolve()),
                     'oracle='+str(args.oracle.resolve()),f'limit={args.limit}','maxseconds=30']
            cpu=subprocess.run(command,capture_output=True,text=True,timeout=31)
            (args.output_dir/'new-cpu-gate.stdout.txt').write_text(cpu.stdout,encoding='utf-8')
            (args.output_dir/'new-cpu-gate.stderr.txt').write_text(cpu.stderr,encoding='utf-8')
            require(cpu.returncode == 0 and cpu.stderr == '' and '[OK]' in cpu.stdout,'new CPU/core exact gate failed')
            for line in cpu.stdout.splitlines():
                if re.fullmatch(r'\d+,\d+,\d+,\d+,\d+,\d+,\d+\.\d+',line):
                    i,v,l,n,steps,old_n,seconds=line.split(',');new_cpu[int(i)-1]=tuple(map(int,(v,l,n,steps)))
            require(len(new_cpu) == args.limit,'new CPU gate did not retain every row')
            report['new_cpu_gate_command']=command
        cuda=Cuda();header=Path(__file__).with_name('layer_gpu_f4_nodp_core.h').read_bytes()
        sm=ct.c_int();cuda.check(cuda.driver.cuDeviceGetAttribute(ct.byref(sm),16,cuda.device))
        require(not occupancy or sm.value == 84,'fixed occupancy batch requires84 SMs')
        report.update(SMs=sm.value,blocks=(args.limit+31)//32,threads_per_block=32)
        source=DEVICE_DEADLINE.encode()+header+WRAPPER.encode()
        stamp=time.perf_counter();cuda.compile(source);report['compile_seconds']=time.perf_counter()-stamp
        report.update(device=cuda.name,attributes=cuda.attributes,function_attributes=cuda.function_attributes,
                      core_sha256=sha(header),compiled_source_sha256=sha(source))
        graph_host=(ct.c_ushort*(12*args.limit))(*(mask for draw in draws for mask in inputs[draw][0]))
        result_host=(ct.c_uint64*(6*args.limit))()
        require(ct.sizeof(graph_host)+ct.sizeof(result_host) <= 1<<30,'explicit GPU byte cap')
        stamp=time.perf_counter();graphs=cuda.alloc(ct.sizeof(graph_host));results=cuda.alloc(ct.sizeof(result_host))
        report['allocation_seconds']=time.perf_counter()-stamp
        report['allocated_bytes']=sum(size for _,size in cuda.allocations)
        require(report['allocated_bytes'] <= 1<<30,'explicit allocations exceed1GiB')
        mark=ct.c_void_p();cuda.check(cuda.driver.cuModuleGetFunction(ct.byref(mark),cuda.module,b'mark_batch'))
        def marker(label):
            items=[ct.c_uint64(700000000),results]
            params=(ct.c_void_p*2)(*(ct.addressof(x) for x in items))
            raw=dict(label=label,host_before_launch_ns=time.perf_counter_ns())
            cuda.check(cuda.driver.cuLaunchKernel(mark,1,1,1,1,1,1,0,None,params,None))
            raw['host_after_launch_ns']=time.perf_counter_ns();cuda.check(cuda.driver.cuCtxSynchronize())
            raw['host_after_sync_ns']=time.perf_counter_ns();value=ct.c_uint64()
            cuda.check(cuda.driver.cuMemcpyDtoH_v2(ct.byref(value),results,8))
            raw['host_after_readback_ns']=time.perf_counter_ns();raw['device_ns']=value.value
            report['markers'].append(raw);emit('MARKER',raw);return raw
        calibration_start=time.perf_counter();marker('warmup_excluded');a=marker('calibration_a')
        for label,delay in (('calibration_b',0.02),('calibration_c',0.04)):
            time.sleep(delay);b=marker(label);check=calibration_pair(a,b)
            report['calibration'].append(check);emit('CALIBRATION',check)
            require(check['passed'],'timer calibration failed; no F4 launch/retry');a=b
        report['calibration_seconds']=time.perf_counter()-calibration_start
        require(time.perf_counter()-started < 110,'host time margin exhausted')
        total_stamp=time.perf_counter()
        cuda.check(cuda.driver.cuMemcpyHtoD_v2(graphs,graph_host,ct.sizeof(graph_host)))
        report['H2D_seconds']=time.perf_counter()-total_stamp
        report['workspace_initialization_seconds']=0
        local_cap=cuda.attributes['clock_khz']*650;report['local_clock_cap']=local_cap
        params=[graphs,results,ct.c_int(args.limit),ct.c_int(args.C*2),ct.c_uint64(2000000),ct.c_uint64(local_cap)]
        pointers=(ct.c_void_p*len(params))(*(ct.addressof(x) for x in params))
        first,last=ct.c_void_p(),ct.c_void_p();cuda.check(cuda.driver.cuEventCreate(ct.byref(first),0));cuda.check(cuda.driver.cuEventCreate(ct.byref(last),0))
        try:
            deadline=marker('batch_deadline');report['batch_start_globaltimer_ns']=deadline['device_ns']
            cuda.check(cuda.driver.cuEventRecord(first,None));require(report['F4_launches'] == 0,'one-shot invariant')
            report['F4_launches']=1
            report['host_before_F4_launch_ns']=time.perf_counter_ns()
            cuda.check(cuda.driver.cuLaunchKernel(cuda.function,(args.limit+31)//32,1,1,32,1,1,0,None,pointers,None))
            report['host_after_F4_launch_ns']=time.perf_counter_ns()
            cuda.check(cuda.driver.cuEventRecord(last,None));cuda.check(cuda.driver.cuEventSynchronize(last))
            elapsed=ct.c_float();cuda.check(cuda.driver.cuEventElapsedTime(ct.byref(elapsed),first,last));report['kernel_ms']=elapsed.value
        finally:
            cuda.driver.cuEventDestroy_v2(first);cuda.driver.cuEventDestroy_v2(last)
        readback_start=time.perf_counter();cuda.check(cuda.driver.cuMemcpyDtoH_v2(result_host,results,ct.sizeof(result_host)))
        report['D2H_seconds']=time.perf_counter()-readback_start
        report['transfer_launch_readback_seconds']=time.perf_counter()-total_stamp
        rows=[];statuses={};values=leaves=nodes=iterations=0;complete=0
        for i in range(args.limit):
            value,leaf,node,steps,status,ticks=result_host[6*i:6*i+6]
            rows.append(dict(row=i,sample_index=draws[i],value=value,leaves=leaf,nodes=node,iterations=steps,status=status,ticks=ticks,
                             retained_oracle=list(oracle[draws[i]]),new_cpu_reference=list(new_cpu[i]) if not occupancy else None))
        with (args.output_dir/'rows.json').open('x',encoding='utf-8') as stream:json.dump(rows,stream,indent=2)
        for i,row in enumerate(rows):
            status=row['status'];statuses[status]=statuses.get(status,0)+1
            if status:
                require(status in (1,3,4,5,6) and row['value'] == 0,'partial/unknown output escaped')
            else:
                observed=(row['value'],row['leaves'],row['nodes'],row['iterations'])
                require((occupancy or observed == new_cpu[i]) and observed[:2] == oracle[draws[i]][:2],
                        'exact new CPU/GPU or retained F4/leaf differential')
                values+=row['value'];leaves+=row['leaves'];nodes+=row['nodes'];iterations+=row['iterations'];complete+=1
        report.update(status=('EXACT_OCCUPANCY_BATCH' if occupancy else 'EXACT_TINY_GATE') if complete == args.limit else 'INCOMPLETE_BOUNDED_BATCH',
                      complete_exact=complete,incomplete_no_value=args.limit-complete,statuses=statuses,
                      accepted_value_sum=values,accepted_leaves=leaves,accepted_nodes=nodes,
                      accepted_iterations=iterations,all_closed=complete == args.limit)
        emit('SUMMARY',{k:v for k,v in report.items() if k not in ('markers','calibration','command','new_cpu_gate_command')})
        require(report['kernel_ms'] <= 1000,'short launch bound exceeded; no repeat')
        require(occupancy or complete == args.limit,'tiny GPU exact gate incomplete; no cap increase/retry')
        require(report['input_sha256'] == sha(args.input.read_bytes()) and report['oracle_sha256'] == sha(args.oracle.read_bytes()),
                'input/oracle changed during bounded probe')
    except Exception as error:
        report.update(error=str(error));emit('FAIL',dict(error=str(error),F4_launches=report['F4_launches']));raise
    finally:
        cleanup_started=time.perf_counter()
        if cuda is not None:cuda.close()
        report.update(total_host_seconds=time.perf_counter()-started,peak_host_rss_bytes=guard.peak,
                      cleanup_seconds=time.perf_counter()-cleanup_started,
                      source_sha256=sha(Path(__file__).read_bytes()),cleanup_called=cuda is not None)
        if occupancy:
            report['complete_batch_hot_graphs_per_second']=args.limit/report['transfer_launch_readback_seconds'] if report.get('all_closed') else None
            report['complete_batch_end_to_end_graphs_per_second']=args.limit/report['total_host_seconds'] if report.get('all_closed') else None
        with (args.output_dir/'summary.json').open('x',encoding='utf-8') as stream:json.dump(report,stream,indent=2)
        emit('CLOSED',dict(status=report['status'],total_host_seconds=report['total_host_seconds'],F4_launches=report['F4_launches']))
        log.close();guard.close()


if __name__ == '__main__':main()
