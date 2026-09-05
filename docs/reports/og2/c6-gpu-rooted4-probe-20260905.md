# Bounded GPU rooted-F4 probe: correct singleton, no speedup established

Date: 2026-09-05. This is a scoped negative decision for a literal
**one-graph-per-GPU-thread** port of the audited rooted degree-four split.
It is not a GPU impossibility result and does not assess a cooperative
warp/block implementation or a large occupancy-qualified batch. No
production kernel, checkpoint, controller or saved production value was
changed. The experiment computed only the separately verified graph F4
values explicitly listed below, not F5 or N(6).

## Local toolchain and exact algorithm

Read-only local discovery found:

```text
GPU: NVIDIA GeForce RTX 5080
driver: 596.36
compute capability: 12.0
reported memory: 16,303 MiB; initially about 13,070 MiB free
CUDA/NVRTC: 12.8
reported clock: 2,625,000 kHz
kernel execution timeout enabled: yes
```

The installed NVRTC DLL and `nvcuda.dll` loaded successfully through Python
3.13 ctypes. MSVC 14.38/14.44 were present, although `cl` was not on PATH.
The executed route needed no host compiler, package installation, network,
environment change or modification of the running CPU producer.

The retained new files are:

```text
experiments/proto/layer_gpu_f4_probe.cu
experiments/proto/layer_gpu_f4_probe.py
```

The device code ports `computeDegree4RootedSplit` in
`src/factorization_orbit.cpp`. It fixes the first two incident edges of the
first left vertex and enumerates spanning degree-two subgraphs H containing
that root pair. It computes

```text
F4(G) = 6 * sum_H 2^(components(H) + components(G-H)).
```

The factor six chooses which unordered pair of the four ordered colors
occurs on the fixed root pair. For each such H, its two ordered colors and
the complement's two ordered colors each have two choices on every cycle.
Thus the identity counts all ordered four-colorings exactly. The GPU uses
the same row ordering and root pair as the CPU oracle.

The forward stage marks reachable ternary column-degree states. Every digit
is in {0,1,2}; its digit sum fixes its row layer. The backward traversal
accepts only marked predecessors and uses two rollback disjoint-set
structures to count the two-factor components. Recursion is replaced by an
explicit fixed-size stack, without changing the search tree. Arithmetic is
unsigned 64-bit integer. The general bound F4<=24^12<2^64 and explicit
addition/final-multiplication checks prevent accepted overflow.

This direct mapping has substantial serial work and divergent paths. Its
per-thread local arrays used 1,536 bytes; the final build used 40 registers.
The ternary marks and bounded frontiers are global GPU arrays private to
each graph. No cooperative work sharing was implemented.

## Bounds and fail-closed behavior

Host runs require a positive input limit <=1,024, batch <=128, a maximum
120-second host budget, and positive limits on traversal work and device
clock ticks. Explicit device allocations are capped at 4 GiB and require an
additional 1 GiB of free GPU memory. The actually executed largest workspace
allocation was 20,972,458 bytes; the final C6 singleton used 1,331,513 bytes.
CUDA context/driver memory is separate from those explicit workspace bytes;
no large allocation or catalogue was loaded.

Default short kernels have a 100-million-clock cap. The final explicitly
approved singleton used one billion clocks, approximately 0.38 seconds at
the reported device clock. A budget above 200 million clocks is refused
unless both the total input limit and batch size equal one. The host refuses
further launches after a kernel exceeds 250 ms, or 500 ms for that one
singleton. The actual longest kernel was 190.046 ms, well below the WDDM
timeout. No timeout/reset occurred.

Outputs have separate status fields:

| Status | Meaning |
|---:|---|
| 0 | Complete closed result |
| 1 | DP-transition or DFS-node cap reached |
| 2 | Frontier workspace cap reached |
| 3 | Device clock cap reached |
| 4 | Integer overflow would occur |
| 5 | Invalid input or inconsistent terminal state |

