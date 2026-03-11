# IP Chain-of-Title Checklist

This checklist supports buyer due diligence for ownership and transfer clarity.

## Ownership Evidence
- [x] Proprietary repository under seller control.
- [x] Commercial license terms included (`COMMERCIAL_LICENSE_AGREEMENT.md`).
- [x] Distribution constraints documented (`DISTRIBUTION_POLICY.md`).

## Third-Party Dependencies
- [x] Core runtime targets zero mandatory third-party runtime dependencies.
- [x] Build/test flow uses standard toolchains (CMake + C++ compiler).

## Source and Artifact Integrity
- [x] Hash tooling present in `tools/hash_package.*`.
- [x] Determinism/reference artifacts provided in `proof/`.
- [x] Release gate script verifies required buyer-facing files (`scripts/verify_release_gate.py`).

## Transfer Readiness
- [x] Buyer onboarding path in `docs/BUYER_START_HERE.md`.
- [x] Scope statement in `docs/UPE_CORE_SCOPE.md`.
- [x] Acceptance checklist in `docs/ACCEPTANCE_CHECKLIST.md`.

## Notes
- Complete legal assignment and closing mechanics are governed by definitive transaction documents.
