"""ONE bounded one-warp-per-SM batch; repeated fixed-seed C6 sample draws.

Reuses the retained serial algorithm unchanged, adding only a common absolute
GPU deadline. No value from an unfinished graph is accepted. No catalogue I/O.
"""
import argparse
import ctypes as ct
import hashlib
import json
from pathlib import Path
import random
import re
import time

from layer_gpu_f4_probe import Cuda, read_inputs


DEVICE_DEADLINE = r'''
__device__ unsigned long long shared_begin, shared_nanos;
__device__ unsigned long long gpu_nanoseconds() {
    unsigned long long result;
    asm volatile("mov.u64 %0, %%globaltimer;" : "=l"(result));
    return result;
}
__device__ bool deadline_expired() {
    return gpu_nanoseconds()-shared_begin>shared_nanos;
}
extern "C" __global__ void mark_batch(unsigned long long budget,
                                      unsigned long long* readback) {
    shared_nanos=budget;shared_begin=gpu_nanoseconds();readback[0]=shared_begin;
}
'''


def device_source():
    raw = Path(__file__).with_name("layer_gpu_f4_probe.cu").read_bytes()
    # Fail closed if the retained source shape changes unexpectedly.
    source = raw.decode()
    token = "clock64()-begin>clockCap"
    if source.count(token) != 2 or source.count("out[4]=3;") != 2:
        raise RuntimeError("retained kernel guard shape changed")
    source = source.replace(token, "(clock64()-begin>clockCap||deadline_expired())")
    source = source.replace("out[4]=3;", "out[4]=deadline_expired()?6:3;")
    source = source.replace("&1023", "&127")
    token = "for(int i=0;i<6;++i)out[i]=0;"
    if source.count(token) != 1:
        raise RuntimeError("retained kernel output initialization changed")
    source = source.replace(token, token + "if(deadline_expired()){out[4]=6;return;}")
    return raw, (DEVICE_DEADLINE + source).encode()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--oracle", type=Path, required=True)
    parser.add_argument("--execute-one-batch", action="store_true", required=True)
    parser.add_argument("--seed", type=int, default=20260905)
    parser.add_argument("--global-ms", type=int, default=700)
    args = parser.parse_args()
    if not 100 <= args.global_ms <= 700:
        parser.error("absolute GPU deadline must be100..700ms")
    started = time.perf_counter()
    inputs = read_inputs(args.input, 6, 1024)
    if len(inputs) != 1024:
        raise ValueError("requires the complete verified1024-sample text")
    oracle = {}
    oracle_bytes = args.oracle.read_bytes()
    for line in oracle_bytes.decode().splitlines():
        if re.fullmatch(r"\d+,\d+,\d+,\d+", line):
            row, value, leaves, nodes = map(int, line.split(","))
            oracle[row-1] = value, leaves, nodes
    if any(i not in oracle for i in range(1024)):
        raise ValueError("oracle does not cover all1024inputs")
    cuda = Cuda()
    try:
        sm = ct.c_int()
        cuda.check(cuda.driver.cuDeviceGetAttribute(ct.byref(sm), 16, cuda.device))
        if not 1 <= sm.value <= 128:
            raise RuntimeError("unexpected SM count, no allocation/launch")
        count = 32 * sm.value
        states, frontier = 531441, 100000
        workspace = count * (24 + 48 + states + 2 * frontier * 4)
        if workspace > 4 << 30:
            raise RuntimeError(f"one warp/SM workspace exceeds4GiB: {workspace}")
        rng = random.Random(args.seed)
        draws = [rng.randrange(1024) for _ in range(count)]
        graph_host = (ct.c_ushort * (12 * count))(*(mask for draw in draws for mask in inputs[draw][0]))
        result_host = (ct.c_uint64 * (6 * count))()
        base, source = device_source()
        stamp = time.perf_counter()
        cuda.compile(source)
        compile_seconds = time.perf_counter() - stamp
        mark = ct.c_void_p()
        cuda.check(cuda.driver.cuModuleGetFunction(ct.byref(mark), cuda.module, b"mark_batch"))
        stamp = time.perf_counter()
        graphs = cuda.alloc(ct.sizeof(graph_host))
        results = cuda.alloc(ct.sizeof(result_host))
        marks = cuda.alloc(count * states)
        fronts = cuda.alloc(count * 2 * frontier * 4)
        allocation_seconds = time.perf_counter() - stamp

        def set_deadline():
            items = [ct.c_uint64(args.global_ms * 1000000), results]
            pointers = (ct.c_void_p * 2)(*(ct.addressof(x) for x in items))
            cuda.check(cuda.driver.cuLaunchKernel(mark, 1, 1, 1, 1, 1, 1, 0, None, pointers, None))

        # Check timer scale before any large computing launch. These tiny marker
        # kernels perform one timestamp store, not F4 work.
        timer = ct.c_uint64()
        wall0 = time.perf_counter()
        set_deadline()
        cuda.check(cuda.driver.cuMemcpyDtoH_v2(ct.byref(timer), results, 8))
        timer0 = timer.value
        time.sleep(0.02)
        set_deadline()
        cuda.check(cuda.driver.cuMemcpyDtoH_v2(ct.byref(timer), results, 8))
        timer_seconds = (timer.value - timer0) / 1e9
        wall_seconds = time.perf_counter() - wall0
        if not 0.005 <= timer_seconds <= 0.2 or not 0.5 <= timer_seconds / wall_seconds <= 1.5:
            raise RuntimeError("GPU-globaltimer nanosecond calibration failed; no F4 launch")
        print("PREFLIGHT", json.dumps({"SMs": sm.value, "graphs": count,
            "blocks": sm.value, "threads_per_block": 32, "draws_with_replacement": True,
            "seed": args.seed, "unique_sample_indices": len(set(draws)),
            "allocated_bytes": workspace, "compile_seconds": compile_seconds,
            "allocation_seconds": allocation_seconds, "timer_seconds": timer_seconds,
            "timer_wall_seconds": wall_seconds, "global_deadline_ms": args.global_ms,
            "base_kernel_sha256": hashlib.sha256(base).hexdigest(),
            "compiled_kernel_sha256": hashlib.sha256(source).hexdigest(),
            "function": cuda.function_attributes}), flush=True)
        if time.perf_counter() - started > 110:
            raise RuntimeError("host120s bound: insufficient margin before launch")
        total_stamp = time.perf_counter()
        cuda.check(cuda.driver.cuMemcpyHtoD_v2(graphs, graph_host, ct.sizeof(graph_host)))
        cuda.check(cuda.driver.cuMemsetD8_v2(marks, 0, count * states))
        cuda.check(cuda.driver.cuCtxSynchronize())
        init_seconds = time.perf_counter() - total_stamp
        # Local cap is650ms at the reported clock, independently bounded by
        # the common deadline even if clocks differ or blocks start late.
        local_clock_cap = cuda.attributes["clock_khz"] * 650
        items = [graphs, results, marks, fronts, ct.c_int(count), ct.c_int(12),
                 ct.c_int(states), ct.c_int(frontier), ct.c_uint64(2000000),
                 ct.c_uint64(local_clock_cap)]
        pointers = (ct.c_void_p * len(items))(*(ct.addressof(x) for x in items))
        first, last = ct.c_void_p(), ct.c_void_p()
        cuda.check(cuda.driver.cuEventCreate(ct.byref(first), 0))
        cuda.check(cuda.driver.cuEventCreate(ct.byref(last), 0))
        try:
            set_deadline()
            cuda.check(cuda.driver.cuEventRecord(first, None))
            cuda.check(cuda.driver.cuLaunchKernel(cuda.function, sm.value, 1, 1, 32, 1, 1,
                                                  0, None, pointers, None))
            cuda.check(cuda.driver.cuEventRecord(last, None))
            cuda.check(cuda.driver.cuEventSynchronize(last))
            elapsed = ct.c_float()
            cuda.check(cuda.driver.cuEventElapsedTime(ct.byref(elapsed), first, last))
        finally:
            cuda.driver.cuEventDestroy_v2(first)
            cuda.driver.cuEventDestroy_v2(last)
        cuda.check(cuda.driver.cuMemcpyDtoH_v2(result_host, results, ct.sizeof(result_host)))
        charged_seconds = time.perf_counter() - total_stamp
        statuses = {}
        complete = 0
        sum_values = sum_leaves = sum_nodes = 0
        attempted_nodes = []
        examples = []
        for i, draw in enumerate(draws):
            value, leaves, nodes, dp, status, ticks = result_host[6*i:6*i+6]
            statuses[status] = statuses.get(status, 0) + 1
            if not status:
                if (value, leaves, nodes) != oracle[draw]:
                    raise RuntimeError(f"CPU/GPU differential draw{i}/sample{draw}")
                complete += 1
                sum_values += value
                sum_leaves += leaves
                sum_nodes += nodes
            else:
                if value:
                    raise RuntimeError("partial value escaped refusal guard")
                attempted_nodes.append(nodes)
                if len(examples) < 8:
                    examples.append({"draw": i, "sample": draw, "flag": status,
                                     "nodes": nodes, "dp_steps": dp, "ticks": ticks, "value": value})
        all_closed = complete == count
        print("SUMMARY", json.dumps({"graphs": count, "complete_exact": complete,
            "incomplete_no_value": count-complete, "statuses": statuses,
            "all_closed": all_closed, "accepted_value_sum": sum_values,
            "accepted_leaves": sum_leaves, "accepted_nodes": sum_nodes,
            "incomplete_nodes_min": min(attempted_nodes, default=0),
            "incomplete_nodes_max": max(attempted_nodes, default=0), "refusal_examples": examples,
            "kernel_ms": elapsed.value, "H2D_workspace_zero_seconds": init_seconds,
            "transfer_init_launch_readback_seconds": charged_seconds,
            "all_closed_graphs_per_second": count/charged_seconds if all_closed else None,
            "total_host_seconds": time.perf_counter()-started,
            "input_sha256": hashlib.sha256(args.input.read_bytes()).hexdigest(),
            "oracle_sha256": hashlib.sha256(oracle_bytes).hexdigest(),
            "fresh_CPU_benchmark": False, "N6_computed": False}), flush=True)
        if elapsed.value > 1000 or time.perf_counter()-started > 120:
            raise RuntimeError("bounded launch exceeded policy; no repeat permitted")
    finally:
        cuda.close()


if __name__ == "__main__":
    main()
