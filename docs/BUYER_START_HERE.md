# Buyer Start Here (15-Minute Evaluation Path)

Headline: **Deterministic rollback-ready physics core (C API, no dependencies) with snapshots, replay validation, state hashing, and proven solver convergence.**

## What You're Evaluating

This is the UPE v1.0.0 **core physics engine**:
- **Rigid body dynamics**: collision detection, constraint solving, integration
- **Determinism**: bit-exact reproducibility across builds and platforms (with `DUPE_DETERMINISTIC=ON`)
- **Rollback support**: snapshots, replay, hashing for netcode/time-travel debugging
- **Zero mandatory dependencies**: pure C implementation using stdlib only
- **Production quality**: warm-starting, manifold caching, island sleeping, CCD

Optional modules (beyond v1.0.0 scope): soft bodies, fluids, thermal, plasma, fracture, orbital mechanics.

## 1) Build (core-focused)

### Windows (MSVC)
```bat
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUPE_BUILD_EXAMPLES=ON -DUPE_BUILD_TESTS=ON -DUPE_DETERMINISTIC=ON
cmake --build build --parallel
```

### Linux/macOS
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUPE_BUILD_EXAMPLES=ON -DUPE_BUILD_TESTS=ON -DUPE_DETERMINISTIC=ON
cmake --build build -j$(nproc)
```

## 2) Run Unit Tests (60+ test cases)
```bat
ctest --test-dir build --output-on-failure -C Release
```
**What this validates**: Collision detection, joint solving, determinism hashing, snapshot/replay, vehicle dynamics, character controller, animation IK.

## 3) Run Determinism Proof (bit-exact check)
```bat
tools\run_determinism_proof.bat build
```
**What this demonstrates**: 
- Same scene simulated twice produces identical frame-by-frame state.
- CRC64 hashes match across runs (no floating-point drift).
- Proof logs stored in `proof/RUNS/determinism_*.log`.

## 4) Run Seven Core Examples

### Minimal 3D (rigid bodies, basic collision)
```bat
build\bin\examples\minimal_3d.exe
```
Shows: Two falling spheres colliding, basic contact response. – **Foundation test.**

### Collision Demo (PGS solver tuning)
```bat
build\bin\examples\collision_demo.exe
```
Shows: Two spheres with configurable solver iterations. Demonstrates transient penetration behavior and iterative convergence. Solver iterations increased to 24 for tighter stability.  – **Solver quality baseline.**

### Vehicle Demo (suspension, tire model, steering)
```bat
build\bin\examples\advanced_vehicle_demo\main.exe
```
Shows: 4-wheel vehicle with world-aware suspension raycast, Pacejka tire model, Ackermann steering. Demonstrates force application at contact points and wheel angular velocity updates. – **Vehicle dynamics.**

### Character Controller Demo (raycast ground, stepping, platforms)
```bat
build\bin\examples\character_controller_demo\main.exe
```
Shows: Capsule character with raycast-driven ground detection, step-up logic, platform velocity inheritance. – **Character movement.**

### Ragdoll Demo (articulated bodies, joints, IK)
```bat
build\bin\examples\ragdoll_character_demo\main.exe
```
Shows: Skeleton with auto-created ball joints, ragdoll falling, IK solver pulling limbs toward targets. – **Articulated systems.**

### Cloth Demo (PBD, pinning, wind)
```bat
build\bin\examples\pbd_cloth_demo\main.exe
```
Shows: Cloth mesh with position-based dynamics, pinned points, wind forces. – **Soft body variant.**

### Fluid Demo (SPH, buoyancy, container interaction)
```bat
build\bin\examples\fluid_demo\main.exe
```
Shows: Particles simulated with SPH, floating objects, deterministic boundary handling. – **Fluid dynamics.**

## 5) Run Benchmarks & Release Gate

### Quick Build & Performance Check
```bat
python scripts\verify_everything.py --release-dir releases_ci --strict-warnings
```
**What this validates**:
- Strict C++20 warnings-as-errors build (no compiler escapes)
- Windows & Linux platform builds  
- All 60+ tests pass
- Examples run without crash
- Determinism proof passes
- Release package creation (buyout zip with sources)

**Outputs**:
- Log files in `proof/RUNS/`
- Release package in `releases_ci/UnifiedPhysicsEngine-1.0.0-buyout-*.zip`
- Full summary: PASS/FAIL per subsystem

## 6) Read Key Documentation

| Document | Purpose |
|-----------|----------|
| **API_REFERENCE.md** | Full C API with function signatures and parameter meanings |
| **IMPLEMENTATION_STATUS.md** | Detailed feature checklist (all phases 1-7) |
| **ARCHITECTURE.md** | Internal design: broadphase, narrowphase, solver, memory layout |
| **DETERMINISM_GUIDE.md** | How determinism works, how to use replay/snapshots |
| **INTEGRATION_GUIDE.md** | Practical patterns for embedding in your engine |
| **RELEASE_NOTES_v1.0.0.md** | Feature set summary and evaluation checklist |
| **KNOWN_LIMITATIONS.md** | Explicit constraints and current boundaries |

## 7) What You Should Conclude

✅ **Core pipeline builds and runs** (strict, deterministic, proven)
✅ **Determinism checks pass** (bit-exact, validated via proof tooling)
✅ **Examples exercise all major subsystems** (vehicles, characters, ragdolls, cloth, fluids)
✅ **Rollback/replay tooling exists and is measurable** (snapshots, hashing, playback)
✅ **Zero mandatory dependencies** (pure C, no external libs)
✅ **Production quality checks pass** (warnings errors, platform builds, all tests)

## 8) Why Choose UPE

**For Netcode/Rollback Games**: 
- Deterministic stepping, snapshot/restore, replay validation built-in.
- CRC64 hashing for quick state verification.

**For Simulation/Research**: 
- No dependency chains, portable C API.
- Extendable constraint system.
- Performance profiling and diagnostics.

**For General Physics Integration**: 
- Solid rigid-body foundation with proven collision/solver.
- Optional advanced domains (fluids, cloth, destruction) as side features.
- Production-grade determinism and serialization.

## Next Steps

1. **If evaluating for game integration**: Read `INTEGRATION_GUIDE.md` and study example bindings.
2. **If evaluating for determinism/netcode**: Run `tools/run_determinism_proof.bat` and read `DETERMINISM_GUIDE.md`.
3. **If evaluating specific domains** (vehicles, fluids, etc.): Review `IMPLEMENTATION_STATUS.md` and run corresponding examples.
4. **For licensing/commercial questions**: Review `COMMERCIAL_LICENSE_AGREEMENT.md` and `LICENSE`.
