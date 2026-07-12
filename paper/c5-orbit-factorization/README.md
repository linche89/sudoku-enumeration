# C=5 orbit-factorization paper

This directory is a self-contained LaTeX draft for the reproducible exact
enumeration of completed `2 x C` Sudoku grids through `C=5`.

## Build

From this directory:

```powershell
latexmk -pdf -interaction=nonstopmode -halt-on-error main.tex
```

Clean generated files with:

```powershell
latexmk -C
```

The visually checked draft PDF is copied to
`../../output/pdf/c5-orbit-factorization-draft.pdf`; generated PDFs are not
tracked in Git.

## Reproduce the computational result

From the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build_og2.ps1
.\build\factorization_orbit.exe 5 pivot rooted4
```

The run must finish with both the outer multiplicity check and the final
decimal comparison marked `[OK]`.

## Files

- `main.tex`: complete manuscript, including two TikZ figures and a
  same-binary algorithmic ablation.
- `references.bib`: primary historical sources and algorithm references.
- `SUBMISSION_CHECKLIST.md`: items that require an author decision or an
  external audit before an arXiv upload.

## Draft status

The mathematics, numerical values, source hash, and benchmark table have been
checked against repository commit
`d86cd88383da915e33caf8e361cfb193134953dc`. The author block is intentionally
a placeholder; no author identity or affiliation was inferred.
