# Maturity Matrix

This matrix classifies repository capabilities for buyer trust and scope clarity.

Status labels:

- `production-ready`: core buyer path, repeatedly validated
- `validated`: implemented and tested, but not the primary buyer-critical path
- `experimental`: present with partial validation or evolving guarantees
- `present-not-buyer-critical`: available in repo but not part of the default acceptance baseline

| Capability Area | Status | Notes |
| --- | --- | --- |
| Core rigid-body world step pipeline | production-ready | Primary buyable v1 scope |
| Collision broadphase/narrowphase + solver | production-ready | Core acceptance path and CI/test covered |
| Replay, snapshot, state hashing | production-ready | Determinism and rollback evaluation wedge |
| Build/test/release gating | production-ready | CI and artifact proof lanes enforced |
| Cross-platform build matrix | validated | CI-built lanes across Windows/Linux/macOS |
| Cross-compiler determinism visibility | validated | Drift surfaced via artifacts/logs |
| Vehicle and articulated systems | validated | Implemented and test/demo covered |
| Soft body / cloth / fluids | validated | Included, benchmarked, and documented |
| Spatial and math utility modules | validated | SDF, octree, pathfinding, math toolbox, and reusable rendering/noise helpers |
| Advanced science domains (thermal/EM/orbital/plasma) | present-not-buyer-critical | Available in repo; not default buyer acceptance baseline |
| Microphysics and molecular approximations | experimental | Seeded deterministic approximations; not first-principles simulation |
| Biomechanics, electronics, and other science-approximation modules | experimental | Usable models with bounded scientific scope |
| Portal, non-Euclidean, and other gameplay-exotic domains | experimental | Intentionally beyond buyer-critical production scope |
| GPU compute acceleration | experimental | Not first-class production path |
| Broad ecosystem bindings beyond primary C API | present-not-buyer-critical | Secondary to C API acceptance flow |

## Usage Guidance

For buyer evaluation and commercial diligence, prioritize `production-ready` and `validated` rows before treating other categories as purchase-critical requirements.

When evaluating repository breadth, use these interpretation rules:

- `validated` means the subsystem is implemented, builds cleanly, and is suitable for engineering review.
- `present-not-buyer-critical` means the code is in-repo but intentionally excluded from the standard diligence baseline.
- `experimental` includes mathematically grounded approximation modules and gameplay-forward mechanics that may evolve.

## Related References

- [BUYER_START_HERE.md](BUYER_START_HERE.md)
- [FEATURE_STATUS_MATRIX.md](FEATURE_STATUS_MATRIX.md)
- [SCIENTIFIC_COMPLETENESS_PLAN.md](SCIENTIFIC_COMPLETENESS_PLAN.md)
- [SIMULATION_DOMAIN_REGISTRY.md](SIMULATION_DOMAIN_REGISTRY.md)
- [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md)
- [KNOWN_LIMITATIONS.md](KNOWN_LIMITATIONS.md)
- [PROOF_INDEX.md](PROOF_INDEX.md)
