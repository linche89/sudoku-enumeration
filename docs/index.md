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
- `../fj9/reproduction.md` — complete FJ05 9x9 reproduction.

## Current mathematics

- `math/c6-current-bottleneck.md` — audited C=6 mathematical frontier.
- `math/og2-band-kernel-lowrank.md` — separate formalized transfer-kernel
  question; it is not the immediate implementation objective.

## Dated evidence

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

## History

Files under `history/` explain how the project arrived here. Their statements,
commands, and source paths may be obsolete and must not override `STATUS.md`.

## Manuscript

- `../paper/c5-orbit-factorization/` — LaTeX paper on the fast, reproducible
  C=5 orbit/factorization computation.  Its author block and archival DOI are
  intentionally left for the user to finalize before submission.
