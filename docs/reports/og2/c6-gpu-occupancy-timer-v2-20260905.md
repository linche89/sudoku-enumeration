# GPU occupancy timer v2: calibration passed, one batch aborted cleanly

Date:2026-09-05. The synchronized timer preflight passed. Exactly one fixed
2,688-graph F4 batch then launched; all2,688 graphs reached the unchanged local
clock cap and returned zero, with no closed values. No retry, larger cap, CPU
oracle recalculation, production change, catalogue read, or N(6) computation
was performed. This is not a GPU impossibility result or a completed-throughput
measurement.

## Isolated change and bounds

The only new prototype is
`experiments/proto/layer_gpu_f4_occupancy_probe_v2.py`. It imports the unchanged,
SHA-pinned `device_source()` from the original occupancy prototype and the
unchanged CUDA driver wrapper. The base rooted-F4 arithmetic, search tree,
frontier, work cap and device guard remain unchanged. The old source and
`c6-gpu-rooted4-probe-20260905.md` were preserved.

The batch uses84 blocks of32 threads, matching the84 reported SMs in count.
This is nominally one warp per SM, **not measured full hardware occupancy**.
Each thread owns one graph workspace. Fixed-seed draws with replacement from
the retained1,024-sample text produced2,688 draws and942 distinct sample indices,
not2,688 new native states. No CPU factorization was rerun.

The guard was reviewed before launch:

- One timestamp marker sets a common absolute global-timer budget of700,000,000
  nanoseconds before the computing kernel. Host synchronization and late block
  starts consume this same budget; no later block receives a fresh deadline.
- The retained kernel checks this deadline on entry and every128 forward-DP
  steps or backward traversal iterations. Nonchecked inner work is bounded by
  the fixed12-vertex graph and rollback stack. This is a cooperative deadline
  with bounded polling work, not an unconditional OS/scheduler latency theorem.
- The unchanged local cap is1,706,250,000 ticks, calculated as the reported
  2,625,000kHz times650. Reported nominal frequency is not a fixed actual rate.
- Per graph:2,000,000 DP/node cap,100,000 frontier entries,531,441 ternary marks.
- Explicit GPU allocations total3,579,106,944bytes, below4GiB, with the retained
  additional1GiB free-memory reserve check. CUDA context/driver memory is separate.
- Host hard bounds are120seconds and1GiB RSS. The code can issue only one F4
  launch, and every nonzero status must leave the value zero.

## Timer correction and raw calibration evidence

A warm-up timestamp marker is launched, synchronized with `cuCtxSynchronize`,
and read back before measured endpoints are taken. Each subsequent marker
retains host times before launch, after launch, after synchronization, and after
readback, plus the raw device timestamp. The preflight uses fixed20ms and40ms
host sleeps; there is no adaptive retry.

If markers A and B occur inside host brackets `[a0,a1]` and `[b0,b1]`, their
nanosecond device difference should lie in `[b0-a1,b1-a0]`. The finite predicate
uses100,000ns tolerance, requires positive10–250ms crossed host intervals,
host-bracket uncertainty at most2ms and10% of the midpoint, and a device/host
midpoint ratio within0.95–1.05. Six offline cases accepted the consistent
nanosecond example and rejected microsecond/millisecond scaling, reversal,
excessive first-launch uncertainty, and excessive host gap. No GPU was used
by those predicate tests.

Actual warm-up launch-to-sync interval:26.723400ms. It was excluded from both
calibration intervals. This demonstrates why excluding startup latency matters
in this run; the earlier failed run retained no raw data, so its exact cause
remains unproved.

| Measured interval | Device difference | Crossed host bracket | Host uncertainty | Device / midpoint |
| --- | ---: | --- | ---: | ---: |
| A→B |20,356,960ns |20,318,400–20,406,600ns |88,200ns |0.999727931246 |
| B→C |40,674,144ns |40,581,500–40,708,800ns |127,300ns |1.000713344643 |

Both passed every predeclared criterion. Actual host sleeps were20,219,700ns
and40,439,100ns. All absolute timestamps, not merely these differences, remain
in both the event JSONL and final summary JSON.

The batch deadline marker recorded global timer1,788,597,597,755,861,504ns;
the shared absolute deadline was1,788,597,598,455,861,504ns.

## One-batch result

