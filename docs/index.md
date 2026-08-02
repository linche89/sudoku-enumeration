# Documentation Index

## Start here

- `../STATUS.md` — the only authoritative current state.
- `../README.md` — repository entry point and commands.
- `runbooks/og2.md` — safe build, verification, and checkpoint workflow.

## Verified methods

- `methods/factorization-orbit.md` — current factorization/orbit algorithm.
- `methods/future-twin.md` — optional exact future-twin recurrence and gates.
- `methods/connectivity-operator.md` — exact symbol-synchronous connectivity
  recurrence and the failed C=5 compression gate for its naive
  double-permanent subset DP.
- `methods/joint-histogram.md` — exact box-order paired-histogram contraction
  through C=4, bounded C=5 frontier data, and the failed scale gate for its
  target-labelled residual operator.
- `methods/reverse-gluing.md` — reverse row-block gluing, exact orbit
  normalization, complete C=2..5 gates, exact C=6 two-row inventory, and the
  failed scale gate for immediate pairwise canonicalization.
- `methods/layer-dp.md` — qualified global row-incremental candidate,
  exact recurrence, canonicalization gates, checkpoint/restart guarantees,
  completed bounded C=6 preflight, and owner-controlled launch boundary.
- `../fj9/reproduction.md` — complete FJ05 9x9 reproduction.

## Current mathematics

- `math/c6-current-bottleneck.md` — audited C=6 mathematical frontier.
- `math/penultimate-layer-burnside.md` — exact native penultimate-layer
  Burnside count, including `M_5(6) = 96,452,755` and small-C differentials.
- `math/og2-band-kernel-lowrank.md` — separate formalized transfer-kernel
  question, completed small-rank gate, fixed-source frontier lower bound, and
  surviving target-only/response-subspace questions.

## Dated evidence

- `reports/og2/layer-dp-c6-bounded-end-to-end-rehearsal-20260802.md`
- `reports/og2/layer-dp-c6-allocation-restart-rehearsal-20260801.md`
- `reports/og2/layer-dp-penultimate-burnside-20260801.md`
- `reports/og2/layer-dp-m4-random-calibration-20260801.md`
- `reports/og2/layer-dp-resource-preflight-20260731.md`
- `reports/og2/layer-dp-s0-random-calibration-20260731.md`
- `reports/og2/layer-dp-checkpoint-gate-20260730.md`
- `reports/og2/source-target-frontier-lower-bound-20260720.md`
- `reports/og2/band-kernel-rank-20260719.md`
- `reports/og2/f4-lookup-coverage-20260719.md`
- `reports/og2/future-tail-later-class-coverage-20260719.md`
- `reports/og2/connectivity-double-permanent-20260719.md`
- `reports/og2/joint-histogram-operator-frontier-20260719.md`
- `reports/og2/joint-histogram-c5-frontier-20260719.md`
- `reports/og2/joint-histogram-c2-c4-20260719.md`
- `reports/og2/reverse-gluing-c6-frontier-probe-20260718.md`
- `reports/og2/reverse-gluing-c4-c5-20260718.md`
- `reports/og2/c6-route-portfolio-20260717.md`
- `reports/og2/future-tail-kernel-table-c6-20260715.md`
- `reports/og2/future-tail-color-symmetry-c6-20260715.md`
- `reports/og2/future-pair-tail-external-c6-20260714.md`
- `reports/og2/future-twin-c6-20260714.md`
- `reports/og2/c6-expert-routes-audit-20260713.md`
- `reports/og2/literature-audit-20260712.md`
- `reports/og2/c5-paper-ablation-20260712.md`
- `reports/og2/factorization-c5-20260712.md`
- `reports/og2/factorization-c6-20260712.md`
- `reports/og2/mpq-c5-calibration-20260704.md`
- `reports/fj9/table71.md`

Reports are append-only evidence. New conclusions belong in `STATUS.md` or a
verified method document, not in an old report.

## Raw expert material

Files under `expert/` preserve questions and responses verbatim for provenance.
They may contain conjectures or proposals that have not been implemented.

- `expert/2026-07-19/c6-mathematical-bottleneck-question.md` — previous request
  for an exact global contraction, a fast implicit band-kernel transform, or a
  relevant lower bound after the then-completed scale gates.
- `expert/2026-07-19/fable_response.md` — raw graph-regression, PSD/Kraus,
  commutant, and signed-permanent suggestions; audited, not authoritative.
- `expert/2026-07-19/gpt5.6_pro_response.md` — raw fixed-source support and
  flattening argument that led to the independently retained certificate.
- `expert/2026-07-20/c6-target-only-global-contraction-question.md` — narrowed
  next question after fixed-source and full-orbital implementations were
  rejected.

## History

Files under `history/` explain how the project arrived here. Their statements,
commands, and source paths may be obsolete and must not override `STATUS.md`.

## Manuscript

- `../paper/c5-orbit-factorization/` — LaTeX paper on the fast, reproducible
  C=5 orbit/factorization computation.  Its author block and archival DOI are
  intentionally left for the user to finalize before submission.
