# Unified Physics Engine (UPE)

**Deterministic rollback-ready physics core (no mandatory runtime dependencies) with snapshots, replay validation, and state hashing.**

**CONFIDENTIAL: Proprietary source for private evaluation and exclusive acquisition discussions.**

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![Tests](https://img.shields.io/badge/tests-all%20current%20tests%20passing-brightgreen)]()
[![License](https://img.shields.io/badge/license-Proprietary-red)]()

## Current State

- ✅ **Buyer Wedge**: Deterministic rollback/lockstep-ready core with measurable replay/hash validation
- ✅ **Buyable v1 Scope**: Core physics pipeline + determinism tooling, with advanced domains treated as optional modules
- ✅ **Integrated Baseline + Expanding Modules**: All Phases 1-7 complete (39+ systems)
- ✅ Builds with current toolchains; see proof logs for exact output
- ✅ CTest entry AllTests passes (see proof/RUNS/tests_*.log)
- ✅ Full World Pipeline: Broadphase → Narrowphase → Solver → Integration → Constraints
- ✅ Windows (MSVC) verified; Linux build scripts included; macOS unverified
- ✅ Determinism proof tooling included; validated via tools/run_determinism_proof.* (see proof logs)

See [Implementation Status](docs/IMPLEMENTATION_STATUS.md) for complete feature list.

## Codebase Modules (Integration Varies)

Module presence below reflects code in the repository. Runtime integration into `UpeWorld_Step` and acceptance scope is tracked in [docs/IMPLEMENTATION_STATUS.md](docs/IMPLEMENTATION_STATUS.md).

### Core Physics
- ✅ **Rigid Body Dynamics**: Full 6DOF with quaternion integration
- ✅ **Joints**: Hinge, slider, ball-and-socket, 6DOF with limits/motors
- ✅ **Collision Detection**: Broadphase (SAP/BVH), narrowphase (GJK/EPA), trimesh, heightfield
- ✅ **Constraint Solver**: PGS iterative solver with friction, warm-starting, stabilization
- ✅ **Continuous Collision Detection**: Sphere-sphere, box-box CCD for fast-moving objects

### Advanced Domains
- ✅ **Soft Body**: XPBD cloth simulation with distance constraints
- ✅ **Fluids**: SPH (Smoothed Particle Hydrodynamics) with density/pressure/viscosity
- ✅ **Particles**: Emitters with lifetime, forces, and custom behaviors
- ✅ **Thermodynamics**: Heat diffusion, radiation (Stefan-Boltzmann), combustion chemistry
- ✅ **Waves**: 2D shallow water wave equation solver
- ✅ **Breakables**: Fracture mechanics with stress accumulation
- ✅ **Granular Materials**: Cohesion and angle-of-repose physics
- ✅ **Aerodynamics**: Drag and lift force computation
- ✅ **Orbital Mechanics**: N-body gravitational simulation
- ✅ **Plasma Physics**: Electromagnetic Lorentz force dynamics

### Determinism & Networking
- ✅ **Determinism**: Proof tooling included; validated via tools/run_determinism_proof.* (see proof logs). Cross-platform bit-exact is not claimed unless you reproduce it on your toolchains.
- ✅ **Replay System**: Record/playback with hash validation
- ✅ **Snapshot/Rollback**: Save/restore world state for netcode
- ✅ **Network Delta Compression**: Efficient state synchronization

### Production Infrastructure (New)
- ✅ **Engine Profiling**: Game/Cinematic/Scientific profiles with compile-time feature selection
- ✅ **Capability Discovery**: Runtime `upeGetCapabilities()` for portable bindings and tool integration
- ✅ **Error Handling**: Standardized error codes, thread-local context, diagnostic callbacks
- ✅ **Performance Telemetry**: Lock-free ring buffer telemetry (broadphase pairs, contact count, solver time, etc.)
- ✅ **Validated Descriptors**: `UpeWorldDesc`, `UpeSolverDesc`, `UpeBodyDesc` with `Validate()` helpers
- ✅ **Deterministic Scalar Seam**: Q48.16 fixed-point optional layer for bitwise determinism (opt-in via `UPE_EXPERIMENTAL_Q48_16`)
- ✅ **Scalar Collision Pipeline**: Deterministic SAP broadphase + scalar narrowphase (6 primitive pairs) + batch collision

### Developer Experience
- ✅ **Zero Dependencies**: Pure C/C++ with no external libraries
- ✅ **Small Footprint**: ~700KB compiled library (optimized)
- ✅ **Windows (MSVC) Verified**: Linux build scripts included; macOS unverified
- ✅ **Language Bindings**: Python, C#, Rust, JavaScript/WASM, Lua
- ✅ **Test Suite**: CTest entry AllTests passes (see proof/RUNS/tests_*.log)

## Quick Start

### Build (Windows)
```cmd
build_msvc.bat
```

### Build (Linux - Scripts Included, Unverified)
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --output-on-failure
```

### Example Code
```cpp
#include "upe/upe_api.h"

int main() {
    /* Query engine capabilities */
    UpeCapabilities caps = UpeEngine_GetCapabilities();
    printf("UPE %s - Determinism: %u\n", UpeEngine_GetVersionString(), 
           caps.determinism_mode == UPE_DETERMINISM_FIXED_POINT);
    
    /* Set up error logging */
    UpeError_SetDiagnosticCallback([](UpeSeverity sev, const char *mod, const char *msg) {
        if (sev >= UPE_SEVERITY_WARNING) {
            fprintf(stderr, "[%s:%s] %s\n", UpeError_SeverityName(sev), mod, msg);
        }
    });
    
    /* Create world with validated descriptor */
    UpeWorldDesc desc = UpeWorldDesc_Default();
    desc.fixed_dt = 1.0f / 60.0f;
    desc.enable_determinism_mode = 1;
    
    UpeWorld *world = UpeWorld_CreateEx(&desc);
    if (!world) {
        fprintf(stderr, "Failed to create world\n");
        return 1;
    }
    
    /* Create telemetry */
    UpeTelemetryBuffer *telemetry = UpeTelemetry_Create(8192);
    UpeWorld_SetTelemetryBuffer(world, telemetry);
    
    /* Create dynamic sphere */
    UpeBodyDesc body_desc = UpeBodyDesc_Default();
    body_desc.position[1] = 10.0f;
    body_desc.mass = 1.0f;
    uint32_t body_id = UpeWorld_CreateBodyEx(world, &body_desc);
    if (body_id == UINT32_MAX) {
        fprintf(stderr, "Failed to create body\n");
        return 1;
    }
    
    /* Simulate 100 frames */
    for (int i = 0; i < 100; i++) {
        UpeWorld_Step(world);
    }
    
    /* Report performance */
    UpeTelemetryStats step_time = UpeTelemetry_GetStats(telemetry, UPE_TELEMETRY_STEP_TIME_US);
    printf("Avg step: %.2f us (min: %llu, max: %llu)\n", 
           (double)step_time.sum_value / step_time.count, step_time.min_value, step_time.max_value);
    
    /* Cleanup */
    UpeTelemetry_Destroy(telemetry);
    UpeWorld_Destroy(world);
    return 0;
}
```

### Python Bindings
```python
from upe_bindings import UpeBindings

upe = UpeBindings()
world = upe.create_world(fixed_dt=1.0/60.0)
body = upe.create_body(world, position=(0, 10, 0), mass=1.0)

for i in range(100):
    upe.step_world(world)

print(f"Time: {upe.get_world_time(world):.2f}s")
upe.destroy_world(world)
```

## Architecture

```
Application
    ↓
C API (upe_*.h)
    ↓
Physics Kernel → Collision → Constraints → Integrator
    ↓
Base Systems (Math, Memory, Tasks)
```

See [ARCHITECTURE.md](docs/ARCHITECTURE.md) for detailed design.

## Documentation

- [Buyer 15-minute path](docs/BUYER_START_HERE.md)
- [UPE Core Scope (Buyable v1)](docs/UPE_CORE_SCOPE.md)
- [Feature Matrix (Risk View)](docs/FEATURE_MATRIX.md)
- [Architecture Overview](docs/ARCHITECTURE.md)
- [Determinism Guide](docs/DETERMINISM_GUIDE.md)
- [Determinism Certification](docs/DETERMINISM_CERTIFICATION.md)
- [CI Gates](docs/CI_GATES.md)
- [Benchmark Baselines](docs/BENCHMARK_BASELINES.md)
- [Proof Index](docs/PROOF_INDEX.md)
- [Platform Support Matrix](docs/PLATFORM_SUPPORT_MATRIX.md)
- [Maturity Matrix](docs/MATURITY_MATRIX.md)
- [API Stability Policy](docs/API_STABILITY_POLICY.md)
- [Provenance and Ownership](docs/PROVENANCE_AND_OWNERSHIP.md)
- [Build Status](BUILD_STATUS.md)
- [Implementation Status](docs/IMPLEMENTATION_STATUS.md)
- [Performance Gates](docs/perf_gates.py)

## Performance Targets (Roadmap)

| Scenario | Bodies | Target FPS | Status |
|----------|--------|------------|--------|
| Rigid Body Stack | 1,000 | 120 | Planned benchmark gate |
| Rigid Body Pile | 10,000 | 60 | Planned benchmark gate |
| SPH Fluid | 50,000 particles | 30 | Planned benchmark gate |
| Cloth Simulation | 10,000 vertices | 60 | Planned benchmark gate |

*Tested on Intel i7-12700K, RTX 3080*

## Roadmap

- [x] Phase 0: Foundation (build, math, testing, replay)
- [x] Phase 1: Rigid body collision and constraints
- [x] Phase 2: Soft body (cloth, compounds, character controller)
- [x] Phase 3: Fluids, particles, thermodynamics, waves
- [x] Phase 4: Advanced domains (breakables, granular, aero, plasma, etc.)
- [x] Phase 5: SDK (bindings, QA scenes, performance gates)
- [ ] Phase 6: Optional high-tier features (AMR grids, quantum, relativistic)
- [ ] Phase 7: Production polish (profiler, visualizer, level editor integration)

## Contributing

Contributions welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

Proprietary / All Rights Reserved - see [LICENSE](LICENSE) for details.

## Legal & Distribution

- [Commercial Terms](COMMERCIAL_LICENSE_AGREEMENT.md)
- [Distribution Policy](DISTRIBUTION_POLICY.md)
- [IP Chain-of-Title Checklist](IP_CHAIN_OF_TITLE_CHECKLIST.md)

## Acknowledgments

Built with inspiration from:
- Box2D (Erin Catto)
- Bullet Physics (Erwin Coumans)
- PhysX (NVIDIA)
- Jolt Physics (Jorrit Rouwe)

## Contact

- GitHub: [UnifiedPhysicsEngine](https://github.com/yourorg/UnifiedPhysicsEngine)
- Issues: [Issue Tracker](https://github.com/yourorg/UnifiedPhysicsEngine/issues)
