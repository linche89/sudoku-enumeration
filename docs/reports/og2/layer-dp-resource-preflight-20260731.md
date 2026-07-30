# Layer-DP resource preflight and in-place resume

Date: 2026-07-31

## Scope

This is the next bounded step after commit `58ab92b`.  It adds a read-only
capacity/resource model, removes the checkpoint child's duplicate resume
payload, and enforces one large C=6 transition per process.  No C=6 layer 3,
4, 5, or 6 was generated and no active checkpoint was modified.

The capacities used below are planning inputs from the prior preflight:

```text
layer 2 = 2,000
layer 3 = 14,000,000
layer 4 = 1,350,000,000
layer 5 = 250,000,000
layer 6 = 100,000
threads = 24
```

The layer-4 and layer-5 capacities are not yet accepted bounds.  Stronger
`M_4` and unbiased 4-to-5 calibration remain mandatory.

## Initial obstruction

The first exact accounting pass modeled the then-current resume path:

1. read the compact child checkpoint into `CkptImage`;
2. allocate a full fixed-capacity child;
3. copy the image into the child;
4. rebuild the child hash table;
5. release the original image.

For transition 3-to-4 at a 1.35-billion cap this gave:

```text
parent resident       =   0.594 GiB
child resident        =  61.262 GiB
extra compact image   =  45.262 GiB
resume-load peak      = 107.119 GiB
required with margin  = 117.831 GiB
host total            = 125.650 GiB [OK]
host available        = about 90.98 GiB [FAIL]
```

Thus the old recovery implementation failed the launch gate even though
steady-state computation fit.

## In-place resume correction

`ck_read_file` now accepts the target fixed capacity.  It reserves that
capacity before reading the compact payload.  `layer_install` then moves the
three payload vectors into the live child and extends their logical sizes
within the reserved buffers before allocating the hash table.

Final-layer `u128` values are moved in the same way.  A resumed fixed child
hard-fails if the cap reservation is absent, so the resource model cannot
silently regress to the duplicate-copy path.

This changes no serialized bytes and retains checkpoint format version 2.
Older v2 files remain readable when their configuration fingerprint matches.

## Resource model

For each staged transition, `--resource-preflight L` counts:

- parent and child `State`, `T`, and stabilizer arrays at fixed cap;
- the exact power-of-two open-addressing tables;
- per-thread raw-child caches;
- global and per-thread final-layer `u128` arrays;
- cap-reserved in-place resume;
- two retained physical generations for every earlier completed stage;
- three images for the active stage: `.a`, `.b`, and the full `.tmp` being
  written before atomic replacement.

Margins are:

```text
RAM  = max(8 GiB, 10% of modeled peak)
disk = max(16 GiB, 10% of modeled peak)
```

The command is read-only and queries the checkpoint base's actual volume:

```powershell
$caps = '2000,14000000,1350000000,250000000,100000'
.\build\layer_dp_gate.exe 6 --threads 24 --caps $caps `
  --checkpoint E:\Code\sudoku_FJ\data\logs\resource-plan-c6 0 `
  --resource-preflight 3
```

## Final capacity-worst-case plan

The host query reported 125.650 GiB total RAM, approximately 90.9 GiB
available RAM, and 1,827.837 GiB free on the selected E: volume.

| transition | runtime peak | resume peak | RAM + margin | retained disk peak | disk + margin | verdict |
|---|---:|---:|---:|---:|---:|---|
| 3->4 | 61.898 GiB | 61.857 GiB | 69.898 GiB | 136.726 GiB | 152.726 GiB | pass |
| 4->5 | 71.685 GiB | 71.644 GiB | 79.685 GiB | 116.609 GiB | 132.609 GiB | pass |
| 5->6 | 10.465 GiB | 10.388 GiB | 18.465 GiB | 108.242 GiB | 124.242 GiB | pass |

The 3-to-4 resume peak fell by 45.262 GiB.  The dominant RAM stage under
these caps is now 4-to-5, while the dominant disk event is the 3-to-4
A/B/tmp write.

These are conservative cap calculations, not an allocation benchmark.
Available RAM is checked again at launch because it is time-dependent.

## Staged-process enforcement

Large C=6 work now requires:

```text
loaded/resumed layer 3 -> stop after layer 4
loaded/resumed layer 4 -> stop after layer 5
loaded/resumed layer 5 -> final CSV
```

An acknowledged monolithic layer-4 invocation is rejected.  Each accepted
large stage runs the resource preflight automatically before loading or
allocating its layer.

The staged C=5 differential found and fixed one operational bug: a
`--load-layer 3 --stop-after 4` process used to test the absent layer-2 mass
and fail after completing the requested transition.  Staged reports now mark
unloaded lower layers explicitly and validate only resident layers.

## Verification

The dedicated gate now includes:

- a read-only force-wide resource dry run;
- an assertion that no checkpoint file is created;
- C=5 staged layer 3-to-4 completion and finalized layer-4 snapshot;
- cap-reserved narrow and wide kill/resume;
- a deterministic locked-generation replacement retry;
- acknowledged C=6 monolith refusal.

### Windows replacement fault

An initial 20-cycle run caught a real intermittent failure at cycle 5 while
replacing generation `.a`.  Both existing generations remained valid, the
new `.a.tmp` remained available for forensics, `.a` was not a snapshot hard
link, and no layer-DP writer remained.  This supports a transient Windows
sharing/access fault; it does not identify the external process holding the
handle.

Atomic checkpoint and snapshot-link replacement now retries only these
Windows errors:

```text
ERROR_ACCESS_DENIED
ERROR_SHARING_VIOLATION
ERROR_LOCK_VIOLATION
ERROR_BUSY
ERROR_USER_MAPPED_FILE
```

The retry interval is 100 ms and the hard limit is five seconds.  Any other
error fails immediately, and exhaustion still fails closed.  The exact
forensic checkpoint that had failed was resumed with the corrected binary
and reproduced the golden 355-class dump.

The regression gate makes this deterministic: it opens both generation files
without delete sharing, starts resume, waits until the executable reports
`replace temporarily blocked (winerr=5)`, releases the handles, and requires
the completed dump to match the golden SHA-256.

The full 20-cycle gate then passed:

```text
LAYER-DP CHECKPOINT AND EXACTNESS GATES PASSED
```

The repository-wide gate also passed after rebuilding every active target:

```text
OG-2 short verification passed
LAYER-DP CHECKPOINT AND EXACTNESS GATES PASSED
N(2), N(3), N(4), N(5): exact
FJ9 full and independent routes: exact
C=6 checkpoint read-only: exact hash preserved
ALL REPOSITORY CHECKS PASSED
```

The active C=6 checkpoint remained
`FE8B68DE6C15592848D7CF69BF43928C000F0B2EE59FC263A094BCA2D376A865`.

## Decision

Resource accounting is now an executable launch gate rather than a prose
estimate, and the avoidable recovery-memory obstruction is removed.

This does not validate the proposed capacities or authorize S1.  The next
mathematical/measurement tasks remain stronger `M_4` estimation and an
unbiased 4-to-5 fan/canonicalization calibration, followed by a guarded
allocation/restart rehearsal.
