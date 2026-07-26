# C=6 layer-DP runbook (handoff document)

Date: 2026-07-27.  This is the operational handoff for completing the
independent verification of Pettersen's N(6) via the global
row-incremental layer DP.  Everything below is measured or exact unless
tagged [estimate].  Read `../2026-07-26/layer-dp-permanent-profile.md`
first for the mathematics; this file is about *running* it.

## 1. Where the project stands

- Recursion, normalization, canonical form: verified exactly at
  C=2,3,4,5 (all 355 C=5 triples against an independent oracle; every
  differential gate green single- and multi-threaded).
- Exact new constants: M_3 = 12,324,872 (layer-3 orbit mass
  566,455,903,200); 2->3 emissions 2,605,194,602 (matches the
  independent 2026-07-21 measurement digit for digit).
- Measured: 3->4 total = 2.136e12 (20k-parent fan sample, +-1%);
  M_4 = 9.0e8, 95% [8.87e8, 9.20e8] (hash-window ZTNB, C=5-calibrated).
- Budget: ~4.5e12 emissions total; 145.7 ns/emission wall on 24
  threads (v5.1); peak dictionary ~59 GB at cap 1.3e9 (machine:
  32 threads / 125.7 GB).
- The 2026-07-26 15-agent audit fixed all confirmed overflow/ceiling
  defects; lock-free design and u64-safety proof for layers 2..5
  survived adversarial verification (final transition and contraction
  use u128 / external exact arithmetic; no CRT anywhere).
- NOT yet implemented: checkpoint/restart, layer-5 anchored canonizer,
  the external N(6) summation script, the T1-T10 battery.  See §5.

## 2. The engine

`experiments/proto/layer_dp_gate.cpp` — single file, all modes.

Build (MinGW g++ 13.2 on the workstation):

```text
g++ -O2 -march=native -std=c++17 -fopenmp -o layer_dp_gate.exe \
    layer_dp_gate.cpp -lpsapi
```

Modes (all composable unless stated):

| flags | purpose |
|---|---|
| `C` (2..6) | full DP through layer C + contraction + built-in N checks (C<=5) |
| `--threads N --caps c2,...,cC` | parallel fixed-capacity mode; caps REQUIRED, include 0.1-1% hole slack |
| `--ref file.csv` | diff all (m, ell, F) triples vs a reference CSV (C=5: `docs/expert/2026-07-21/native_c5_response_quotient_triples.csv`) |
| `--rank` | mod-p ranks of completion operators U_L (two primes) |
| `--invariance N` / `--scan-check N` | canonize(gx)==canonize(x); orbit-membership + direct-stabilizer referee |
| `--layer-mass L` | full-scan orbit mass of layer L (C=5 L=4 must give 62,185,328) |
| `--stop-after L [--fan-sample N]` | build to layer L only + count-only fan sample (in-run mass gates: layer 2 = 20,338,525 at C=6, 165,744 at C=5) |
| `--probe L n` | bounded transition probe on n strided parents |
| `--m4probe K W CAP` | hash-window distinct sampling of layer 4 (K parents, keep 2^-W) |
| `--wlseed` | pair-profile seeded canonizer (default OFF after A/B; candidate for 4->5/5->6 only) |
| `--dump f.csv` | per-class (m, ell, F) exact dump (REQUIRED at C=6: in-engine N is disabled by design there) |

Gate ladder to re-run after ANY engine change (all must be green):

```text
layer_dp_gate 2 --scan-check 500
layer_dp_gate 3 --scan-check 1000
layer_dp_gate 4 --threads 8 --caps 60,120,60 --scan-check 3000
layer_dp_gate 5 --threads 8 --caps 200,20000,20000,600 \
    --ref ../../docs/expert/2026-07-21/native_c5_response_quotient_triples.csv \
    --rank --invariance 10000 --scan-check 800
```

Expected C=5: states [1,107,16150,17120,355], emissions
[440192, 2324325, 11957632, 462403], N(5) exact, 355/355 triples,
rank chain 355/353/107/1 (both primes), 0 failures everywhere.

## 3. Staged production architecture (the run itself)

Never run C=6 as one monolith.  Stages, each a separate process reading
the previous stage's verified dump (serialization is part of the
checkpoint work item, §5.2):

- **S0 layers 1-3** (~10 min, 0.7 GB): `6 --threads 24 --caps
  2000,14000000,...` + in-run gates (772 children / layer-2 mass
  20,338,525 / 2->3 emissions 2,605,194,602 / M_3 = 12,324,872).
  Serialize layer 3.
