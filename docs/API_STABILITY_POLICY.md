# API Stability Policy

This policy defines the integration contract for the UPE C API.

## Contract Surface

The stable integration boundary is the public C API in `core/include/upe/`.

Scope:

- `upe_*.h` headers intended for external consumers
- Exported C symbols documented in API reference docs
- Build and package metadata needed to consume the library

Out of scope:

- Internal C++ implementation details
- Unpublished internal headers
- Experimental flags not documented as stable

## Versioning Policy

UPE follows semantic versioning intent for buyer-facing integrations:

- `MAJOR`: may contain API/ABI breaking changes
- `MINOR`: additive, backward-compatible changes
- `PATCH`: bug fixes and non-breaking behavior corrections

Current package version is tracked in build metadata and API capability discovery.

The product-level guarantee for the supported core is intentionally stricter than generic semantic-versioning language:

- source compatibility within the same minor series is the default expectation
- patch releases must not silently alter documented replay/snapshot/determinism outcomes for supported scenes
- all public compatibility changes require release notes and migration guidance

## Compatibility Expectations

For a given major version:

- Public C API should remain source-compatible unless explicitly documented otherwise.
- New features should be additive.
- Any behavior changes that affect determinism, serialization, or replay should be documented in release notes.
- Experimental modules remain excluded from the stable compatibility contract until explicitly promoted.

## Production-Core Compatibility Boundary

The production compatibility boundary is the supported core path:

- world create/step lifecycle
- rigid-body lifecycle and shape setup
- solver, contact, and narrowphase pipeline
- deterministic hash and replay/snapshot workflows
- canonical query and step-graph accessors

The following remain outside the compatibility promise until they are promoted:

- advanced scientific packs
- exploratory multiphysics domains
- experimental orbital / chemistry / relativistic modules
- unvetted lab features in the broader simulation stack

## Deprecation Rules

When a C API entry point is planned for removal:

1. Mark as deprecated in docs and release notes.
2. Provide a replacement API path.
3. Keep deprecated entry points through at least one minor cycle before removal in a major release.

## Header Quality Requirements

Public headers should maintain:

- clear naming consistency
- explicit ownership/lifetime behavior
- thread-safety expectations per API family
- determinism notes where call ordering or flags matter

## Ownership and Thread-Safety Notes

Baseline rules:

- World-scoped handles are owned by the world lifecycle unless explicitly transferred.
- API consumers own caller-allocated buffers unless documented otherwise.
- Mutating world operations are not assumed thread-safe unless explicitly documented.
- Read/query operations should still respect frame-step synchronization constraints.

## Migration Notes Requirement

Any release with observable integration-impact changes should include:

- migration notes
- before/after examples for renamed or replaced APIs
- known compatibility caveats

## Related References

- [../API_REFERENCE.md](../API_REFERENCE.md)
- [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md)
- [DETERMINISM_CERTIFICATION.md](DETERMINISM_CERTIFICATION.md)
- [KNOWN_LIMITATIONS.md](KNOWN_LIMITATIONS.md)
