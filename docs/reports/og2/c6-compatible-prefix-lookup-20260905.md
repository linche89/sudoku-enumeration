# Compatible-prefix extraction and same-table C6 lookup gate

Date: 2026-09-05. The compatible canonical-search optimization passes its
isolated exact gates and a complete 512-source query differential against the
real C6 native index. It is not installed in either released computing
engine. No numerical C6 F4 values were used and no F5 or N(6) was computed.

## Implementation boundary

`experiments/proto/layer_two_missing_prefix_canon.h` extracts only the
maximum-missing-edge prefix from the earlier isolated geometry probe. It has
no geometry inventory, transporter trie, rekeyed index or alternate native
representative. The proof is Lemma A of
`../../math/two-missing-canonical-prefix.md`.

The checked dispatch requires C=2..6, N2C=2C, the unseeded native convention,
valid 0/2/3 fields, zero padding, exactly C-2 occupied boxes per mask, and
four missing symbols plus C-2 symbols on each side in every box. It retains
all maximum-edge ties, both side flips, and every later box choice. Other
native cases use the unchanged native canonicalizer; a separate strict
entry refuses inputs outside the optimized domain. In particular C5 L4
still uses the original one-missing anchor.

`layer_reverse_f5_prefix_bench.cpp` is a separate benchmark translation unit.
The existing native engine and catalogue loader are included before a
narrowly scoped `canonize` macro substitutes the checked dispatch in the
reverse core. The production core and producer files are not edited.

`layer_reverse_f5_prefix_compare.cpp` includes the original reverse core and
the prefix version in distinct namespaces. It loads one immutable native
catalogue, compares every selected residual key/stabilizer/ID before timing,
and then uses each core's explicitly value-free `lookup_only` return type.
No fallback or prefix call is inferred: the benchmark checks the actual
dispatch totals. An independent read-only source review found the extracted
preconditions and macro/include scope consistent with the proof.

## Build and small exact gates

Three new binaries were compiled without overwriting an existing executable:

```powershell
g++ -O3 -mpopcnt -std=c++20 -fopenmp -Wall -Wextra `
  experiments/proto/layer_two_missing_prefix_gate.cpp `
  -o build/layer_two_missing_prefix_gate_20260905.exe -lpsapi
g++ -O3 -mpopcnt -std=c++20 -fopenmp -Wall -Wextra `
  experiments/proto/layer_reverse_f5_prefix_bench.cpp `
  -o build/layer_reverse_f5_prefix_bench_20260905.exe -lbcrypt -lpsapi
g++ -O3 -mpopcnt -std=c++20 -fopenmp -Wall -Wextra `
  experiments/proto/layer_reverse_f5_prefix_compare.cpp `
  -o build/layer_reverse_f5_prefix_compare_20260905.exe -lbcrypt -lpsapi
```

All three builds exited zero without warnings. Reproduction should use new
unused binary paths if these files already exist. Every small numerical gate
used one computing thread and a 1-GiB limit; the extracted gate used a
115-second internal and 120-second external wall limit. Its retained logs are
under `data/logs/extracted-prefix-gate-84f054088a6643598276ff1edc6b6e33/`.

Exact executed results:

| Domain | Native queries | Coordinate transformations | Result |
|---|---:|---:|---|
| C2 L0 | 1 | 8, full group | Identical keys/stabilizers |
| C3 L1 | 1 | 48, full group | Identical keys/stabilizers |
| C4 L2 | all 23 | 8,832, full group on every state | Identical keys/stabilizers/histogram |
| C5 L3 | all 16,150 | 129,200 | Identical keys/stabilizers/histogram |
| C6 residuals of 64 L5 sources | 314,584 | 1,024 | Identical keys/stabilizers |

The small complete cases also check idempotence and independent full-group
stabilizers (every C2..4 input; eight C5 inputs). Seeded inputs use the native
fallback; the strict entry rejects field 1, outside-domain bits and nonzero
padding. C6 residuals occupy 314,117 distinct native classes and have query
stabilizer histogram `1:314223,2:360,4:1`.

The complete C5 L3->L4 coefficient chain exercises the optimized branch, not
merely the different one-missing branch of a C5 F5 check:

```powershell
build/layer_two_missing_prefix_gate_20260905.exe 5 4 `
  input=data/logs/layer-direct-gate-20260905-fixtures-v2/c5.L4.txt `
  preload=data/logs/layer-direct-gate-20260905-fixtures-v2/c5.L3.txt `
  limit=17120 maxqueries=5000000 maxseconds=115
