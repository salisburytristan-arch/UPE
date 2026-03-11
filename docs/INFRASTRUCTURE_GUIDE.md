# Infrastructure Guide

This guide summarizes the production-facing infrastructure added around the runtime core.

## Scope

In this repository, infrastructure refers to the non-simulation systems that make the engine practical to integrate, validate, and ship:

- Build configuration and CI workflow
- Runtime configuration and capability discovery
- Error reporting and diagnostics
- Telemetry and profiling hooks
- Validation descriptors and release gating

## Build and CI Infrastructure

The project uses CMake as the primary build interface and GitHub Actions for cross-platform verification.

Relevant references:

- [BUILD_GUIDE.md](BUILD_GUIDE.md)
- [TEST_GUIDE.md](TEST_GUIDE.md)
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md)

## Runtime Infrastructure

The release notes identify the runtime-focused infrastructure modules in the current production foundation:

- Engine configuration
- Error handling
- Telemetry export
- Validated descriptors
- Extended world API helpers

For the high-level architecture around those systems, see [ARCHITECTURE.md](ARCHITECTURE.md).

## Validation Infrastructure

The repository includes explicit validation paths for:

- Unit tests via CTest
- Determinism proof scripts
- Release-gate artifact verification
- Documentation and topic audit freshness checks

Buyer-facing entry points:

- [BUYER_START_HERE.md](BUYER_START_HERE.md)
- [DETERMINISM_GUIDE.md](DETERMINISM_GUIDE.md)
- [TOPIC_TRUTH_AUDIT.md](TOPIC_TRUTH_AUDIT.md)

## Operational Guidance

When evaluating or integrating the engine:

1. Build with deterministic mode enabled.
2. Run CTest and the determinism proof.
3. Review known limitations before broad feature assumptions.

That sequence keeps evaluation aligned with the infrastructure the repository actually verifies.