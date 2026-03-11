# Dependency Inventory

This file captures dependency posture for buyer diligence and integration planning.

## Runtime Dependencies

Core runtime target:

- No mandatory third-party runtime physics engine dependency for standard C API integration.

## Build and Tooling Dependencies

Typical build/tooling stack:

- CMake
- Native compiler toolchains (MSVC, GCC, Clang)
- Python for repository scripts and validation helpers

These are build-time/tooling dependencies, not mandatory runtime physics engine dependencies.

## Optional Integration Layers

Bindings directories exist for:

- Python
- C#
- Rust
- JavaScript/WASM
- Lua

Bindings are treated as integration convenience layers and are not required for core C API consumption.

## Exclusions

This inventory does not assert cryptographic SBOM attestations. If SBOM generation is required for a transaction, add it as a packaging task in release automation.

## Related References

- [BINDINGS_STATUS.md](BINDINGS_STATUS.md)
- [PROVENANCE_AND_OWNERSHIP.md](PROVENANCE_AND_OWNERSHIP.md)
- [CI_GATES.md](CI_GATES.md)
