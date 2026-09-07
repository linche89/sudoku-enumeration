# Publication as sudoku-enumeration

Date: 2026-09-07

## Scope and preservation

The owner selected `https://github.com/linche89/sudoku-enumeration.git` as the
new public repository and authorized project organization and push. The new
repository was empty at preflight. The original history was retained, not
squashed or force-pushed. Local `origin` now names the new repository; the
previous `https://github.com/ChopperLin/sudoku_FJ.git` remote is retained as
`legacy`. The publication branch is `main`. Git credential account selection
is scoped to the new repository URL and selects the existing linche89 account;
no credential is stored in tracked files.

The former full status is preserved as
`docs/history/status-through-c6-verification-20260907.md`. Its Git-normalized
blob equals the previous STATUS.md blob exactly:
`2ce24c62871a1fae1c1bd0f09b7f671989060f26`. The local safety tag
`archive/pre-publication-20260907` points to the previous verified state
`c735085`; it is not a production checkpoint migration.

The main README, concise current status, documentation index, prototype map,
reproducibility guide and C5 manuscript artifact URLs were updated. The
stale agent instruction saying that only G1 was known was corrected to refer
to the completed C6 certificate and its precise independence limits.
No counting algorithm, canonicalization rule, CSV value or checkpoint format
was changed. No production checkpoint was moved, deleted or overwritten.
No new license or manuscript author identity was chosen for the owner.

The pre-existing untracked L4 export receipt, two global-pivot probe files,
`paper/c5-open-verification/` and `run_continue.ps1` were preserved and excluded
from the publication commits. Large production images and transient logs
remain ignored. A tracked-tree and reachable-history size scan found no
oversized checkpoint blob; the largest existing blob was 2,439,561 bytes.
A bounded common-credential-pattern scan found no matches in the tracked
tree or reachable patch history. This is a scoped check, not a guarantee
against every possible secret format.

## Changes and commits

- `2708dcd`: standalone C6 certificate packaging and bundle instructions.
- `1a73655`: public project documentation and preserved development status.
- `f749723`: fixed LF checkout for the SHA-pinned C5 reference fixture.
- `abe06a0`: result-first README with direct standalone C6 verification,
  small-case computation, regression and explicitly qualified runtime sections.

The bundle/tag source is `abe06a0f3a04e481747a9720369df71d678c46ea`.
The actual C6 production source remains
`4dd86b590a2531993af0d00dbb93cf9a13b4de6c`; publication is not a new execution
of that large computation.

## Full regression and the fresh-clone finding

The original working tree passed:

```powershell
python scripts/run_guarded_step.py --seconds 600 --gib 8 --output data/logs/repo-publication-20260907-fullgate -- powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify_all.ps1
```

Result: exit 0, `ALL REPOSITORY CHECKS PASSED`, 173.2659915 seconds,
1,132,408,832-byte sampled aggregate peak, no surviving child. The optional
C6 read-only G1 memo reload also ran and passed.

A clone containing only committed files at 1a73655 failed its full gate after
48.4576994 seconds. The cause was Git's Windows LF-to-CRLF checkout of
`docs/expert/2026-07-21/native_c5_response_quotient_triples.csv`, before any
layer-DP numerical mismatch. Original and clone became byte-identical after
removing only the clone's CR characters from CRLF pairs. Expected LF SHA-256:
`D2FDEB354ED4C1443E9870B5727CE35C88BA6B392C6DAA32CD92D2601BCDB1F5`;
converted SHA-256:
`2659908F01859BEE14EB56DA8C992937860DA35D7B2CB0C5093A3348C0CB66AB`.

The fix is one narrowly scoped `text eol=lf` rule in `.gitattributes`.
The fixture contents and required hash were not changed. A second, new
clone checked the expected LF hash before rerunning the complete gate.
The failed clone and its diagnostic logs were retained, not disguised as
a successful test.

The corrected clean-clone command was:

```powershell
git clone --no-local --single-branch --branch main . build/publication-clean-lf
python scripts/run_guarded_step.py --seconds 600 --gib 8 --output data/logs/repo-publication-20260907-clean-lf-fullgate -- powershell -NoProfile -ExecutionPolicy Bypass -File build/publication-clean-lf/scripts/verify_all.ps1
```

