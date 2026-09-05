"""One bounded occupancy batch; synchronized marker calibration, raw evidence.

The old occupancy/device source is imported unchanged and SHA-pinned. Only
timestamp markers warm up before calibration; exactly one F4 launch is possible.
Every nonclosed value must be zero. No CPU oracle calculation or catalogue I/O.
"""
from __future__ import annotations
import argparse
import ctypes as ct
import hashlib
import json
from pathlib import Path
import random
import re
import sys
import time

from layer_gpu_f4_occupancy_probe import device_source
from layer_gpu_f4_probe import Cuda, read_inputs
from layer_shared_resume_audit import HardBounds

PINNED = {
    'layer_gpu_f4_occupancy_probe.py':'0347C4863AF68786EBF5245873AEF7F655A1AA46C89777E54C095254184CBA73',
    'layer_gpu_f4_probe.py':'775908D26757D1A77C27013742AC8B09FD542C3CC543C1C0F37C305585B73520',
    'layer_gpu_f4_probe.cu':'0EDB6BCF9F8C50D52648A7F0366B28AC4E6C7EF4D507DC669CC3321D8EB08BF9',
}
INPUT_SHA = '38F4B8EF65FA4BEEF7FB05995E2A64B7096162A5E1212A0388AF5A9E195CA99C'
ORACLE_SHA = '8B3DD7335033F834631AB10C930807E4ADEB0C3CE4B8571145737A6EEFE80F1A'


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(raw):
    return hashlib.sha256(raw).hexdigest().upper()


def calibration_pair(first, last):
    """Each device timestamp lies between host launch-before and sync-after.

    Therefore a true nanosecond device difference lies between the two crossed
    host endpoint differences, up to the declared100us timer/read uncertainty.
    Reject imprecise brackets instead of widening tolerance or retrying.
    """
    device_ns = last['device_ns']-first['device_ns']
    lower_ns = last['host_before_launch_ns']-first['host_after_sync_ns']
    upper_ns = last['host_after_sync_ns']-first['host_before_launch_ns']
    midpoint_ns = (lower_ns+upper_ns)/2
    width_ns = upper_ns-lower_ns
    tolerance_ns = 100000
    criteria = dict(
        monotone_device=device_ns > 0,
        short_positive_host_interval=10000000 <= lower_ns <= upper_ns <= 250000000,
        narrow_host_brackets=0 <= width_ns <= min(2000000,midpoint_ns/10),
        inside_host_bracket=lower_ns-tolerance_ns <= device_ns <= upper_ns+tolerance_ns,
        nanosecond_scale=midpoint_ns > 0 and 0.95 <= device_ns/midpoint_ns <= 1.05,
    )
    return dict(first=first['label'],last=last['label'],device_delta_ns=device_ns,
                host_lower_ns=lower_ns,host_upper_ns=upper_ns,host_midpoint_ns=midpoint_ns,
                host_uncertainty_width_ns=width_ns,tolerance_ns=tolerance_ns,
                device_over_host_midpoint=device_ns/midpoint_ns if midpoint_ns > 0 else None,
                criteria=criteria,passed=all(criteria.values()))


