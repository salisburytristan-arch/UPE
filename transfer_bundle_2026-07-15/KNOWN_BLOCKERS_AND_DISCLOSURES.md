# Known Blockers and Mandatory Disclosures

Date: 2026-07-15
Project: Unified Physics Engine (UPE)
Purpose: Enumerate sale blockers that must be fixed or explicitly accepted by buyer in writing before closing.

## 1) Demo Stability Blocker

Current known result set (example validation lane):
- Compiled examples: 41
- Passed: 39
- Crashed: `animation_demo`, `state_machine_demo`

Known fault class reported:
- AddressSanitizer heap-use-after-free involving animation clip ownership.

Required action before clean sale:
1. Fix and re-verify both demos, or
2. Add explicit risk acceptance in definitive agreement schedules.

## 2) Release Gate Artifact Blocker

Release package verification requires Linux reference-run artifacts under `proof/REFERENCE_RUNS/`.
If a tracked Linux reference artifact is absent from the source snapshot, release gate fails.

Required action before clean sale:
1. Generate package from a clean commit that contains required tracked reference artifacts.
2. Attach release gate pass evidence to closing package.

## 3) Source ZIP Must Match Git Commit

Observed risk profile from technical diligence:
- line-ending-only differences are acceptable if normalized,
- uncommitted substantive file changes are not acceptable,
- missing tracked files are not acceptable,
- large untracked artifact drift can confuse diligence.

Required action before clean sale:
1. Build sale archive from a clean, tagged commit only.
2. Record commit hash and tag in final manifest.
3. Provide checksums for all deliverables.

## 4) Licensing-Business Language Conflict

The repository contains licensing and pricing language that can conflict with one-time full-sale posture.

Required action before clean sale:
1. Deal docs must state one-time complete sale for UPE-specific assets.
2. Remove or neutralize conflicting commercial-license narratives in buyer-facing package.

## 5) Website/Domain Scope Correction

Correct scope statement for this transaction:

"Static UPE promotional website files are included. No standalone domain registration, BearPack parent domain, hosting account, payment account, or dedicated operating website is included."

Required action before clean sale:
1. Include this exact scope in transfer schedules and buyer summary documents.

## 6) Brand Identity Separation

Risk:
- Repository and materials may include references to BearPack, Arctic Codex, seller email, and seller checkout systems.

Required action before clean sale:
1. Explicit excluded-assets schedule separating UPE-specific assets from seller parent identity/business.
2. Explicit transfer scope schedule listing only in-scope UPE assets.

## 7) Definitive Agreement Gap

`TRANSFER_RIGHTS_AND_TRANSITION_TERMS.md` is an operational template and does not transfer ownership by itself.

Required action before clean sale:
1. Execute a signed asset purchase and IP assignment agreement.
2. Ensure it covers: right/title/interest transfer, copyright assignment, moral-rights treatment,
   excluded assets, price/payment, ownership reps, third-party and AI-assisted development disclosure,
   confidentiality, liability limits, transition support, final source delivery, and deletion obligations.

## 8) Transition Support Scope

Default support posture for fast close:
- 30 calendar days
- 10 total support hours
- one architecture walkthrough
- build/import troubleshooting only
- additional work prepaid at agreed hourly rate

Required action before clean sale:
1. Ensure final agreement schedules match this cap unless intentionally renegotiated.

## Closing Condition

A "clean sale package" should not be declared complete until:
1. technical blockers are fixed or contractually disclosed/accepted,
2. package is generated from a clean tagged commit,
3. definitive agreement is executed,
4. delivery and deletion obligations are completed and signed off.