- **S1 3->4** (~1.0-3.5 days depending on §5 optimizations, ~59 GB):
  layer-4 cap 1.30e9.  Checkpoint every ~40 min (17-34 s each).
  Running emission counter must track toward 2.136e12 (+-1%);
  background sampled re-canonization audit.  Serialize layer 4
  (~34 GB) + record exact emission total and hole count.
- **S2 4->5** (~1-3 days, cap [estimate] 1.5e8 -> verify with the §5.4
  calibration first): same checkpoint discipline.  Serialize layer 5.
- **S3 5->6 + contraction** (hours): fan is exactly 64/state; T
  accumulates in per-thread u128; final stab via full scan (parallel);
  per-class CSV dump of all 63,199 (m, ell, F).
- **S4 external exact sum** (minutes, Python): N(6) = sum m*ell*F^2
  over the dump; cross-checks BEFORE looking at the total: F values at
  the two known classes (F6(G1) = 6,986,348,258,918,400, F6(G2) =
  7,053,808,087,203,840), sum m*ell = 924^6 = 6.23e17 exact, the two
  defect-2 checksum relations if the C=6 conjecture is confirmed
  (`nullspace-defect2-c5.md`).  Then compare N(6) against Pettersen's
  38296278920738107863746324732012492486187417600000.
- **Dress rehearsal before S1**: run the entire S1-S4 pipeline on a 1%
  strided parent subset (~2 h), including one deliberate mid-S1 kill
  and resume.  The numbers are meaningless; the mechanics are not.

Risk envelope once this is in place: any crash costs <= ~40 min.

## 4. Decision points that need the owner (not the engineer)

- Firing S1 (the multi-day resource commitment).
- Any change to STATUS.md's authorized-scope statements.
- Accepting N(6) agreement/disagreement as a project conclusion.

## 5. Pre-flight work program (order matters; ~2-3 working days)

1. **Layer-5 anchored canonizer** (0.5-1 day, certain ~3.6-4.8x on the
   53% budget bucket): design and measured feasibility in
   `layer5-anchor-design.md`.  The (C-1)-row anchored form must replace
   the generic search for that whole level atomically; re-run the §2
   gate ladder (C=5's 3->4 exercises the (C-1)-row path).  Optional
   follow-up: the 4-row analog (1 more day, may return "not worth it";
   details in the same file).
2. **Checkpoint/restart** (~19 h incl. kill-loop validation at C=5):
   patch-level spec in `checkpoint-restart-spec.md` (barrier-quiesce at
   parent-chunk boundaries, exact, versioned dumps, resume rebuilds the
   table from keys).
3. **4x M_4 probe** (~19 min): `--m4probe 58400 6 60000000` after an S0
   rebuild; converts M_4 into a measurement and finalizes the S1 cap.
4. **4->5 calibration** (bounded, ~1 h): extend a strided sample of
   REAL layer-4 states (built from sampled layer-3 parents) and measure
   the 4->5 table fan directly — the current 2.4e12 figure is the
   budget's only calibrated-estimate leg (53%).
5. **T1-T10 battery** (one day, specified in
   `preflight-test-battery.md`): includes the 1% dress rehearsal,
   memory-envelope test, and the integrity-sentinel checks.
6. Production decision (owner sign-off, §4).

## 6. Known warts and gotchas

- `data/logs/**` is gitignored: run logs referenced by the docs exist
  only on the workstation.  Copy them elsewhere if the machine is at
  risk.
- Binaries are not committed (`*.exe` ignored); rebuild from source.
- The engine's in-run `N(6)` is intentionally disabled (per-term u128
  overflow); anyone "fixing" that re-introduces a corrupted total.
- Fixed-capacity runs abort (fail-stop, by design) if
  distinct+holes > cap; always provision slack; holes vary run to run.
- Strided samplers are estimator paths only; the exact DP never
  strides.
- `--wlseed` changes the canonical form; never mix seeded and unseeded
  dumps of the same layer.
- The m4probe/fan estimates assume the 2026-07-26 measured profile; if
  the engine's emission semantics ever change, re-derive from
  `layer-dp-permanent-profile.md` §4-5.

## 7. Document map

- Mathematics + all measurements: `../2026-07-26/layer-dp-permanent-profile.md`
- Corrected-recursion verification scripts: `../2026-07-26/*.py`
- Certified C=5 null vectors: `../2026-07-26/c5_wvecs.txt`, `c5_kappa_table.csv`
- Agent design reports (this directory): layer5-anchor-design,
  checkpoint-restart-spec, preflight-test-battery,
  m4-statistics-hardening, nullspace-defect2-c5
- Historical context: `../2026-07-25/` (erratum), `../2026-07-20/`,
  `../2026-07-21/` (reference triples), `docs/methods/reverse-gluing.md`