| Quantity | Actual result |
| --- | ---: |
| F4 computing launches |1 |
| Timestamp-marker launches |5 |
| Closed exact graphs |0 |
| Aborted graphs, all status3(local clock) |2,688 |
| Returned nonzero values |0 |
| Kernel event time |593.030640ms |
| Transfer, workspace initialization, launch and result readback |0.595013s |
| Compilation |0.188709s |
| Explicit allocation |0.007472s |
| Cleanup |0.041213s |
| Total host time including cleanup |1.668626s |
| Peak host RSS |179,810,304bytes |

All threads reached backward traversal. Their partial DFS-node diagnostics
ranged20,674–22,150, sum57,475,524; forward DP steps ranged15,366–111,204,
sum154,321,896. Returned local elapsed ticks ranged1,706,287,519–1,710,827,647,
consistent with local-cap refusals plus finite polling work. Partial leaves
ranged3,667–7,858. These are diagnostic search counts, not accepted F4 values.

The v2 driver checks each closed triple `(F4,leaves,nodes)` against its retained
CPU oracle and rejects any nonzero aborted value. In this batch the closed
comparison is vacuous because there are no closed graphs; it establishes no
new positive numerical F4 certificate. A separate bounded post-read confirmed
all2,688 statuses were3 and all2,688 values were zero. All raw per-draw records,
sample indices and expected CPU triples are retained.

The local clock guard fired before the shared700ms deadline. The run does not
show that a larger cap would or would not complete these graphs, and no larger
cap was attempted. It provides no measured closed-graph throughput or justified
reason to replace the running CPU producer.

## Exact commands and provenance

```powershell
python experiments/proto/layer_gpu_f4_occupancy_probe_v2.py --self-test-preflight --output-dir data/logs/c6-direct-route-20260905/gpu-occupancy-v2-preflight-tests
python experiments/proto/layer_gpu_f4_occupancy_probe_v2.py --input data/logs/c6-direct-route-20260905/l4-sample.txt --oracle data/logs/c6-direct-route-20260905/l4-parallel24-1024.log --execute-one-batch --seed 20260905 --global-ms 700 --output-dir data/logs/c6-direct-route-20260905/gpu-occupancy-v2-one-batch
```

Both commands exited0. Probe source SHA-256:

```text
75DEC4C200E8CF9AD47C6052FB1F06663EE551858DEF74EB9D10E4ACC4902D8B
```

Retained logs under `data/logs/c6-direct-route-20260905/`:

| Artifact | SHA-256 |
| --- | --- |
| `gpu-occupancy-v2-preflight-tests/summary.json` | `EAD7891A655ABD68A060299D953BC2548038B6641383420C653F289F946CACA5` |
| `gpu-occupancy-v2-one-batch/summary.json` | `C36DC5D1385402FB4CC4065A65C34EF0D0E79F61FBB95CED39080953A125DA12` |
| `gpu-occupancy-v2-one-batch/events.jsonl` | `41704BC38C61DDC6F6700A64EA01493E6A5DD60F7BCD4C06C25334C806C75FED` |
| `gpu-occupancy-v2-one-batch/draw-results.json` | `83EAC7DB1355C87850C47A797E674F113AD2E33AD73ADE044F99522897D21727` |

Compiled device-source SHA is
`343C2A289FE87E2A3A16869542FABCA552B4F99564A241E4F412EA7BD37D5470`.
The three imported/base sources were checked before launch and independently
rehashed afterward, agreeing with their unchanged earlier SHA values:

```text
occupancy v1: 0347C4863AF68786EBF5245873AEF7F655A1AA46C89777E54C095254184CBA73
base driver:  775908D26757D1A77C27013742AC8B09FD542C3CC543C1C0F37C305585B73520
base device:  0EDB6BCF9F8C50D52648A7F0366B28AC4E6C7EF4D507DC669CC3321D8EB08BF9
sample text:  38F4B8EF65FA4BEEF7FB05995E2A64B7096162A5E1212A0388AF5A9E195CA99C
CPU oracle:   8B3DD7335033F834631AB10C930807E4ADEB0C3CE4B8571145737A6EEFE80F1A
```

The cleanup path executed and the process terminated; no v2 GPU probe process
remained. `git diff --check` passed. No full counting gate was rerun because no
counting kernel or production integration changed. The task stopped after the
single authorized batch.
