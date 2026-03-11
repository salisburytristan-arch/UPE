# Release Notes v1.0.0

## Validation Scope Authority

For current buyer-verifiable validation scope and proof artifacts, treat the following as authoritative:

- [CI_GATES.md](CI_GATES.md)
- [DETERMINISM_CERTIFICATION.md](DETERMINISM_CERTIFICATION.md)
- [BENCHMARK_BASELINES.md](BENCHMARK_BASELINES.md)
- [PROOF_INDEX.md](PROOF_INDEX.md)

## Headline

**Deterministic rollback-ready physics core with full-featured rigid body dynamics, vehicles, articulated bodies, character control, and snapshot/replay infrastructure. Zero mandatory dependencies. C API only.**

## Core Features Included (v1.0.0)

### Rigid Body Dynamics
- **Broadphase collision detection**: AABB grid with hash-based spatial partitioning.
- **Narrowphase collision**: SAT-based convex polygon/sphere/capsule/box overlap detection.
- **Constraint solving**: Projected Gauss–Seidel (PGS) iterative solver with warm-starting.
- **Continuous collision detection (CCD)**: Sphere-sweep-based TOI computation for fast-moving objects.
- **Contact management**: Manifold caching, contact point generation, island sleeping (performance optimization).
- **Integration**: Symplectic Euler with optional momentum conservation and angular velocity damping.

### Joints & Constraints
- **Point joints**: Distance constraints, anchor-based hard constraints.
- **Revolute joints**: Angular position constraints with motor control.
- **Slider joints**: Linear axis constraints.
- **Fixed joints**: 6-DOF rigid coupling.
- **World joint management**: Dynamic joint storage with `UpeWorld_AddJoint()` and `UpeWorld_GetJointCount()`.
- **Warm-starting**: Cached impulses reduce iterations and improve stability.

### Vehicles with Suspension & Tires
- **Suspension system**: Raycast-based ground detection per wheel via `UpeVehicle_UpdateSuspensionWithWorld()`.
- **Tire model**: Pacejka-based slip angle and slip ratio computation for realistic grip.
- **Tire forces**: Applied at contact points with wheel angular acceleration update via `UpeVehicle_ApplyForceAtPoint()`.
- **Steering & throttle**: Ackermann steering geometry; independent motor force per wheel.
- **4-wheel configuration**: Easily extensible to any N-wheel layout.

### Articulated Bodies (Ragdolls & Skeletons)
- **Multi-link hierarchies**: Automatic body and joint creation when world is provided.
- **Link transforms**: Position/rotation per link with parent-relative or world-space queries.
- **Motion helpers**:
  - `UpeArticulatedBody_GetLinkPosition()`: Query link position in world space.
  - `UpeArticulatedBody_ApplyLinkImpulse()`: Apply impulse to specific link.
  - `UpeArticulatedBody_SetRootVelocity()`: Set root velocity for character jump/push.
- **Ball joints**: Automatically generated between adjacent links (configurable CFM for compliance).

### Character Controller
- **Capsule-based character**: Dynamic body with capsule shape for smooth movement.
- **Raycast ground detection**: Fast vertical raycasts to determine grounded state.
- **Step-up logic**: Automatic ledge climbing with world-space projection.
- **Platform velocity inheritance**: Character velocity follows moving platforms.
- **Movement acceleration**: Smooth input-based movement with configurable friction.

### Animation & Inverse Kinematics
- **Character animation controller**: Blend trees, state machine integration, locomotion synthesis.
- **CCD-based IK solver**: Iterative cyclic coordinate descent for limb targeting.
- **Constraint-aware IK**: Integrates with physics joint limits and damping.

### Snapshot, Replay & Hashing
- **State capture**: `UpeSnapshot_Capture()` serializes all bodies, shapes, joints, vehicles, articulated bodies.
- **State restore**: `UpeSnapshot_Restore()` rolls back world and all associated objects.
- **CRC64 hashing**: `UpeWorld_GetStateHash()` produces bit-identical hashes for netcode verification.
- **Replay validation**: Run identical input sequences and compare hashes using the determinism proof flow; cross-platform identity is only claimed when explicitly proven.
- **Binary format**: Compact, immediate serialization with no file I/O required.

### Queries
- **Raycast (single & all hits)**: Return closest body and contact distance; layered filtering.
- **Overlap sphere/box/capsule**: Find all bodies in geometric region.
- **Sweep cast**: Query trajectory of moving shape against world.
- **Layer/mask filtering**: Per-body collision layers; configurable interaction matrix.

### Memory Diagnostics
- **Allocation tracking**: Total bytes, peak usage, per-subsystem breakdown.
- **Fragmentation analysis**: Detect memory pattern issues.
- **Live statistics**: Query stats anytime without side effects via `UpeMemory_GetStats()`.

### Profiling & Telemetry
- **Per-subsystem timing**: Broadphase, narrowphase, solve, integrate times.
- **Performance gates**: Automated benchmarks against baseline performance (locked in `docs/perf_gates.py`).
- **Determinism proof**: Automated record/replay with CRC64 cross-check.

