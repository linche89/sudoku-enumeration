# Source Map

## Primary programs

- `factorization_orbit.cpp` — primary exact 2xC factorization/orbit engine.
- `future_twin.hpp` — optional exact future-twin backend used by
  `factorization_orbit.cpp`; it is never selected by default.
- `multiset_q.cpp` — independent transfer-kernel research engine.
- `multiset_fast.cpp`, `multiset_par.cpp`, `multiset_c6.cpp` — independent
  exact validators and bounded-memory variants.
- `canon_refine2.cpp` — standalone canonicalization validator.
- `main.cpp`, `reduce_main.cpp` — completed FJ05 9x9 reproduction.

## Stable supporting tools

- `combinatorial*.cpp`, `reduce44.cpp` — verified 9x9 combinatorial routes.
- `sudoku_rc.cpp` — independent small-C 2xC oracle.
- `bench.cpp` — FJ9 benchmark helper.

## Shared headers

- `big.hpp` — exact integer support for OG-2 engines.
- `band.hpp`, `count.hpp`, `reduce.hpp` — FJ9 reproduction.
- `mpq_sigkey.hpp` — fixed-buffer signature key used by `multiset_q.cpp`.
- `layer_two_missing_prefix_canon.h` — compatible maximum-missing-edge
  canonical-search helper for the separately gated reverse-F5 engine; it
  preserves native keys/stabilizers and falls back outside its exact domain.

Superseded code lives in `experiments/legacy/`; bounded decision engines and
pre-promotion candidates live in `experiments/proto/`.  The layer-DP candidate
there has its own build/gate scripts and is called by the full repository
verification, but it remains outside `src/` until its C=6 preflight is
complete.
