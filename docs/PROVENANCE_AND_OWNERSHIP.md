# Provenance and Ownership

This document summarizes subsystem provenance and ownership posture for diligence.

## Ownership Statement

- UPE source in this repository is maintained as proprietary project IP.
- Chain-of-title and transfer-oriented diligence checklists are provided separately.
- No ownership transfer is implied by repository access alone.

See also:

- `IP_CHAIN_OF_TITLE_CHECKLIST.md`
- `COMMERCIAL_LICENSE_AGREEMENT.md`

## Subsystem Provenance (High Level)

| Subsystem | Provenance Summary |
| --- | --- |
| Core world/body/shape lifecycle | In-repo implementation under `core/src/` and public C headers |
| Collision and solver pipeline | In-repo implementation; deterministic validation via in-repo tests/scripts |
| Replay/snapshot/hash tooling | In-repo implementation with proof scripts in `tools/` and `scripts/` |
| Build/release gating scripts | In-repo automation in `scripts/` and CI workflows |
| Bindings wrappers | In-repo convenience layers under `bindings/` |

## Third-Party Influence Disclosure

The project documentation references established physics engine concepts and industry-standard numerical techniques. This document does not claim derivation from any specific third-party proprietary source.

If a buyer requires line-by-line provenance evidence for a specific subsystem, that review should be handled as part of due diligence scope.

## Dependency Posture

Core runtime posture is documented as no mandatory external runtime dependencies for standard C API usage.

See [DEPENDENCY_INVENTORY.md](DEPENDENCY_INVENTORY.md).

## AI Use Policy Reference

See [AI_USE_POLICY.md](AI_USE_POLICY.md) for repository-level policy statements around AI-assisted development workflows.