Result at f749723: exit 0, `ALL REPOSITORY CHECKS PASSED`, 181.3350529 seconds,
1,128,583,168-byte sampled aggregate peak and no surviving child. The optional
C6 memo reload was correctly skipped because the clone contains no large
checkpoint. It passed in the original tree above. The later abe06a0 change
was README-only; no tested numerical source, fixture or build flag changed.
No Linux/macOS full build or manuscript re-typesetting was run.

## Standalone bundle qualification

The published payload uses the existing three verification implementations;
the new packager does no counting. It accepts only the frozen complete C6 CSV,
uses exclusive archive creation, checks all ZIP members by readback, and
includes content hashes and source revision in `manifest.json`.

The initial candidate archive was extracted into a new directory. These
commands ran against the extracted programs and extracted CSV:

```text
python s4_certificate_verify.py c6-final.csv --c 6 --classes 63199 --expect-n 38296278920738107863746324732012492486187417600000 --quiet
python s4_exact_sum.py c6-final.csv --classes 63199 --expect-n 38296278920738107863746324732012492486187417600000
powershell -NoProfile -ExecutionPolicy Bypass -File s4_exact_sum.ps1 -Dump c6-final.csv -Classes 63199 -ExpectN 38296278920738107863746324732012492486187417600000
```

| Check | Result | Guard | Elapsed seconds | Sampled aggregate peak bytes |
|---|---|---|---:|---:|
| Complete semantic certificate | PASS, exit 0 | 120 s / 2 GiB | 23.9780782 | 83,824,640 |
| Python exact sum | PASS, exit 0 | 30 s / 1 GiB | 0.1966614 | 56,958,976 |
| Independent .NET sum | PASS, exit 0 | 60 s / 1 GiB | 24.5721850 | 328,515,584 |

Semantic and .NET checks ran concurrently; these are validation observations,
not standalone performance comparisons. No process survived any guard.
All three obtained the exact N(6); the semantic checker confirmed all 63,199
classes, mass 622345892187672576, G1 at qid 43200 and G2 at qid 43206.

Reusing an existing output archive and supplying an invalid-length input each
produced the required refusal exit code 2. The existing archive was retained;
no rejected-input archive was published. The final archive's five data/code/
instruction payloads are byte-identical to the tested candidate; only the
packaging-head metadata changed after the publication commits.

Final package:

```text
name: sudoku-c6-certificate-2026-09-07.zip
members: 6
bytes: 1074359
SHA256: 0B4769DEAE561308CD17960AF926087CDEA158BAB4DE38B32E9A78FC9ED35A12
CSV bytes: 4766612
CSV SHA256: 84F2720E7BA8296D78604153B934B7291AF93C329C07F5D46538F9D7101127C7
```

All five tested payloads were compared exactly with the final ZIP. This
bundle verifies the final table; it does not recompute all upstream F values
and contains no large checkpoint or production executable.

## Remote publication and readback

The branch and annotated certificate tag were pushed without force:

```text
git push -u origin main
git push origin refs/tags/c6-verified-2026-09-07
```

At initial publication, the remote main branch and peeled certificate tag
both resolved to abe06a0f3a04e481747a9720369df71d678c46ea. The annotated tag
object is 1654ecb92788bea596f7ab09fba1459a603193cf. This report and the
public-artifact links are a subsequent documentation-only main-branch update;
the released tag and its numerical sources remain fixed.

The GitHub release was created as a draft, its uploaded asset size and
server-reported digest were checked, and it was then published:

- [Repository](https://github.com/linche89/sudoku-enumeration)
- [Release](https://github.com/linche89/sudoku-enumeration/releases/tag/c6-verified-2026-09-07)
- [Complete certificate ZIP](https://github.com/linche89/sudoku-enumeration/releases/download/c6-verified-2026-09-07/sudoku-c6-certificate-2026-09-07.zip)
- Release ID 383933114; asset ID 548441422.

The published ZIP was downloaded without authentication to a new local path.
Its size and SHA-256 matched the values above. All five data/program/readme
members exactly matched the independently tested extracted candidate, and
the manifest names abe06a0 as its packaging source. The public main-branch
README was read back through the GitHub API and its new title and standalone
C6-verification section were checked. The repository description was updated
to the exact-count/reproducibility scope; no license was added.

Local evidence is retained under `data/logs/repo-publication-20260907-*`.
Generated ZIPs, clean test clones and downloaded readback are under ignored
`build/`. They are disposable publication-test artifacts, not production
checkpoints. No old logs, checkpoints, physical backups or user drafts were
deleted during publication.
