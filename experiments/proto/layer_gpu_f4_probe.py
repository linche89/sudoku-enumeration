"""Bounded NVRTC/driver-only exact GPU F4 probe. No installation/catalogue I/O."""
import argparse
import ctypes as ct
import hashlib
import json
import os
from pathlib import Path
import re
import time


def signature(dll, name, result, arguments):
    function = getattr(dll, name)
    function.restype = result
    function.argtypes = arguments
    return function


class Cuda:
    def __init__(self):
        self.directory = Path("C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.8/bin")
        self.cookie = os.add_dll_directory(str(self.directory))
        self.rtc = ct.CDLL(str(self.directory / "nvrtc64_120_0.dll"))
        self.driver = ct.WinDLL("nvcuda.dll")
        self.allocations = []
        self.context = ct.c_void_p()
        self.module = ct.c_void_p()
        self._bind()
        self.check(self.driver.cuInit(0))
        self.device = ct.c_int()
        self.check(self.driver.cuDeviceGet(ct.byref(self.device), 0))
        self.check(self.driver.cuCtxCreate_v2(ct.byref(self.context), 0, self.device))
        name = ct.create_string_buffer(256)
        self.check(self.driver.cuDeviceGetName(name, len(name), self.device))
        self.name = name.value.decode()
        self.attributes = {}
        for label, number in (("major", 75), ("minor", 76), ("clock_khz", 13), ("kernel_timeout", 17)):
            value = ct.c_int()
            self.check(self.driver.cuDeviceGetAttribute(ct.byref(value), number, self.device))
            self.attributes[label] = value.value

    def _bind(self):
        p, i, u, z, q = ct.c_void_p, ct.c_int, ct.c_uint, ct.c_size_t, ct.c_uint64
        defs = {
            "cuInit": [u], "cuDeviceGet": [ct.POINTER(i), i],
            "cuCtxCreate_v2": [ct.POINTER(p), u, i], "cuCtxDestroy_v2": [p],
            "cuDeviceGetName": [p, i, i], "cuDeviceGetAttribute": [ct.POINTER(i), i, i],
            "cuModuleLoadData": [ct.POINTER(p), p], "cuModuleUnload": [p],
            "cuModuleGetFunction": [ct.POINTER(p), p, ct.c_char_p],
            "cuMemAlloc_v2": [ct.POINTER(q), z], "cuMemFree_v2": [q],
            "cuMemcpyHtoD_v2": [q, p, z], "cuMemcpyDtoH_v2": [p, q, z],
            "cuMemsetD8_v2": [q, ct.c_ubyte, z], "cuCtxSynchronize": [],
            "cuMemGetInfo_v2": [ct.POINTER(z), ct.POINTER(z)],
            "cuLaunchKernel": [p, u, u, u, u, u, u, u, p, ct.POINTER(p), p],
            "cuEventCreate": [ct.POINTER(p), u], "cuEventRecord": [p, p],
            "cuEventSynchronize": [p], "cuEventElapsedTime": [ct.POINTER(ct.c_float), p, p],
            "cuEventDestroy_v2": [p], "cuFuncGetAttribute": [ct.POINTER(i), i, p],
        }
        for name, args in defs.items():
            signature(self.driver, name, i, args)
        rtc_defs = {
            "nvrtcCreateProgram": [ct.POINTER(p), ct.c_char_p, ct.c_char_p, i, p, p],
            "nvrtcCompileProgram": [p, i, ct.POINTER(ct.c_char_p)],
            "nvrtcGetProgramLogSize": [p, ct.POINTER(z)], "nvrtcGetProgramLog": [p, p],
            "nvrtcGetPTXSize": [p, ct.POINTER(z)], "nvrtcGetPTX": [p, p],
            "nvrtcDestroyProgram": [ct.POINTER(p)],
        }
        for name, args in rtc_defs.items():
            signature(self.rtc, name, i, args)

    @staticmethod
    def check(code):
        if code:
            raise RuntimeError(f"CUDA/NVRTC error {code}")

    def compile(self, source):
        program = ct.c_void_p()
        self.check(self.rtc.nvrtcCreateProgram(ct.byref(program), source, b"bounded_f4.cu", 0, None, None))
        try:
            arch = f"--gpu-architecture=compute_{self.attributes['major']}{self.attributes['minor']}".encode()
            options = (ct.c_char_p * 2)(arch, b"--std=c++17")
            status = self.rtc.nvrtcCompileProgram(program, len(options), options)
            size = ct.c_size_t()
            self.check(self.rtc.nvrtcGetProgramLogSize(program, ct.byref(size)))
            log = ct.create_string_buffer(size.value)
            self.check(self.rtc.nvrtcGetProgramLog(program, log))
            if log.value:
                print("NVRTC_LOG", log.value.decode(), flush=True)
            self.check(status)
            self.check(self.rtc.nvrtcGetPTXSize(program, ct.byref(size)))
            ptx = ct.create_string_buffer(size.value)
            self.check(self.rtc.nvrtcGetPTX(program, ptx))
            self.check(self.driver.cuModuleLoadData(ct.byref(self.module), ptx))
            self.function = ct.c_void_p()
            self.check(self.driver.cuModuleGetFunction(ct.byref(self.function), self.module, b"rooted_f4"))
            self.function_attributes = {}
            for name, number in (("registers", 4), ("local_bytes", 3), ("max_threads", 0)):
                value = ct.c_int()
                self.check(self.driver.cuFuncGetAttribute(ct.byref(value), number, self.function))
                self.function_attributes[name] = value.value
        finally:
            self.rtc.nvrtcDestroyProgram(ct.byref(program))

    def alloc(self, amount):
        if sum(size for _, size in self.allocations) + amount > 4 << 30:
            raise RuntimeError("4 GiB allocation cap")
        free, total = ct.c_size_t(), ct.c_size_t()
        self.check(self.driver.cuMemGetInfo_v2(ct.byref(free), ct.byref(total)))
        if amount + (1 << 30) > free.value:
            raise RuntimeError("less than 1 GiB GPU safety reserve")
        pointer = ct.c_uint64()
        self.check(self.driver.cuMemAlloc_v2(ct.byref(pointer), amount))
        self.allocations.append((pointer, amount))
        return pointer

    def close(self):
        for pointer, _ in reversed(self.allocations):
            self.driver.cuMemFree_v2(pointer)
        if self.module:
            self.driver.cuModuleUnload(self.module)
        if self.context:
            self.driver.cuCtxDestroy_v2(self.context)