```

```text
predecessors=16150 targets=17120
labelled_matchings=3375557 weak_queries=2935081
all_term_keys_stabs_coefficients_equal=YES all_F4_equal=YES
predecessor_transforms=129200
sumF4=3972941184 sumOrbitF4=14365876248576
seconds=11.069061 peak_rss_bytes=9121792
dispatch=2951231 fallback=17120
```

Every raw residual term keeps exactly its old native key and stabilizer;
its matching multiplicity is unchanged. Thus the complete grouped operator
coefficients are unchanged, not only their contraction with the supplied
F3 vector. All 17,120 resulting F4 values agree with the independent closed
reference. The explicit fallback checks cover every one-missing target.

The separate reverse-prefix benchmark checked all 355 exact C5 F5 values
from a newly exported native L4 catalogue and fresh expected-F4 text:

```text
sources=355 labelled_matchings=425652 weak_residuals=270870
hits=270870 misses=0 checked=355
lookup_checksum=6574795732546654280
query_wall_s=0.378418300 total_wall_s=0.415013900
peak_rss_bytes=7938048
[OK] COMPLETE_C5_ALL355_EXACT_F5
```

That C5 path correctly uses native fallback because its residual degree is
C-1. This does not substitute for the preceding two-missing coefficient gate.
Logs are under
`data/logs/extracted-prefix-reverse-c5-80c2160776244ff78d3f769b5c290d31/`.
The same-table comparison program's additional C5 smoke checked all 270,870
query IDs and reported exactly 270,870 native fallbacks and zero prefix calls.

## Real C6 same-table differential

The coordinating agent released the heavy-memory slot after the fourth shared-F4 window.
The GPU worker also held its kernel launch until this test exited. The
remaining independent namespace audit used less than 1 GiB; it did not
perform numerical counting or load a native catalogue.

The executed command was wrapped by `scripts/watch_rss.ps1` with a 55-GiB
working-set bound and three-minute external limit:

```powershell
build/layer_reverse_f5_prefix_compare_20260905.exe 6 `
  catalog=data/checkpoints/layer_dp_c6_s1_prod_20260802.a `
  input=data/logs/c6-direct-route-20260905/l5-sample-512.txt `
  limit=512 maxrecords=5000000 maxseconds=175 maxrssgib=55 `
  checkpointreadonly repair-l4-support ack-large-c6
```

The original input's full SHA validated as
`ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844`.
All old T fields were discarded; the one support witness was added only in
memory. The same 903,398,603-key index served every phase. The loader and
post-run size/timestamp check were read-only; no checkpoint was rewritten.

```text
index_threads=24
read_s=11.877415300 sha256_s=12.162074700
index_s=8.302757300 catalogue_wall_s=32.504287200
sources=512 labelled_matchings=2691041 queries=2471101
all_keys_stabilizers_IDs_equal=YES misses=0
untimed_differential_seconds=14.224598300
```

The untimed gate recomputed both native and prefix keys and exact stabilizers
for **every** actual residual query, found the same stable native ID in both
cases, and checked that the catalogue's stored stabilizer agreed.

## Timed complete-query comparison

Times below include source conversion, rooted matching selection, residual
enumeration, canonicalization, native lookup and loop overhead. The CPU
columns are sums over workers, not wall times when there are 24 threads.

| Workers | Canonicalizer | Query wall seconds | Canonicalization CPU seconds | Lookup CPU seconds |
|---:|---|---:|---:|---:|
| 1 | Native | 9.110397100 | 6.833245400 | 1.472620000 |
| 1 | Compatible prefix | 7.303558900 | 5.065048900 | 1.468089800 |
| 24 | Compatible prefix | 0.330693900 | 5.411764300 | 1.298964700 |
| 24 | Native | 0.422382100 | 7.644828800 | 1.286450500 |

The same-table observed query speed ratios are about 1.247x at one worker
and 1.277x at 24 workers. Native search nodes were 53,161,937 per pass;
prefix nodes were 43,660,709. Every prefix pass dispatched all 2,471,101
queries to the optimized branch and made zero native fallbacks.

Every timed phase reproduced the same work counts, all hits, and checksum
`11399444842143351229`. Native ran before prefix at one worker; the order
was reversed at 24 workers. This is one finite same-table comparison, not
a randomized repeated timing study or an entire F5 production run.

```text
engine_total_wall_s=63.917486700
external_controller_wall_s=66.2607737
peak_rss_bytes=41133170688
exit=0
source_checkpointreadonly=yes production_T_values_read=no
F5=NOT_COMPUTED N6=NOT_COMPUTED file_writes=none
```

Logs, complete counters and RSS samples:
`data/logs/c6-prefix-shared-table-6b16a3e178a948598509e11985851540/`.
The process exited and released its large allocation before the GPU worker
was notified to resume.

## Scope and reproducibility pins

The measured improvement is a compatible-key **lookup-only** gain. Numerical
F4 memory reads, F5 multiplication/addition, immutable F5 chunks, export and
end-to-end daily scheduling were not measured. The result does not justify
applying 1.277x to total S2 or to the full C6 job. Adoption into the actual
reverse producer still requires a separate full release/recovery gate; the
shared-F4 producer does not need to change.

```text
header layer_two_missing_prefix_canon.h:
450FEFB0FEB62E6E2CA29218F96101E7304FCB10A5C419F7B8CE090D2107082E
gate executable:
2AC2B6497EB9B2AB0BAE54920E0B72BE50BDE9E93E4093EFA9555DDBC8F458F3
prefix reverse executable:
DE62840F2EEC89C8E042E01543D1B2C3F915691B082A212D0DEEE4E2888F58D0
same-table comparison executable:
67C720AB6974B4DE6A8283A9C6B6A9E7292A29A5B1392D84A03C2C03690E8E9B
512-source sample:
B535C873117F8583CA5069A16D2430767DAFC79FCFE6949325ABE9B5F853CE25
```

Both released engines retain their exact original SHA, size and write time:

```text
layer_shared_f4.exe
45A8387A031C3236191C71C28F73773711EEC34B9A628D4E715B2FE5F8EDD4FC
1587447 bytes; 2026-09-05T06:25:03.7380234Z
layer_reverse_f5.exe
C7BDBC4DEA0FE8497787D7DB127A9B718D591BCA5F59B9394C64F6E11F295761
527456 bytes; 2026-09-05T06:49:46.9352003Z
```