A nonzero status always leaves the returned value zero. Partial leaf/node
counts are diagnostics only; the host never adds them to accepted exact
totals. Node-cap=1, frontier-cap=1 and clock-cap=1 probes each triggered the
expected status 1,2,3 and confirmed that no partial value escaped.

## Exact finite checks and timings

Existing per-input CPU logs were used as the numerical oracle, avoiding a
competing 24-CPU reevaluation during the owner's active F4 window. Every
accepted GPU graph agreed in **all three** of F4, two-factor leaves and DFS
nodes, not merely in a final sum.

| Domain | Accepted graphs | Sum F4 | Leaves | DFS nodes | Sum kernel time |
|---|---:|---:|---:|---:|---:|
| Complete C4 L4 | 26 | 1,365,888 | 14,484 | 49,113 | 120.537 ms |
| First C5 L4 fixtures | 4 | 943,488 | 11,196 | 41,488 | 97.531 ms |
| First uniform C6 L4 sample | 1 | 1,634,688 | 16,456 | 78,415 | 190.046 ms |

The successful C4/C5 runs used one graph per launch. Earlier divergent
multi-graph trials reached the short clock cap: all eight tested C5 graphs
and all four tested C6 graphs were explicitly incomplete. They contributed
no accepted values. The original complete-C4 attempt with batch 26 also
stopped on a clock refusal; the subsequent singleton schedule closed all 26.

With a 200-million-clock cap, the first C6 singleton stopped after 73.358 ms
at 26,116 DFS nodes following 59,760 forward transitions. Its numerical
output was zero. Increasing only that singleton's allowed clock budget
closed it exactly; this distinguishes algorithm correctness from an overly
short guard.

Final C6 command:

The one tested graph, in sorted true-slot masks, is self-contained:

```text
102 102 170 533 1161 1297 1360 1440 2436 2569 2570 2640
```

Executed command:

```powershell
python experiments/proto/layer_gpu_f4_probe.py --C 6 `
  --input data/logs/c6-direct-route-20260905/l4-sample.txt `
  --oracle data/logs/c6-direct-route-20260905/l4-parallel24-1024.log `
  --limit 1 --batch 1 --clock-cap 1000000000 --seconds 120
