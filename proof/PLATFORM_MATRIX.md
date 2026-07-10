# PLATFORM MATRIX

This matrix describes the current validated and non-validated platform story for UPE.

| Platform | Toolchain | Current Status | Evidence |
| --- | --- | --- | --- |
| Windows x64 | MSVC | verified-in-ci | Build and test lanes plus determinism proof scripts |
| Linux x64 | GCC | verified-in-ci | Build and test lanes plus proof artifacts |
| Linux x64 | Clang | verified-in-ci | Build and test lanes plus proof artifacts |
| macOS | Apple Clang | verified-in-ci | Build and test lanes plus proof artifacts |
| Windows x64 | MinGW/GCC | experimental | Local build paths exist, but not a current buyer proof lane |
| Linux ARM64 | GCC/Clang | not-yet-validated | No active proof lane in this snapshot |
| macOS ARM64-specific guarantees | Apple Clang | experimental | No dedicated architecture proof lane |

## Interpretation

- `verified-in-ci` is the strongest current signal.
- `experimental` and `not-yet-validated` should not be marketed as production certainty.
- Use `proof/PROOF_INDEX.md` and `proof/CI_DETERMINISM_REPORT.md` together for the full buyer path.