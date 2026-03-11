# Fixed-Point Migration

This note captures the role of fixed-point math in the deterministic evaluation path.

## Purpose

The repository positions fixed-point math as part of the engine's determinism strategy, especially for replay validation and state hashing.

## What Matters for Review

- Deterministic mode is enabled with `-DUPE_DETERMINISTIC=ON`.
- Replay, snapshot, and hashing flows are part of the buyer validation path.
- The runtime is documented as using fixed-point support for deterministic collision and simulation-critical behavior.

## Migration Boundary

From an integration perspective, the important question is not whether every subsystem uses the same numeric model internally, but whether the supported deterministic validation path stays reproducible under the documented toolchain and configuration.

That boundary is validated through:

- [DETERMINISM_GUIDE.md](DETERMINISM_GUIDE.md)
- [TEST_GUIDE.md](TEST_GUIDE.md)
- [ACCEPTANCE_CHECKLIST.md](ACCEPTANCE_CHECKLIST.md)

## Recommended Usage

If your product depends on rollback, lockstep, or state hash verification, treat deterministic mode and proof tooling as mandatory acceptance steps.