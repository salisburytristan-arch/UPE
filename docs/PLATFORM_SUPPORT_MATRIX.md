# Platform Support Matrix

This matrix separates build evidence from support guarantees.

Status legend:

- `verified-in-ci`: CI build/test evidence with active artifacts in current workflow
- `verified-locally`: locally validated path with reproducible commands/artifacts
- `build-config-present`: build path exists, but no current strong validation signal
- `experimental`: partial support path exists, not yet consistently validated
- `not-yet-validated`: no current validation signal

| Platform | Toolchain | Status | Evidence |
| --- | --- | --- | --- |
| Windows x64 | MSVC (Release/Debug) | verified-in-ci | CI Windows matrix + uploaded test logs/artifacts |
| Linux x64 | GCC | verified-in-ci | CI Linux matrix — see .github/workflows/ci.yml |
| Linux x64 | Clang | verified-in-ci | CI Linux matrix — see .github/workflows/ci.yml |
| macOS | Apple Clang | verified-in-ci | CI macOS matrix build/test logs present |
| Windows x64 | MinGW/GCC | experimental | Local build paths exist; not part of official CI matrix contract |
| Linux ARM64 | GCC/Clang | not-yet-validated | No active CI lane in current workflow |
| macOS ARM64-specific guarantees | Apple Clang | experimental | Built via macOS CI image, but no dedicated architecture proof lane |

## Interpretation

- `verified-in-ci` is the strongest current buyer-facing signal in this matrix.
- `verified-locally` is useful but weaker than CI-backed evidence unless artifacts are published.
- `build-config-present`, `experimental`, and `not-yet-validated` should not be represented as production-validated support.

## Related References

- [CI_GATES.md](CI_GATES.md)
- [PROOF_INDEX.md](PROOF_INDEX.md)
- [TRUTH_HIERARCHY.md](TRUTH_HIERARCHY.md)
