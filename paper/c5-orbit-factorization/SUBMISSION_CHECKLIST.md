# Submission checklist

Complete these items before uploading the manuscript.

- [ ] Replace the placeholder author block with names, affiliations, email,
      and ORCID identifiers as desired.
- [ ] Decide the repository software license and add a root `LICENSE` file.
- [ ] Tag the audited release and replace the draft commit hash if necessary.
- [ ] Archive that release with a persistent DOI, then add the DOI to the
      paper and `references.bib`.
- [ ] Ask at least one external reader to audit the skeleton-to-graph
      bijection, the orbit multiplicity, and the rooted degree-four identity.
- [ ] Ask a second person to reproduce `C=5` from a clean checkout on another
      machine and retain the complete output plus toolchain details.
- [ ] Contact Kjell Fredrik Pettersen and/or Frazer Jarvis if practical, both
      as scholarly courtesy and to ask whether historical `C=5` source code or
      an unpublished independent verification still exists.
- [ ] Decide whether to publish the 355 weighted outer terms as a compact
      machine-checkable certificate.
- [ ] Run `scripts\verify_all.ps1` on the exact release commit.
- [ ] Rebuild the PDF twice with a clean LaTeX auxiliary directory and inspect
      every page at submission size.
- [ ] Remove the word `Draft` and update the manuscript date.
