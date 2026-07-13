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

Superseded prototypes and diagnostic kernels live in `experiments/legacy/`.
They are not built by the standard scripts.
