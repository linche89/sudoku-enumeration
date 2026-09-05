# Packed-degree F4 without ternary DP: exact tiny gates, partial GPU batch

Date:2026-09-05. An independent exact fixed-root F4 kernel removes the ternary
reachability table and per-graph frontier. Complete C4 and bounded C5/C6 tiny
GPU gates pass. One subsequently authorized2,688-draw GPU batch closed59 draws
and aborted2,629 at its unchanged local clock cap, with all unfinished values
zero. Thus the experiment establishes a compact exact kernel on these finite
checks, **not complete-batch throughput, production acceleration, or N(6)**.

No released source, production checkpoint, controller, old GPU prototype or
old report was changed. No GPU launch followed the one authorized occupancy
batch. The original CPU oracle was only read, never recalculated.

## Exact identity and pruning

**(A) Self-contained proof.** Let G be a simple four-regular bipartite graph
with n vertices on each side. Fix two incident edges of the first left vertex.
Let H range over spanning two-factors containing exactly these two edges there.
Then

\[
F_4(G)=6\sum_H 2^{c(H)+c(G-H)}.
\]

For an ordered proper four-edge-coloring, those fixed edges carry one unordered
pair of colors. There are six such pairs. The union of these two colors is one
admissible H. Conversely, for fixed H and fixed unordered color pair, each
cycle in H and G-H has independently two alternating assignments of its two
colors. These constructions are inverse, proving the formula with no extra
row, symbol or coordinate factor.

The new enumeration selects one of six column pairs on each left row, keeping
the first row's pair fixed exactly as in the retained CPU oracle. Column
degrees0,1,2 are represented by disjoint bitsets `once` and `twice`. Adding
column pair p is legal only if `p & twice == 0`, and updates

```text
nextOnce  = once ^ p
nextTwice = twice | (once & p).
```

**(A) Self-contained proof.** A selected pair increments each of its two
distinct columns once. The update changes degree0 to1 and degree1 to2; the
precheck excludes degree2 to3. Hence it represents precisely these degrees.

Precomputed suffix masks state which columns occur in at least one, or at
least two, remaining rows. A currently degree-zero column needs at least two
remaining incidences; a degree-one column needs at least one. Rejecting a
violation cannot discard any completion. These necessary tests are not claimed
to guarantee a completion. Exhausting all six row choices, with this safe
pruning and final degree-two checks, visits every valid H exactly once. Therefore
the fixed-root valid-leaf count must equal the independent existing oracle's
leaf count, although traversal-node counts need not agree.

**(A) Self-contained proof.** At a valid leaf, suppress each degree-two left
vertex: its chosen pair becomes an edge between two distinct right vertices.
The resulting right-side multigraph is two-regular, with parallel edges allowed,
and suppression preserves components. Every component is a cycle, including a
two-vertex parallel-edge cycle. Starting from n separate right vertices, each
successful disjoint-set union reduces components by one; therefore its cycle
count is n minus successful unions. Repeating for complementary row pairs gives
the exponent above. Parent labels0..11 fit twelve4-bit fields in one u64; joining
roots maintains a forest and makes the packed find operation terminate.

All counting is u64 integer arithmetic. Explicit addition and final-times-six
overflow checks remain. Independently, an ordered coloring assigns one of24
color permutations to each left row, so F4<=24^n<2^64 for n<=12.

## Implementation and guard

New isolated files:

- `experiments/proto/layer_gpu_f4_nodp_core.h`: portable host/device exact core,
  packed degree/suffix state and packed right-side disjoint sets.
- `experiments/proto/layer_gpu_f4_nodp_cpu_gate.cpp`: bounded new-core CPU check
  against retained oracle CSV, never the original factorization evaluator.
- `experiments/proto/layer_gpu_f4_nodp_probe.py`: tiny gate driver plus the
  subsequently authorized fixed one-batch occupancy mode.

The GPU driver uses the already qualified synchronized timestamp-marker
calibration and shared absolute700ms deadline. Each invocation has one computing
launch maximum, a2,000,000-node cap, local tick cap1,706,250,000, and120s/1GiB
host guards. Explicit GPU allocations are capped at1GiB. The core polls the
guard at entry and every128 traversal iterations. Between polls, the graph and
union loops have fixed n<=12 bounds. Late blocks share the same absolute
deadline rather than receiving additional execution windows.

