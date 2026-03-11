# Contradiction Fix Checklist

This checklist tracks trust-alignment between legacy and trust-oriented docs.

## Determinism Claims

- [x] Normalize determinism authority to `DETERMINISM_CERTIFICATION.md` and `PROOF_INDEX.md`.
- [x] Add explicit determinism claim levels to a shared hierarchy doc.
- [x] Remove blanket cross-platform bit-exact wording from buyer and release framing docs.
- [ ] Add line-by-line determinism wording audit for remaining legacy docs outside `docs/`.

## Platform and Support Language

- [x] Keep platform confidence anchored to matrix/evidence language.
- [ ] Standardize all platform labels to the hierarchy canonical set (`verified-in-ci`, `verified-locally`, `build-config-present`, `not-yet-validated`, `experimental`).

## Build Status Confidence

- [x] Rewrite `BUILD_STATUS.md` into confidence tiers:
- Verified now
- Verified with caveats
- Pending verification
- Not claimed

## Feature Breadth vs Maturity

- [x] Treat maturity/support docs as authoritative when wording conflicts exist.
- [ ] Add explicit "maturity docs win" note to additional broad-scope docs where needed.

## Release Notes vs Proof Surface

- [x] Add top-of-file trust-authority note in `RELEASE_NOTES_v1.0.0.md` linking to proof docs.
- [ ] Apply same authority note to any additional release-summary files.

## Source-of-Truth Hierarchy

- [x] Publish hierarchy document: `TRUTH_HIERARCHY.md`.
- [x] Link hierarchy from `PROOF_INDEX.md` and `INDEX.md`.