def offline_preflight_tests():
    first=dict(label='a',device_ns=1000000000,host_before_launch_ns=5000000000,
               host_after_sync_ns=5000100000)
    last=dict(label='b',device_ns=1020100000,host_before_launch_ns=5020100000,
              host_after_sync_ns=5020200000)
    tests=[]
    for label,change,expected in (
        ('correct_nanoseconds',{},True),
        ('microsecond_scale',{'device_ns':1000020100},False),
        ('millisecond_scale',{'device_ns':1000000020},False),
        ('nonmonotone_device',{'device_ns':999999999},False),
        ('large_lazy_launch_uncertainty',{'host_after_sync_ns':5080000000},False),
        ('excessive_host_gap',{'host_before_launch_ns':5500000000,'host_after_sync_ns':5500100000},False),
    ):
        candidate=last.copy();candidate.update(change)
        result=calibration_pair(first,candidate)
        require(result['passed'] == expected,'offline calibration predicate differs: '+label)
        tests.append(dict(name=label,expected=expected,observed=result))
    return dict(status='TEST_ONLY_PASS',tests=tests,GPU_launches=0)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path)
    parser.add_argument('--oracle',type=Path)
    parser.add_argument('--output-dir',type=Path,required=True)
    parser.add_argument('--execute-one-batch',action='store_true')
    parser.add_argument('--self-test-preflight',action='store_true')
    parser.add_argument('--seed',type=int,default=20260905)
    parser.add_argument('--global-ms',type=int,default=700)
    args=parser.parse_args()
    require(args.execute_one_batch != args.self_test_preflight,'choose exactly one batch or offline test')
    require(100 <= args.global_ms <= 700,'absolute deadline must be100..700ms')
    require(args.seed == 20260905,'this one-shot qualification fixes seed20260905')
    log_root=Path(__file__).resolve().parents[2]/'data'/'logs'
    require(not args.output_dir.exists() and args.output_dir.resolve().is_relative_to(log_root.resolve()),
            'fresh output directory under data/logs required')
    args.output_dir.mkdir()
    if args.self_test_preflight:
        report=offline_preflight_tests()
        with (args.output_dir/'summary.json').open('x',encoding='utf-8') as stream:
            json.dump(report,stream,indent=2)
        print(json.dumps(report,indent=2))
        return
    require(args.input is not None and args.oracle is not None,'one batch needs pinned input/oracle paths')
    started=time.perf_counter()
    guard=HardBounds(120,1<<30)
    cuda=None
    report=dict(status='PREFLIGHT',F4_launches=0,markers=[],calibration=[],fresh_CPU_benchmark=False,
                N6_computed=False,concurrent_CPU_F4=True,command=sys.argv,
                host_time_cap_seconds=120,host_rss_cap_bytes=1<<30,
                explicit_device_allocation_cap_bytes=4<<30,global_deadline_ms=args.global_ms)
    events=(args.output_dir/'events.jsonl').open('x',encoding='utf-8',buffering=1)

    def emit(kind,value):
        events.write(json.dumps(dict(kind=kind,host_elapsed_seconds=time.perf_counter()-started,data=value))+'\n')
        print(kind,json.dumps(value),flush=True)

    try:
        for name,pin in PINNED.items():
            require(sha(Path(__file__).with_name(name).read_bytes()) == pin,'retained source SHA differs: '+name)
        input_bytes=args.input.read_bytes();oracle_bytes=args.oracle.read_bytes()
        require(sha(input_bytes) == INPUT_SHA and sha(oracle_bytes) == ORACLE_SHA,'pinned input/oracle SHA differs')
        inputs=read_inputs(args.input,6,1024)
        require(len(inputs) == 1024,'requires exactly1024 preverified sample inputs')
        oracle={}
        for line in oracle_bytes.decode().splitlines():
            if re.fullmatch(r'\d+,\d+,\d+,\d+',line):
                row,value,leaves,nodes=map(int,line.split(','))
                require(row-1 not in oracle,'duplicate oracle row')
                oracle[row-1]=(value,leaves,nodes)
        require(sorted(oracle) == list(range(1024)),'oracle must cover exactly1024 rows')
        require(all(expected is None or expected == oracle[i][0] for i,(_,expected) in enumerate(inputs)),
                'input exact expectation disagrees with retained oracle')
        cuda=Cuda()
        sm=ct.c_int()
        cuda.check(cuda.driver.cuDeviceGetAttribute(ct.byref(sm),16,cuda.device))
        require(sm.value == 84,'this fixed authorized batch requires84 SMs')
        count,states,frontier=2688,531441,100000
        workspace=count*(24+48+states+2*frontier*4)
        require(workspace == 3579106944 and workspace <= 4<<30,'explicit allocation geometry differs')
        rng=random.Random(args.seed)
        draws=[rng.randrange(1024) for _ in range(count)]
        graph_host=(ct.c_ushort*(12*count))(*(mask for draw in draws for mask in inputs[draw][0]))
        result_host=(ct.c_uint64*(6*count))()
        base,source=device_source()  # Unchanged old absolute-deadline transformation.
        report.update(device=cuda.name,device_attributes=cuda.attributes,SMs=sm.value,graphs=count,
                      blocks=84,threads_per_block=32,draws_with_replacement=True,seed=args.seed,
                      unique_sample_indices=len(set(draws)),allocated_bytes=workspace,
                      source_sha256=PINNED,input_sha256=INPUT_SHA,oracle_sha256=ORACLE_SHA,
                      base_kernel_sha256=sha(base),compiled_kernel_sha256=sha(source),
                      node_cap=2000000,frontier_cap=frontier,ternary_states_per_graph=states)
        stamp=time.perf_counter();cuda.compile(source)
        report['compile_seconds']=time.perf_counter()-stamp
        report['function_attributes']=cuda.function_attributes
        mark=ct.c_void_p()
        cuda.check(cuda.driver.cuModuleGetFunction(ct.byref(mark),cuda.module,b'mark_batch'))
        stamp=time.perf_counter()
        graphs=cuda.alloc(ct.sizeof(graph_host));results=cuda.alloc(ct.sizeof(result_host))
        marks=cuda.alloc(count*states);fronts=cuda.alloc(count*2*frontier*4)
        report['allocation_seconds']=time.perf_counter()-stamp
        require(sum(size for _,size in cuda.allocations) == workspace,'actual explicit allocations differ')
        emit('DEVICE', {k:report[k] for k in ('device','device_attributes','SMs','graphs','blocks',
             'threads_per_block','allocated_bytes','compile_seconds','allocation_seconds','function_attributes')})

        def synchronized_marker(label):
            items=[ct.c_uint64(args.global_ms*1000000),results]
            pointers=(ct.c_void_p*2)(*(ct.addressof(x) for x in items))
            raw=dict(label=label,host_before_launch_ns=time.perf_counter_ns())
            cuda.check(cuda.driver.cuLaunchKernel(mark,1,1,1,1,1,1,0,None,pointers,None))
            raw['host_after_launch_ns']=time.perf_counter_ns()
            cuda.check(cuda.driver.cuCtxSynchronize())
            raw['host_after_sync_ns']=time.perf_counter_ns()
            timer=ct.c_uint64()
            cuda.check(cuda.driver.cuMemcpyDtoH_v2(ct.byref(timer),results,8))
            raw['host_after_readback_ns']=time.perf_counter_ns();raw['device_ns']=timer.value
            report['markers'].append(raw);emit('MARKER',raw)
            return raw

        # Warm-up is logged, but never forms an endpoint in the scale predicate.
        synchronized_marker('warmup_excluded')
        first=synchronized_marker('calibration_a')
        for label,delay in (('calibration_b',0.020),('calibration_c',0.040)):
            sleep_start=time.perf_counter_ns();time.sleep(delay);sleep_stop=time.perf_counter_ns()
            last=synchronized_marker(label)
            comparison=calibration_pair(first,last)
            comparison.update(requested_host_sleep_seconds=delay,host_sleep_start_ns=sleep_start,
                              host_sleep_stop_ns=sleep_stop,actual_host_sleep_ns=sleep_stop-sleep_start)
            report['calibration'].append(comparison);emit('CALIBRATION',comparison)
            require(comparison['passed'],'globaltimer calibration failed; no F4 launch, no retry')
            first=last
        report['calibration_passed']=True
        emit('PREFLIGHT',dict(calibration_passed=True,F4_launches=0,
                            global_deadline_ms=args.global_ms,unique_sample_indices=len(set(draws))))
        require(time.perf_counter()-started < 110,'host120s bound: insufficient margin before launch')
        total_stamp=time.perf_counter()
        cuda.check(cuda.driver.cuMemcpyHtoD_v2(graphs,graph_host,ct.sizeof(graph_host)))
        cuda.check(cuda.driver.cuMemsetD8_v2(marks,0,count*states))
        cuda.check(cuda.driver.cuCtxSynchronize())
        report['H2D_workspace_zero_seconds']=time.perf_counter()-total_stamp
        local_clock_cap=cuda.attributes['clock_khz']*650
        report['local_clock_cap_ticks']=local_clock_cap
        items=[graphs,results,marks,fronts,ct.c_int(count),ct.c_int(12),ct.c_int(states),ct.c_int(frontier),
               ct.c_uint64(2000000),ct.c_uint64(local_clock_cap)]
        pointers=(ct.c_void_p*len(items))(*(ct.addressof(x) for x in items))
        first_event,last_event=ct.c_void_p(),ct.c_void_p()
        cuda.check(cuda.driver.cuEventCreate(ct.byref(first_event),0))
        cuda.check(cuda.driver.cuEventCreate(ct.byref(last_event),0))
        try:
            deadline=synchronized_marker('batch_deadline')
            report['batch_start_globaltimer_ns']=deadline['device_ns']
            report['batch_absolute_deadline_globaltimer_ns']=deadline['device_ns']+args.global_ms*1000000
            cuda.check(cuda.driver.cuEventRecord(first_event,None))
            require(report['F4_launches'] == 0,'one-shot F4 launch invariant failed')
            report['F4_launches']=1  # Set before API call so exceptions cannot trigger retry.
            report['host_before_F4_launch_ns']=time.perf_counter_ns()
            cuda.check(cuda.driver.cuLaunchKernel(cuda.function,84,1,1,32,1,1,0,None,pointers,None))
            report['host_after_F4_launch_ns']=time.perf_counter_ns()
            cuda.check(cuda.driver.cuEventRecord(last_event,None))
            cuda.check(cuda.driver.cuEventSynchronize(last_event))
            report['host_after_F4_event_sync_ns']=time.perf_counter_ns()
            elapsed=ct.c_float()
            cuda.check(cuda.driver.cuEventElapsedTime(ct.byref(elapsed),first_event,last_event))
            report['kernel_ms']=elapsed.value
        finally:
            cuda.driver.cuEventDestroy_v2(first_event);cuda.driver.cuEventDestroy_v2(last_event)
        cuda.check(cuda.driver.cuMemcpyDtoH_v2(result_host,results,ct.sizeof(result_host)))
        report['transfer_init_launch_readback_seconds']=time.perf_counter()-total_stamp
        # Raw results are retained before validation so even a refusal/mismatch
        # has complete per-draw evidence. Never sum a partial graph value.
        statuses={};complete=0;sum_values=sum_leaves=sum_nodes=0
        rows=[]
        for i,draw in enumerate(draws):
            value,leaves,nodes,dp,status,ticks=result_host[6*i:6*i+6]
            rows.append(dict(draw=i,sample_index=draw,value=value,leaves=leaves,nodes=nodes,
                             dp_steps=dp,status=status,ticks=ticks,oracle=list(oracle[draw])))
        with (args.output_dir/'draw-results.json').open('x',encoding='utf-8') as stream:
            json.dump(rows,stream,indent=2)
        for row in rows:
            status=row['status'];statuses[status]=statuses.get(status,0)+1
            if status == 0:
                require((row['value'],row['leaves'],row['nodes']) == tuple(row['oracle']),
                        f"CPU/GPU differential draw{row['draw']}/sample{row['sample_index']}")
                complete+=1;sum_values+=row['value'];sum_leaves+=row['leaves'];sum_nodes+=row['nodes']
            else:
                require(status in (1,2,3,4,5,6) and row['value'] == 0,'unknown status or partial value escaped guard')
        report.update(status='EXACT_BOUNDED_BATCH',complete_exact=complete,incomplete_no_value=count-complete,
                      statuses=statuses,all_closed=complete == count,accepted_value_sum=sum_values,
                      accepted_leaves=sum_leaves,accepted_nodes=sum_nodes,
                      every_closed_value_leaves_nodes_matches_oracle=True,every_incomplete_value_zero=True,
                      graph_mapping='one graph per lane;84 blocks is nominal one warp per SM, not measured full occupancy')
        report['draw_results_sha256']=sha((args.output_dir/'draw-results.json').read_bytes())
        report['total_host_seconds_before_cleanup']=time.perf_counter()-started
        emit('SUMMARY', {k:v for k,v in report.items() if k not in ('markers','calibration','command','source_sha256')})
        require(report['kernel_ms'] <= 1000,'kernel exceeded short-launch policy; no repeat permitted')
        require(sha(args.input.read_bytes()) == INPUT_SHA and sha(args.oracle.read_bytes()) == ORACLE_SHA,
                'input/oracle changed during probe')
    except Exception as error:
        report.update(status='REFUSED_OR_FAILED',error_type=type(error).__name__,error=str(error))
        emit('FAIL',dict(error=str(error),F4_launches=report['F4_launches']))
        raise
    finally:
        cleanup=time.perf_counter()
        if cuda is not None:
            cuda.close()
        report['cleanup_seconds']=time.perf_counter()-cleanup
        report['total_host_seconds']=time.perf_counter()-started
        report['peak_host_rss_bytes']=guard.peak
        report['allocations_released']=cuda is not None
        report['v2_source_sha256']=sha(Path(__file__).read_bytes())
        with (args.output_dir/'summary.json').open('x',encoding='utf-8') as stream:
            json.dump(report,stream,indent=2)
        emit('CLOSED',dict(status=report['status'],total_host_seconds=report['total_host_seconds'],
                           F4_launches=report['F4_launches'],allocations_released=report['allocations_released']))
        events.close();guard.close()


if __name__ == '__main__':
    main()
