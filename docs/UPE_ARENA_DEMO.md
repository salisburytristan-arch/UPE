# UPE Arena Demo

This demo is the first concrete step toward the roadmap's "killer demonstration" milestone: a compact, buyer-friendly simulation showcase that publishes runtime metrics and a state hash in a single screen.

## Goals

- Demonstrate a representative rigid-body scene in one binary
- Show the engine working under a deterministic stepping pipeline
- Publish a human-readable performance snapshot
- Give a concrete path for a later graphical version of the same scene

## Scene layout

- Ground plane
- Stacked rigid boxes in a compact 4x4x4 grid
- Gravity-enabled world with fixed timestep
- Deterministic hashing around each step for reproducibility checks

## Run it

```bash
cmake -S . -B build -DUPE_BUILD_EXAMPLES=ON -DUPE_BUILD_TESTS=ON -DUPE_DETERMINISTIC=ON
cmake --build build --target upe_arena
./build/bin/upe_arena
```

## Example output

```text
UPE Arena
Bodies:        65
Frames:        180
Step:          0.812 ms
Physics FPS:   1231.6
State Hash:    0x6E8B11F9A2C3D4E1
Deterministic: YES
```

## Why this matters for the roadmap

This is not a toy benchmark; it is a reusable presentation artifact. It creates a stable visual and textual baseline that can later be expanded into:

- richer vehicle and joint scenes
- a proper renderer or visual overlay
- deterministic replay/export output
- cross-platform certification comparisons

The idea is to move from "codebase with many features" to "engine with a visible, credible, repeatable product demo."