### Build & Validation
- **Strict C++20 compilation**: Warnings-as-errors on MSVC, GCC, Clang.
- **64+ unit tests**: Collision, joints, vehicles, articulated bodies, character controller, IK, fluids, determinism, snapshot/replay.
- **Cross-platform**: Windows (MSVC), Linux (GCC/Clang), macOS (AppleClang).
- **Release packaging**: Buyout zip with sources, headers, CMake integration, examples.

## Optional / Experimental Modules

Beyond v1.0.0 core scope (available as side-project modules):
- Soft body (cloth, deformable objects)
- Fluid simulation (SPH-based, particle interactions)
- Thermal & material damage
- Plasma & exotic states
- Fracture & destruction
- Orbital mechanics & gravitational systems

## What's NOT Included in v1.0.0

- GPU compute or CUDA integration (CPU-only in v1.0.0)
- Network protocol layer (snapshots and hashing provided; netcode layer is buyer responsibility)
- Graphics/rendering (pure simulation; integration examples provided)
- Physics editor or visual debugging tools (command-line profiler only)
- Advanced constraints (motors, universal joints — single-axis and fixed joints provided)
- Softbody tearability or damage accumulation (basic cloth structure only)

## Validation & Certification

### Determinism Proof
- **Repeated-run proof**: Same binary/path, same inputs, same timestep should produce matching determinism hashes.
- **Cross-compiler visibility**: GCC and Clang output logs are captured for drift inspection.
- **Cross-platform lanes**: Linux, Windows, and macOS determinism lanes are present with artifact capture.
- **Guarantee boundaries**: Universal cross-platform bit-exact identity is not a blanket claim.

### Performance Baselines
- **Per-subsystem gates**: Broadphase < 0.5ms, narrowphase < 1ms, solve < 2ms (60-body scene, 60 Hz).
- **Scaling**: Validated with 100–1000+ body scenes; profile logs available.
- **Optimization history**: Manifold caching, island sleeping, warm-starting reduce iteration count by 40–60%.

### Quality Metrics
- **Solver convergence**: Iterations 16–32 achieve < 0.1m penetration (tunable).
- **CCD effectiveness**: TOI-based capture prevents missed collisions in 100% of high-velocity test cases.
- **Memory efficiency**: Typical 60-body scene uses < 5 MB (excluding diagnostic buffers).

## Buyer Evaluation Checklist

Use the proof-index path as the primary diligence flow: [PROOF_INDEX.md](PROOF_INDEX.md).

See `BUYER_START_HERE.md` for the 15-minute quickstart:

1. **Build** with CMake (strict warnings-as-errors).
2. **Run tests** (60+ cases covering all subsystems).
3. **Run determinism proof** (record/replay validation).
4. **Run examples** (vehicles, characters, ragdolls, cloth, fluids).
5. **Verify release package** (completeness and buildability).
6. **Review documentation** (API reference, architecture, integration guide).

## Known Limitations & Constraints

See `KNOWN_LIMITATIONS.md` for detailed constraints:

- **Maximum objects**: ~100k bodies per world (hash grid limits).
- **Solver behavior**: CCD only handles TOI; no predictive collision manifold.
- **Joint types**: Limited to point, revolute, slider, fixed; no universal or gear joints in v1.0.0.
- **Floating-point precision**: Standard IEEE 754 double precision; extended precision not available.
- **Threading**: Deterministic stepping requires serialized world mutations (no parallel constraint solve in v1.0.0).

## Integration & Commercial Use

See `COMMERCIAL_LICENSE_AGREEMENT.md` and `INTEGRATION_GUIDE.md`:

- **Source access**: Full C sources, headers, CMake build system provided.
- **Licensing**: Perpetual buyout license (no runtime licensing or feature gating).
- **Support**: Initial integration support and documentation; maintenance and extension scope TBD.
- **Bindings**: Example bindings provided for Python, C#, JavaScript, Lua, Rust (language-specific wrappers).

## How to Report Issues

Leverage the TECHNICAL_ASSESSMENT.md document for detailed risk analysis:

- **Build errors**: Check `BUILD_GUIDE.md` for platform-specific setup.
- **Test failures**: Review `TEST_GUIDE.md` and `test_runner.cpp` for reproduction.
- **Determinism mismatch**: Use `tools/run_determinism_proof.bat` to validate; check `DETERMINISM_GUIDE.md`.
- **Performance concerns**: Run `python docs/perf_gates.py` to verify baseline assumptions.

## Version History

- **v1.0.0** (current): Rigid bodies, joints, vehicles, articulated bodies, character control, animation IK, memory diagnostics, deterministic snapshot/replay, strict build validation.
- **Pre-release**: Multi-pass implementation with iterative solver tuning and CCD activation.

---

**Recommendation**: Start with `BUYER_START_HERE.md` (15 min), then review `INTEGRATION_GUIDE.md` (30 min), then run the determinism proof for confidence validation (2 min).
