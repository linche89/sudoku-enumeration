# Sixth bounded shared-F4 C6 window — 2026-09-05

N(6) remains uncomputed. This window added 48,375,000 stable-ID records and
7,639,462 closed graph-representative F4 values. The current prefix is
`[0,156375000)`, with 24,667,321 closed representative values. No native layer
was exported.

## Executed bounded command

The existing guarded shared-F4 controller verified the original source's
physical backup, copied and verified all 4,321 previous committed files,
then launched:

```powershell
& E:\Code\sudoku_FJ\build\layer_shared_f4.exe 6 `
  catalog=E:\Code\sudoku_FJ\data\checkpoints\layer_dp_c6_s1_prod_20260802.a `
  output=E:\Code\sudoku_FJ\data\checkpoints\c6_shared_f4_20260905 `
  chunk=25000 limit=50000000 threads=24 workseconds=1500 maxseconds=1795 `
  maxrssgib=55 checkpointreadonly repair-l4-support
```

The child had a 25-minute soft stop, 1,795-second internal hard bound and
55-GiB working-set bound. The controller's external child limit was 30 minutes.
It stopped normally before exhausting the positive record limit. Engine and
controller exited 0, and the controller completed its after-backup before
reporting `WINDOW_END exit=0`.

The source's old partial T values were discarded. Its full SHA was checked
again by the loader:
`ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844`.
Loading/validation took 32.604065 seconds, including 12.045075 seconds for
payload reading/checking, 12.201027 for SHA, 0.161164 for weight erasure and
8.196450 for indexing. The only support repair was appended in RAM.

Physical backup/hash overhead is outside the child budget. Bounded validation
tasks also ran during this window, so these resource observations are not an
isolated performance benchmark or a new global runtime estimate.

## Exact retained result

```text
resumed prefix / chunks: 108000000 / 4320
resumed closed representatives: 17027859
new IDs / chunks: 48375000 / 1935
new closed representatives: 7639462
new representative checksum modulo 2^64: 10638783660288
current prefix: [0,156375000)
complete ID domain including holes: 903398621
live records in prefix / holes: 156375000 / 0
committed chunks / files including manifest: 6255 / 6256
total bytes: 1878101536
closed representative F4 values: 24667321
representative checksum modulo 2^64: 34408298870016
new computation/commit wall: 1464.604096 seconds
complete engine wall: 1500.277530 seconds
peak working set: 44789915648 bytes
status: INCOMPLETE_RESUMABLE
N6: NOT_COMPUTED
```

Exact aliases retain the distinct native responses. Some representatives are
outside the processed prefix: 59,094,214 processed IDs currently resolve to
closed representatives, while 97,280,786 address future representatives.
Therefore 156,375,000 committed records do not mean that all those native T4
weights are available. This is neither a percentage-complete certificate for
N(6) nor a complete native F4 export.

## Physical copies and independent production audit

The new external directories are:

```text
D:\sudoku_FJ_checkpoint_backups\c6_shared_f4_20260905\
  20260905-173432-23bc6465d3a74b1cb642e7a088e1b34f-before\
  20260905-173432-23bc6465d3a74b1cb642e7a088e1b34f-after\
```

The after-copy contains all 6,256 committed files. The independently
implemented two-pass native bitset audit checked every current file,
header/payload/value convention, alias destination within the closed prefix,
both physical copies, and preservation of all 4,321 previous files. It bound
its predecessor to the window-5 audit SHA, verified both receipt chains, and
reproduced every count and engine-summary field above.

The production audit is `shared-f4-native-bitset-resume-v1`, with status
`PASS`; qualification-replay, protocol-test and snapshot flags are all false.
It took 21.9385034 seconds overall (worker 21.3292 seconds), with worker peak
30,715,904 bytes and parent peak 75,358,208 bytes. Its representative bitset
used 19,546,875 bytes. The worker command retained in the JSON used
`maxseconds=178 maxrssmib=1024` and the pinned audit plan.

```text
namespace manifest SHA256:
848B9DC72452AB389C5BF5424E264AE86C673C7FBA673F5F14FDC845BF4DEB91
before-backup receipt / preceding after-backup receipt SHA256:
927CECD30305BC8A84161983414B0EC87C1CD9F821EEAB1E0FA75A3248984E72
after-backup receipt SHA256:
E54BADC15712081634993B6CF1C70753F32DEBF21F5B7D273951F8456948EFF9
previous window-5 audit SHA256:
93BC52CB7234383405EC4BA8A18FF175E0E49FC478BA136DE136CEDA6C110CA4
independent window-6 production audit SHA256:
E4544DDE15555EA4F47A63F5405C6200119BEF8351EFA688CDBAB5834FE6F0BE
audit worker SHA256:
DD541C9318334DCA8763FEBD66E60B81F51CF35D2887456EEBFD9DD804F7D4C4
```

The audit JSON is
`data/logs/c6-direct-route-20260905/shared-window6-independent-audit.json`;
its complete worker command, file-level hashes and plan remain retained
there. The pinned audit SHA above was rechecked when preparing this report.
The audit reads no full original catalogue and does not independently
reevaluate the numerical F4 values. Its integrity claim applies at the
verified reads, not to permanently immutable or simultaneously locked storage.

Controller log:
`data/logs/c6-direct-route-20260905/shared-production-window6-controller.log`.
Engine stdout, empty stderr and RSS log:
`data/logs/shared-f4-window-20260905-173432-23bc6465d3a74b1cb642e7a088e1b34f/`.

Original sources, all earlier committed chunks and older physical copies
remain preserved. Full alias closure and native F4 export, numerical reverse
F5 and the independently replayed final 63,199-class weighted-square sum
remain prerequisites for N(6). This report adds no new tests or computation;
subsequent release qualification is separate from this window's evidence.
