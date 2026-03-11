# Feature Matrix

This matrix separates the buyer-critical path from broader repository capabilities so evaluation stays grounded in what is directly validated.

| Area | Status | Validation Path | Primary Reference |
| --- | --- | --- | --- |
| Core C API | Present | Build + tests | [API_REFERENCE.md](API_REFERENCE.md) |
| Rigid body pipeline | Present | Tests + examples | [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md) |
| Determinism, replay, hashing | Present | Determinism proof | [DETERMINISM_GUIDE.md](DETERMINISM_GUIDE.md) |
| Build and packaging tooling | Present | CI + helper scripts | [BUILD_GUIDE.md](BUILD_GUIDE.md) |
| Troubleshooting and known boundaries | Present | Buyer evaluation | [KNOWN_LIMITATIONS.md](KNOWN_LIMITATIONS.md) |
| Bindings | Present, secondary | Binding-specific follow-up | [BINDINGS_STATUS.md](BINDINGS_STATUS.md) |
| Scientific breadth registry | Present | Doc review + module inspection | [SIMULATION_DOMAIN_REGISTRY.md](SIMULATION_DOMAIN_REGISTRY.md) |
| Feature-level scientific coverage | Present | Feature audit | [FEATURE_STATUS_MATRIX.md](FEATURE_STATUS_MATRIX.md) |
| Scientific completeness roadmap | Present | Phase and scope review | [SCIENTIFIC_COMPLETENESS_PLAN.md](SCIENTIFIC_COMPLETENESS_PLAN.md) |
| Advanced and experimental breadth | Repository present | Evaluate case-by-case | [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md) |

## Interpretation

- `Present` means the repository contains code and documentation for the area.
- `Buyer-critical path` means the area is part of the standard build, test, and determinism validation flow.
- `Secondary` means the capability may matter for adoption, but it is not required to validate the engine core.

## Risk View

Lowest integration risk:

- C API evaluation
- Deterministic rigid body stepping
- Scripted build and test workflow

Higher review effort:

- Optional bindings
- Broader subsystem exploration beyond the standard acceptance path
- Scientific approximation domains and experimental gameplay subsystems

## Breadth Summary

Repository-present breadth now includes:

- Microscopic and molecular approximations
- Additional soft-body and material models such as DEM, shape matching, and viscoelastic hysteresis
- Environmental domains such as phase change, acoustics, optics, and weathering
- Vehicle and biomechanics extensions including ballistics and physiology-inspired models
- Astronomic, relativistic, and EM/electronics subsystems
- Utility modules for SDFs, octrees, pathfinding, procedural noise, and rendering math

Use [FEATURE_STATUS_MATRIX.md](FEATURE_STATUS_MATRIX.md) for item-by-item coverage and [MATURITY_MATRIX.md](MATURITY_MATRIX.md) for subsystem maturity.

Use [UPE_CORE_SCOPE.md](UPE_CORE_SCOPE.md) as the scope boundary when deciding what must pass before purchase review.