No ternary/global-frontier arrays are allocated. Only24 input bytes and48 result
bytes per graph are explicitly allocated. The compiled kernel reports39
registers and288 private local bytes per thread, compared with40 and1,536 for
the earlier ternary-DP port. CUDA context and implicit compiler local storage
are separate from the explicit allocation totals.

Any refusal returns before assigning the final value, leaving it zero. Partial
leaf/node/iteration counters are retained only as diagnostics. The tiny gates
compare every F4 and valid-leaf value with the independent retained oracle and
also compare the new traversal's node/iteration counts with its CPU execution.
The occupancy test reads only the existing oracle; it performs no CPU counting.

## Executed exact tiny gates

**(C) Exact finite checks, not a complete C5 production gate.**

| Domain | Closed / tested | Sum F4 | Valid leaves | New DFS nodes | New iterations | GPU event time |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Complete C4 L4 |26/26 |1,365,888 |14,484 |43,566 |218,058 |16.164448ms |
| First four C5 L4 fixtures |4/4 |943,488 |11,196 |46,310 |256,994 |33.232128ms |
| First C6 L4 sample |1/1 |1,634,688 |16,456 |71,062 |398,698 |87.957085ms |

Explicit GPU bytes were1,872,288 and72 respectively. Total host times were
0.991850,0.378720 and0.387795s, including each new-core CPU gate, context,
compilation/calibration and cleanup. The first C6 masks were:

```text
102 102 170 533 1161 1297 1360 1440 2436 2569 2570 2640
```

The new-core CPU execution of that graph took about2.93ms in the initial tiny
check. These observations are not a same-load CPU/GPU throughput comparison.
The C6 singleton event time is lower than the earlier190.046ms singleton, but
no full-C6 runtime or globally representative speedup follows from that fact.

## Exactly one subsequent occupancy qualification

After review of the tiny results, a separately authorized batch used the exact
same ordered2,688 draws as the earlier occupancy attempt: seed20260905, draws
with replacement from1,024 verified sample rows,942 distinct sample indices.
The grid was84 blocks ×32 lanes on the84-SM device; actual hardware occupancy
was not measured. A bounded read-only postcheck confirmed every draw index
matched the previous batch's corresponding index.

Both fixed timestamp intervals passed their predeclared host-bracket tests:
device20,583,296ns against host20,460,800–20,606,300ns, and device42,190,432ns
against host42,061,500–42,316,200ns. All raw timestamps are retained.

| Quantity | Actual result |
| --- | ---: |
| Computing launches |1 |
| Closed draws / distinct closed sample indices |59 /25 |
| Aborted draws |2,629 |
| Abort status |3(local clock), all unfinished values0 |
| Accepted F4 sum |93,015,552 |
| Accepted valid-leaf sum |903,995 |
| Accepted new nodes / iterations |2,967,394 /15,347,788 |
| Explicit GPU allocation |193,536bytes |
| Compilation |0.106313s |
| Allocation |0.000143s |
| Calibration including markers and sleeps |0.063381s |
| H2D |0.0000238s |
| Workspace initialization |0(no workspace) |
| GPU kernel event |590.661804ms |
| D2H |0.0000731s |
| Combined transfer/marker/launch/readback |0.5909967s |
| Cleanup |0.0153982s |
| Total host time |0.9360211s |
| Peak host RSS |180,039,680bytes |

Every closed F4/valid-leaf pair agrees with the retained independent oracle.
Old and new traversal-node counts are deliberately not equated. Aborted
new-node diagnostics ranged43,869–55,683 and iteration diagnostics ranged
262,784–294,912; these are not completed values.

`all_closed=false`; both hot and end-to-end complete-batch throughput fields
are explicitly null. Closed draws are selected by the runtime cap, so their
completion rate must not be extrapolated as unbiased full-workload throughput.
No cap was raised and no GPU kernel was launched afterward in this turn.

The definite improvement is the eliminated large per-graph workspace and the
new compact exact implementation. The batch remains incomplete, so it does not
yet justify changing the working CPU producer.

## Commands and retained evidence

