# Sale Package Structure

This document defines a three-layer package model for outreach through diligence.

## 1) First-Contact Package

Purpose: generate interest quickly.

Recommended contents:

- `ONE_PAGE_OVERVIEW.md`
- `outreach_email.md`
- short differentiator list
- target use-case fit summary
- proof-surface teaser with links to `docs/PROOF_INDEX.md`

## 2) Technical Evaluation Package

Purpose: support serious engineering assessment.

Recommended contents:

- `docs/BUYER_START_HERE.md`
- `docs/PROOF_INDEX.md`
- `docs/CI_GATES.md`
- `docs/DETERMINISM_CERTIFICATION.md`
- `docs/BENCHMARK_BASELINES.md`
- `docs/API_STABILITY_POLICY.md`
- `docs/MATURITY_MATRIX.md`
- `docs/PLATFORM_SUPPORT_MATRIX.md`
- `docs/KNOWN_LIMITATIONS.md`
- `docs/INTEGRATION_GUIDE.md`
- `docs/RELEASE_NOTES_v1.0.0.md`
- `docs/SHOWCASE_DETERMINISTIC_REPLAY_SPEC.md`

## 3) Diligence Package

Purpose: close-stage legal and technical diligence.

Recommended contents:

- `docs/PROVENANCE_AND_OWNERSHIP.md`
- `IP_CHAIN_OF_TITLE_CHECKLIST.md`
- `COMMERCIAL_LICENSE_AGREEMENT.md`
- `docs/DEPENDENCY_INVENTORY.md`
- acceptance checklist and release manifest artifacts
- release verification outputs from `releases_ci/`
- proof archive from CI artifacts

## Authority Rule

When package layers conflict, defer to `docs/TRUTH_HIERARCHY.md`.