def read_inputs(path, boxes, limit):
    records = []
    for line in path.read_text().splitlines():
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        fields = line.split()
        masks = sorted(map(int, fields[:2 * boxes]))
        if len(masks) != 2 * boxes or any(mask.bit_count() != 4 or mask >> (2 * boxes) for mask in masks):
            raise ValueError("requires degree-four graphs")
        if any(sum(mask >> col & 1 for mask in masks) != 4 for col in range(2 * boxes)):
            raise ValueError("unbalanced graph")
        expected = None
        if len(fields) > 2 * boxes:
            if len(fields) != 2 * boxes + 1 or not fields[-1].startswith("expected="):
                raise ValueError("unexpected input suffix")
            expected = int(fields[-1].split("=", 1)[1])
        records.append((masks + [0] * (12 - len(masks)), expected))
        if len(records) == limit:
            return records
    if not records:
        raise ValueError("empty input")
    return records


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--C", type=int, choices=(4, 5, 6), required=True)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--oracle", type=Path, required=True)
    parser.add_argument("--limit", type=int, required=True)
    parser.add_argument("--batch", type=int, default=32)
    parser.add_argument("--node-cap", type=int, default=2000000)
    parser.add_argument("--clock-cap", type=int, default=100000000)
    parser.add_argument("--frontier-cap", type=int, default=100000)
    parser.add_argument("--seconds", type=float, default=120)
    parser.add_argument("--allow-incomplete", action="store_true")
    args = parser.parse_args()
    if not 1 <= args.limit <= 1024 or not 1 <= args.batch <= 128 or not 1 <= args.node_cap <= 5000000:
        parser.error("limit<=1024 batch<=128 node-cap<=5m must be positive")
    if not 1 <= args.clock_cap <= 1000000000 or not 0 < args.seconds <= 120:
        parser.error("clock cap<=1b ticks, host seconds<=120 must be positive")
    if args.clock_cap > 200000000 and (args.batch != 1 or args.limit != 1):
        parser.error("above200m clocks allowed only for ONE single-graph launch")
    if not 1 <= args.frontier_cap <= 100000:
        parser.error("frontier-cap must be1..100000")
    started = time.perf_counter()
    inputs = read_inputs(args.input, args.C, args.limit)
    oracle = {}
    for line in args.oracle.read_text().splitlines():
        if re.fullmatch(r"\d+,\d+,\d+,\d+", line):
            row, value, leaves, nodes = map(int, line.split(","))
            oracle[row - 1] = (value, leaves, nodes)
    if any(i not in oracle for i in range(len(inputs))):
        raise ValueError("CPU oracle must cover every source")
    for i, (_, value) in enumerate(inputs):
        if value is not None and value != oracle[i][0]:
            raise ValueError("closed text and CPU oracle disagree")
    cuda = Cuda()
    try:
        source = Path(__file__).with_suffix(".cu").read_bytes()
        stamp = time.perf_counter()
        cuda.compile(source)
        compile_seconds = time.perf_counter() - stamp
        n = args.C * 2
        states, frontier = 3 ** n, args.frontier_cap
        batch = min(args.batch, len(inputs))
        graph_memory = cuda.alloc(batch * 12 * 2)
        result_memory = cuda.alloc(batch * 6 * 8)
        mark_memory = cuda.alloc(batch * states)
        front_memory = cuda.alloc(batch * 2 * frontier * 4)
        print("DEVICE", json.dumps({"name": cuda.name, **cuda.attributes,
              **cuda.function_attributes, "allocated_bytes": sum(s for _, s in cuda.allocations),
              "compile_seconds": compile_seconds, "kernel_sha256": hashlib.sha256(source).hexdigest()}), flush=True)
        complete = incomplete = 0
        kernel_ms = host_seconds = maximum_kernel_ms = 0.0
        value_sum = leaves_sum = nodes_sum = 0
        for start in range(0, len(inputs), batch):
            if time.perf_counter() - started > args.seconds:
                raise RuntimeError("host time bound")
            rows = inputs[start:start + batch]
            graphs = (ct.c_ushort * (len(rows) * 12))(*(mask for masks, _ in rows for mask in masks))
            result = (ct.c_uint64 * (len(rows) * 6))()
            stamp = time.perf_counter()
            cuda.check(cuda.driver.cuMemcpyHtoD_v2(graph_memory, graphs, ct.sizeof(graphs)))
            cuda.check(cuda.driver.cuMemsetD8_v2(mark_memory, 0, batch * states))
            parameters = [graph_memory, result_memory, mark_memory, front_memory,
                          ct.c_int(len(rows)), ct.c_int(n), ct.c_int(states), ct.c_int(frontier),
                          ct.c_uint64(args.node_cap), ct.c_uint64(args.clock_cap)]
            pointers = (ct.c_void_p * len(parameters))(*(ct.addressof(value) for value in parameters))
            first, last = ct.c_void_p(), ct.c_void_p()
            cuda.check(cuda.driver.cuEventCreate(ct.byref(first), 0))
            cuda.check(cuda.driver.cuEventCreate(ct.byref(last), 0))
            try:
                cuda.check(cuda.driver.cuEventRecord(first, None))
                cuda.check(cuda.driver.cuLaunchKernel(cuda.function, (len(rows) + 31) // 32, 1, 1,
                                                      32, 1, 1, 0, None, pointers, None))
                cuda.check(cuda.driver.cuEventRecord(last, None))
                cuda.check(cuda.driver.cuEventSynchronize(last))
                elapsed = ct.c_float()
                cuda.check(cuda.driver.cuEventElapsedTime(ct.byref(elapsed), first, last))
            finally:
                cuda.driver.cuEventDestroy_v2(first)
                cuda.driver.cuEventDestroy_v2(last)
            cuda.check(cuda.driver.cuMemcpyDtoH_v2(result, result_memory, ct.sizeof(result)))
            host_seconds += time.perf_counter() - stamp
            kernel_ms += elapsed.value
            maximum_kernel_ms = max(maximum_kernel_ms, elapsed.value)
            statuses = {}
            refusals = []
            for local in range(len(rows)):
                value, leaves, nodes, dp, status, ticks = result[6 * local:6 * local + 6]
                statuses[status] = statuses.get(status, 0) + 1
                if status:
                    if value:
                        raise RuntimeError("partial value escaped failure guard")
                    incomplete += 1
                    refusals.append({"graph": start + local, "flag": status, "nodes": nodes,
                                     "dp_steps": dp, "ticks": ticks, "value": value})
                    if not args.allow_incomplete:
                        raise RuntimeError(f"INCOMPLETE graph={start+local} flag={status} nodes={nodes} dp={dp}")
                else:
                    if (value, leaves, nodes) != oracle[start + local]:
                        raise RuntimeError(f"CPU/GPU differential graph={start+local}: {(value,leaves,nodes)} vs {oracle[start+local]}")
                    complete += 1
                    value_sum += value
                    leaves_sum += leaves
                    nodes_sum += nodes
            print("BATCH", json.dumps({"start": start, "graphs": len(rows), "kernel_ms": elapsed.value,
                                       "statuses": statuses, "refusals": refusals}), flush=True)
            if elapsed.value > (500 if args.limit == args.batch == 1 else 250):
                raise RuntimeError("short-kernel policy: >500ms singleton or >250ms batch; no further launch")
        print("SUMMARY", json.dumps({"C": args.C, "graphs": len(inputs), "complete_exact": complete,
            "incomplete_no_value": incomplete, "value_sum": value_sum, "leaves_sum": leaves_sum,
            "nodes_sum": nodes_sum, "kernel_ms": kernel_ms, "maximum_kernel_ms": maximum_kernel_ms,
            "batch_host_seconds": host_seconds, "total_seconds": time.perf_counter() - started,
            "CPU_oracle": str(args.oracle), "CPU_oracle_sha256": hashlib.sha256(args.oracle.read_bytes()).hexdigest(),
            "concurrent_CPU_F4": True, "N6_computed": False}), flush=True)
    finally:
        cuda.close()


if __name__ == "__main__":
    main()