```powershell
g++ -O3 -mpopcnt -std=c++20 experiments/proto/layer_gpu_f4_nodp_cpu_gate.cpp -o build/layer_gpu_f4_nodp_cpu_gate.exe
python experiments/proto/layer_gpu_f4_nodp_probe.py --C 4 --input data/logs/layer-direct-gate-20260905-fixtures-v2/c4.L4.txt --oracle data/logs/c6-direct-route-20260905/c4-l4-parallel8-gate.log --cpu-gate build/layer_gpu_f4_nodp_cpu_gate.exe --limit 26 --execute-tiny-gate --output-dir data/logs/c6-direct-route-20260905/gpu-nodp-c4-26
python experiments/proto/layer_gpu_f4_nodp_probe.py --C 5 --input data/logs/layer-direct-gate-20260905-fixtures-v2/c5.L4.txt --oracle data/logs/c6-direct-route-20260905/c5-l4-parallel24-gate.log --cpu-gate build/layer_gpu_f4_nodp_cpu_gate.exe --limit 4 --execute-tiny-gate --output-dir data/logs/c6-direct-route-20260905/gpu-nodp-c5-4
python experiments/proto/layer_gpu_f4_nodp_probe.py --C 6 --input data/logs/c6-direct-route-20260905/l4-sample.txt --oracle data/logs/c6-direct-route-20260905/l4-parallel24-1024.log --cpu-gate build/layer_gpu_f4_nodp_cpu_gate.exe --limit 1 --execute-tiny-gate --output-dir data/logs/c6-direct-route-20260905/gpu-nodp-c6-1
python experiments/proto/layer_gpu_f4_nodp_probe.py --C 6 --input data/logs/c6-direct-route-20260905/l4-sample.txt --oracle data/logs/c6-direct-route-20260905/l4-parallel24-1024.log --limit 2688 --occupancy-one-batch --seed 20260905 --output-dir data/logs/c6-direct-route-20260905/gpu-nodp-c6-occupancy-one-batch
```

All commands exited0. The tiny driver was then extended to admit only the one
explicit fixed occupancy mode; the core and compiled device-source SHA were
identical for all four GPU runs. Each tiny directory retains the actual new CPU
gate command/stdout/stderr, all per-row GPU comparisons, raw markers and summary.

| Frozen source or artifact | SHA-256 |
| --- | --- |
| `layer_gpu_f4_nodp_core.h` | `B3E38472159813BA7A0CC587487B0040AF3C01E191935D12FD542C063F5BEF96` |
| `layer_gpu_f4_nodp_cpu_gate.cpp` | `52F7A144EB2BC61FF93A8748FB564D25A3E1ABA763EAB0CAEA8EC912CD72ECD3` |
| Final `layer_gpu_f4_nodp_probe.py` | `E112F4D6654E3A0F939640F378EC550F29ED7BB949744070C7D2954C5BD2199F` |
| CPU gate binary | `C774085DA6A16663BFEC4F9DC68978F0688C8059B5FC421D13BC5432A83848B0` |
| All four compiled device sources | `07AF9FC21D3C036AA2A85293998693754852D70B8EAFC2599F5FA81B65DFFADE` |
| C4 tiny summary | `18FF340E27FF6E56A93CB822EBA4A1D453C9559216E6DB4823DD9E3B0B8E2480` |
| C5 tiny summary | `5F738970D8850B18C66CCB0ABD182BDE0C58C305DB650BCC275297CA408B7395` |
| C6 tiny summary | `34A86364314CC1F232F5765810BBFE8580326066C9225C5F77998001B2C1164E` |
| Occupancy `summary.json` | `B9B8B0F3A453B0D2C1511DE9226CA10CBD2B5DB3E1C23CC143825FDC2C6C9691` |
| Occupancy `rows.json` | `35DB31A27985FD31A1FAEF3560E577AD04C01E2088711BFA0E64BBAD7726E1D5` |
| Occupancy `events.jsonl` | `93A7A3DBB33C267664AAED36D22999CD557D547EC86C6466BA057062675B9D8C` |

The occupancy input/oracle SHA values are
`38F4B8EF65FA4BEEF7FB05995E2A64B7096162A5E1212A0388AF5A9E195CA99C`
and `8B3DD7335033F834631AB10C930807E4ADEB0C3CE4B8571145737A6EEFE80F1A`.
They were rehashed after the run. `git diff --check` passed and no probe process
remained. A complete C5 counting/factorization promotion gate was not run:
this is an isolated, explicitly bounded research prototype, not a promoted
production backend.
