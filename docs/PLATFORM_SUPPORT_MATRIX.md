# Platform Support Matrix

This matrix separates build evidence from support guarantees.

Status legend:

- `verified`: locally validated and CI-built with active proof artifacts
- `CI-built`: CI builds/tests run, but broader validation scope is limited
- `experimental`: partial support path exists, not yet consistently validated
- `unverified`: no current validation signal

| Platform | Toolchain | Status | Evidence |
| --- | --- | --- | --- |
| Windows x64 | MSVC (Release/Debug) | verified | CI Windows matrix + uploaded test logs/artifacts |
| Linux x64 | GCC | verified | CI Linux matrix + strict warnings + benchmark and determinism jobs |
| Linux x64 | Clang | verified | CI Linux matrix + strict warnings + cross-compiler determinism logs |
| macOS | Apple Clang | CI-built | CI macOS matrix build/test logs present |
| Windows x64 | MinGW/GCC | experimental | Local build paths exist; not part of official CI matrix contract |
| Linux ARM64 | GCC/Clang | unverified | No active CI lane in current workflow |
| macOS ARM64-specific guarantees | Apple Clang | experimental | Built via macOS CI image, but no dedicated architecture proof lane |

## Interpretation

- `verified` is the strongest buyer-facing signal.
- `CI-built` indicates practical build confidence but narrower guarantee scope.
- `experimental` and `unverified` should not be represented as production-validated support.

## Related References

- [CI_GATES.md](CI_GATES.md)
- [PROOF_INDEX.md](PROOF_INDEX.md)
