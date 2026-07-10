# LIMITATIONS

The current repository is useful for evaluation, but it is not perfect.

## Determinism Limits

- Repeated-run determinism is the strongest current claim.
- Cross-platform bit-exact determinism is not universally guaranteed.
- Compiler and platform differences can still show drift outside the validated lanes.
- Determinism depends on the documented build flags and engine allocator path.

## Build Limits

- Clean checkout builds rely on a working CMake toolchain and a supported compiler.
- The repo does not vendor a third-party dependency tree in the current snapshot.
- Some optional build paths remain experimental rather than fully production-locked.

## Feature Limits

- Several advanced scientific modules are still better treated as expansion candidates than as hardened production commitments.
- Not every demo or binding target has a validated end-to-end proof lane.
- Benchmark coverage exists, but it is not yet a full performance contract for every module.

## Buyer Caveats

- This repository shows the engine and its proof surface, but some roadmap items are still aspirational.
- The honest source of truth for current confidence levels is `proof/` plus the status docs.