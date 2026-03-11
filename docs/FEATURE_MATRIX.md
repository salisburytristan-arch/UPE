# Feature Matrix

This matrix separates the buyer-critical path from broader repository capabilities so evaluation stays grounded in what is directly validated.

| Area | Status | Validation Path | Primary Reference |
|---|---|---|---|
| Core C API | Present | Build + tests | [API_REFERENCE.md](API_REFERENCE.md) |
| Rigid body pipeline | Present | Tests + examples | [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md) |
| Determinism, replay, hashing | Present | Determinism proof | [DETERMINISM_GUIDE.md](DETERMINISM_GUIDE.md) |
| Build and packaging tooling | Present | CI + helper scripts | [BUILD_GUIDE.md](BUILD_GUIDE.md) |
| Troubleshooting and known boundaries | Present | Buyer evaluation | [KNOWN_LIMITATIONS.md](KNOWN_LIMITATIONS.md) |
| Bindings | Present, secondary | Binding-specific follow-up | [BINDINGS_STATUS.md](BINDINGS_STATUS.md) |
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

Use [UPE_CORE_SCOPE.md](UPE_CORE_SCOPE.md) as the scope boundary when deciding what must pass before purchase review.