```

Selected exact output:

```text
complete_exact=1 incomplete_no_value=0
value_sum=1634688 leaves_sum=16456 nodes_sum=78415
kernel_ms=190.0457305908203
batch_host_seconds=0.19051719999697525
compile_seconds=0.1022062999982154
total_seconds=0.3596207000009599
allocated_bytes=1331513
concurrent_CPU_F4=true N6_computed=false
```

The batch-host interval includes H2D input copying, zeroing of the ternary
workspace, kernel event/launch handling and D2H result copying. Allocation,
compilation/context work and input/oracle parsing are included in the total
host time, not hidden in the kernel measurement.

For context, retained CPU measurements evaluated all 1,024 C6 sample graphs
in 3.0679564 kernel seconds with one CPU thread or 0.1699464 kernel seconds
with 24 CPU threads. The corresponding C4 26-graph eight-thread CPU gate took
0.0004511 kernel seconds. These historical measurements are **not** a fresh,
same-load CPU/GPU throughput comparison: the GPU tests used tiny batches,
and the owner was running the real CPU producer concurrently. In particular
the C6 singleton underoccupies the GPU extremely strongly. No general
throughput lower bound or impossibility claim follows from these numbers.

Decision: the literal serial GPU port gives no reason to replace the
working 24-CPU F4 engine. Further work would need a different mapping, such
as cooperative graph processing or independent-prefix tasks with controlled
divergence and workspace traffic. Such a redesign was not attempted here.

## Provenance and retained logs

Final device source SHA-256:

```text
0EDB6BCF9F8C50D52648A7F0366B28AC4E6C7EF4D507DC669CC3321D8EB08BF9
```

Python driver SHA-256:

```text
775908D26757D1A77C27013742AC8B09FD542C3CC543C1C0F37C305585B73520
```

The earlier C4/C5 runs used device source SHA
`A44F560B609EBAC8857EF20824CE89D8E295D02E7120F332912869A6F5F26BAD`.
The subsequent change only retained more progress diagnostics on refusal
returns; accepted arithmetic/search semantics were not changed. The final
source was used for the C6 closure and all three explicit refusal tests.

C6 sample text SHA-256:

```text
38F4B8EF65FA4BEEF7FB05995E2A64B7096162A5E1212A0388AF5A9E195CA99C
```

CPU oracle SHA-256 values:

```text
C4 c4-l4-parallel8-gate.log:
41380C7044EB5DB069C49BC02F8D678BD4147F683EED7966119EDCF7BD39ED4D
C5 c5-l4-parallel24-gate.log:
FC09FB4390EF269AACC226BAF1DA173B4A8ADD7166A012B08701C72038136C62
C6 l4-parallel24-1024.log:
8B3DD7335033F834631AB10C930807E4ADEB0C3CE4B8571145737A6EEFE80F1A
```

All logs below are under `data/logs/c6-direct-route-20260905/`:

```text
gpu-f4-c4-26.log                 # refused earlier batch, not complete
gpu-f4-c4-26-b1.log              # complete26, exact
gpu-f4-c5-8.log                  # eight clock refusals
gpu-f4-c5-4-b1.log               # completefour, exact
gpu-f4-c6-4-cap.log              # four clock refusals
gpu-f4-c6-1-b1.log               # tighter singleton cap, incomplete
gpu-f4-c6-1-closed.log           # final exact singleton closure
gpu-f4-node-refusal.log
gpu-f4-workspace-refusal.log
gpu-f4-clock-refusal.log
```

The active CPU checkpoint was never opened by the GPU host. No GPU probe
process remained at handoff. The retained prototype is not linked into any
standard build or production route.

## Later occupancy qualification attempt: rejected before computation

The coordinating agent subsequently requested one near-one-warp-per-SM throughput batch
because singleton latency does not qualify GPU throughput. A read-only
driver query found 84 SMs. A batch of 2,688 graphs would need exactly
3,579,106,944 explicitly allocated bytes, below the 4-GiB cap. The planned
inputs were fixed-seed repeated draws, with replacement, from the same
1,024 independently verified C6 sample graphs, not 2,688 new native states.

The separate retained driver is
`experiments/proto/layer_gpu_f4_occupancy_probe.py`, SHA-256:

```text
0347C4863AF68786EBF5245873AEF7F655A1AA46C89777E54C095254184CBA73
```

It preserves the serial algorithm and adds a common absolute device deadline
using the locally documented PTX global timer. A tiny marker kernel sets the
deadline before the intended computing launch; late-starting blocks would
share that same deadline instead of getting independent extra waves of
runtime. Node/workspace/local-clock guards remain, and unfinished values
remain zero. The intended batch deadline was 700 ms, with frequent checks.

The one authorized attempt executed:

```powershell
python experiments/proto/layer_gpu_f4_occupancy_probe.py `
  --input data/logs/c6-direct-route-20260905/l4-sample.txt `
  --oracle data/logs/c6-direct-route-20260905/l4-parallel24-1024.log `
  --execute-one-batch --seed 20260905 --global-ms 700
```

It failed closed with:

```text
RuntimeError: GPU-globaltimer nanosecond calibration failed; no F4 launch
```

The 3.579-GB allocation succeeded and was released by `finally`. Only two
one-thread timestamp-marker kernels ran. **The 2,688-graph F4 kernel never
launched**, so there are no closed/aborted graph counts, accepted sums or
throughput measurements from this attempt. The exception happened before
the preflight JSON was printed; raw timer/wall calibration values were not
retained. The attempted stdout log is therefore not a numerical certificate.

The guard compares a device interval to a host interval that starts before
the first marker launch. First-launch overhead could bias that comparison,
but the missing raw measurements do not establish the actual cause. This is
a preflight rejection, **not** evidence that a full batch cannot meet its
deadline or that the mapping cannot deliver throughput. No retry or larger
GPU workload was performed after the coordinating agent requested that this attempt be
frozen. All allocations and probe processes were released.
