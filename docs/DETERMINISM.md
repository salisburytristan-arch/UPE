# UPE Determinism Proof Tooling

Status: determinism proof tooling available and CI-integrated
Primary scripts: tools/run_determinism_proof.(bat|sh)

## Claim Model (Authoritative)

Authoritative determinism claim levels are defined in [DETERMINISM_CERTIFICATION.md](DETERMINISM_CERTIFICATION.md):

- Level 1: repeated-run determinism (guaranteed in the proof path)
- Level 2: toolchain-scoped reproducibility
- Level 3: cross-compiler drift visibility (not guaranteed identical)
- Level 4: cross-platform bit-exact parity (only when explicitly proven)

If wording in this file appears stronger than the certification document, the certification document wins.

## Core Principle
**Unified Physics Engine includes determinism proof tooling for repeatable runs on a given binary/toolchain setup. Cross-platform bit-exact identity is not a blanket claim.**

## Determinism Mechanisms

### 1. Floating Point Determinism
- All math operations use IEEE 754 single-precision (32-bit float)
- Consistent rounding modes enforced across platforms
- Operations ordered to prevent compiler reordering
- No platform-specific intrinsics in deterministic path
- Epsilon policy for consistent comparisons

### 1.1 Experimental Fixed-Point Foundation
- Optional experimental integer math module is available at `core/include/upe/upe_fixed_q48_16.h`
- Type: signed deterministic `Q48.16` fixed-point (`int64_t` backing)
- Current scope: base arithmetic (add/sub/neg/mul/min/max/compare)
- This module is a migration seam and is not yet wired as the default world/solver scalar type

### 2. Data Structure Determinism
- Iteration order guaranteed via sorted arrays
- Hash tables use deterministic hash functions with fixed seed
- No pointer-based ordering or memory address dependencies
- Contact/constraint ordering sorted by stable IDs

### 3. Integration Determinism
- Fixed timestep (typically 1/60 sec)
- No adaptive timestepping in deterministic mode
- Integration order: broadphase → narrowphase → solver → integrator
- Body processing order sorted by body ID

### 4. Solver Determinism
- PGS iterations in stable order
- Island processing deterministically sorted
- Constraint assembly order fixed
- Warm-start from previous frame (optional, but deterministic)

### 5. Random Number Determinism
- Xorshift32 PRNG with known seed
- Separate RNG state per subsystem
- Replay stream captures RNG state

### 6. Parallel Determinism
- Task graph with explicit dependencies
- No race conditions via data partitioning
- Work distribution deterministic based on task ID
- Results combined in sorted order

## Replay System

### Replay Stream Format
```
Frame:  0 | Hash: 0x12345678 | Inputs: [position=(0,0,0), force=(0,-9.8,0)]
Frame:  1 | Hash: 0x23456789 | Inputs: [position=(0,-0.49,0), force=(0,-9.8,0)]
...
```

### Replay Validation
1. Capture input stream during recording
2. Serialize world state at key frames
3. Hash state at each frame (position, velocity, rotation)
4. Replay: reset to snapshot, apply inputs, compare hashes
5. Divergence triggers detailed diff report

## Testing Strategy

### Determinism Tests
- **Cross-toolchain validation**: Reproduce hash matches on your exact compiler/platform targets
- **Multi-run**: Run same simulation 1000 times → identical hashes
- **Replay**: Record 10-second simulation → replay → validate hash match
- **Rollback**: Rollback N frames → resimulate → verify convergence

### CI Validation
- CI includes determinism evidence lanes for Linux, Windows, and macOS.
- Repeated-run hash matching is enforced via the proof scripts.
- Cross-compiler outputs are captured for drift visibility.
- Use workflow artifacts as the source of truth for current validation coverage.

## Known Limitations

### Non-Deterministic by Default
- Multithreading (enable deterministic_parallel mode)
- System allocator (use UpeAllocator)
- Clock time (use frame counter)

### Platform Differences
- Ensure same compiler flags: `/fp:precise` (MSVC), `-ffp-contract=off` (GCC/Clang)
- Disable SIMD unless guaranteed bit-exact implementations
- x87 FPU (legacy) not supported - SSE2+ required

## Best Practices for Users

1. **Use Fixed Timestep**: Always call `UpeWorld_Step()` with same dt
2. **Seed RNG**: Call `UpeRng_Seed()` with known value before simulation
3. **Avoid System Inputs**: Use recorded inputs, not live player input during replay
4. **Validate Hashes**: Check `UpeReplay_ValidateHash()` periodically
5. **Snapshot Frequently**: Save state every N frames for rollback safety

## Performance vs. Determinism Trade-offs

| Feature | Deterministic Path | Fast Path |
|---------|-------------------|-----------|
| Broadphase | Sorted SAP | BVH with spatial hash |
| Parallel | Task graph | Work stealing |
| Math | Software scalar | SIMD intrinsics |
| Sleep | Disabled | Energy threshold |

**Recommendation**: Use deterministic mode for networked multiplayer, fast path for local simulation.
