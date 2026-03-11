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
| Advanced science domains (thermal/EM/orbital/plasma) | present-not-buyer-critical | Available in repo; not default buyer acceptance baseline |
| GPU compute acceleration | experimental | Not first-class production path |
| Broad ecosystem bindings beyond primary C API | present-not-buyer-critical | Secondary to C API acceptance flow |

## Usage Guidance

For buyer evaluation and commercial diligence, prioritize `production-ready` and `validated` rows before treating other categories as purchase-critical requirements.

## Related References

- [BUYER_START_HERE.md](BUYER_START_HERE.md)
- [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md)
- [KNOWN_LIMITATIONS.md](KNOWN_LIMITATIONS.md)
- [PROOF_INDEX.md](PROOF_INDEX.md)
