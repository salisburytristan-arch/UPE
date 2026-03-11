# UPE Core Scope

This document defines the buyer-facing scope for the core UPE package.

## In Scope

The core runtime evaluation path includes the capabilities that are directly validated by the repository build, test, and determinism tooling:

- Rigid body world lifecycle and stepping
- Collision detection via broadphase and narrowphase
- Constraint solving, friction, restitution, and warm starting
- Continuous collision detection for the covered primitive paths
- Snapshot, replay, and state hashing for deterministic validation
- Example applications and C API integration path

## Included Artifacts

The core scope assumes these repository assets are part of evaluation:

- Public headers under `core/include/`
- Engine sources under `core/src/`
- Tests under `tests/`
- Example applications under `examples/`
- Build and validation helpers under `tools/`

## Included Documentation

- [BUYER_START_HERE.md](BUYER_START_HERE.md)
- [FEATURE_MATRIX.md](FEATURE_MATRIX.md)
- [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md)
- [DETERMINISM_GUIDE.md](DETERMINISM_GUIDE.md)
- [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md)

## Outside the Core Buyable Path

The broader repository contains additional systems and experimental breadth beyond the strict core evaluation path. These areas may exist in the codebase, but they are not the first-step acceptance baseline for a buyer review:

- Secondary language bindings
- Domain-specific science topic catalog breadth
- Internal commercialization and diligence packaging materials
- Deep-dive implementation exploration outside the standard build-test-proof flow

## Acceptance Baseline

The scope should be evaluated against the following observable outcomes:

1. The project configures and builds on the supported toolchains.
2. CTest passes from a clean build.
3. Determinism proof scripts complete successfully.
4. Example binaries run as smoke tests.

For the detailed checklist, see [ACCEPTANCE_CHECKLIST.md](ACCEPTANCE_CHECKLIST.md).