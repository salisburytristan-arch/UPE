# Unified Physics Engine (UPE)

**A deterministic native simulation runtime for rollback-capable gameplay, auditable replay workflows, and dependency-light source integration.**

---

## 🚀 Overview & Value Proposition

Unified Physics Engine (UPE) is a buyer-verifiable physics foundation centered on:
- **Deterministic stepping, replay, snapshots, and state hashing**
- **C++20 implementation with a stable C API boundary**
- **No mandatory runtime dependencies** beyond the standard library
- **Clean native integration paths** for engine, server, or headless workflows
- **Structured proof artifacts and maturity docs** for technical diligence
- **Optional language bindings** for Python, C#, Rust, JavaScript/WASM, and Lua

### Product Structure
- **UPE Core** — rigid body, collision, constraints, CCD, determinism, replay/snapshots, hashing
- **UPE Applied Packs** — vehicles, character/ragdoll, cloth/softbody, fluids, rollback workflows
- **UPE Labs** — broader scientific and experimental domains kept separate from core production claims

**CONFIDENTIAL: Proprietary source for private evaluation, source licensing, or acquisition discussions.**

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![Tests](https://img.shields.io/badge/tests-all%20current%20tests%20passing-brightgreen)]()
[![License](https://img.shields.io/badge/license-Proprietary-red)]()

---

## 📊 Platform & Toolchain Support

| Platform         | Toolchain         | Status            | Evidence                                      |
|------------------|-------------------|-------------------|-----------------------------------------------|
| Windows x64      | MSVC              | verified-in-ci    | CI matrix + uploaded test logs/artifacts      |
| Linux x64        | GCC, Clang        | verified-in-ci    | CI matrix + strict warnings + proof logs      |
| macOS            | Apple Clang       | verified-in-ci    | CI matrix + build/test logs                   |
| Windows x64      | MinGW/GCC         | experimental      | Local build path, not official CI             |
| Linux ARM64      | GCC/Clang         | not-yet-validated | No active CI lane                             |
| macOS ARM64      | Apple Clang       | experimental      | Built via CI, no dedicated arch proof lane    |

See [Platform Support Matrix](docs/PLATFORM_SUPPORT_MATRIX.md) for full details.

---

## 🛡️ Determinism, Replay, and Validation

 **Determinism Proof**: Bitwise-reproducible simulation validated by CI and local scripts ([Determinism Certification](docs/DETERMINISM_CERTIFICATION.md)). Bitwise determinism requires `/fp:precise` (MSVC), `-ffp-contract=off` (GCC/Clang), and use of UpeAllocator. Cross-platform bit-exact is not guaranteed by default.

**Determinism Guarantee Levels:**
1. **Repeated-Run**: Guaranteed (same binary, same input, same output)
2. **Cross-Compiler**: Observed, not guaranteed (drift visible in CI logs)
3. **Cross-Platform**: Not guaranteed (per-platform proof only)

See [Determinism Certification](docs/DETERMINISM_CERTIFICATION.md) and [CI Gates](docs/CI_GATES.md) for full methodology.

---

## 🧩 Modules & Features

### Core Physics
- **Rigid Body Dynamics**: 6DOF, quaternion integration, stable handles
- **Joints**: Hinge, slider, ball-and-socket, 6DOF, distance, spring, gear, pulley, universal
- **Collision Detection**: Broadphase (SAP/BVH), narrowphase (GJK/EPA), trimesh, heightfield
- **Constraint Solver**: PGS iterative, XPBD, friction, restitution, warm-starting, stabilization
- **Continuous Collision Detection**: Sphere, box, capsule, mesh
- **Snapshot/Replay/Hashing**: Full world serialization, delta encoding, CRC64 state hash

### Advanced Domains
- **Soft Body**: XPBD cloth, ropes, pinning, wind, air damping
- **Fluids**: SPH (Smoothed Particle Hydrodynamics), surface tension, spray, boundaries
- **Particles**: Emitter pool, burst/continuous/timed, custom behaviors, gravity/wind
- **Thermodynamics**: Heat diffusion, radiation, combustion, thermal expansion
- **Waves**: 2D shallow water, grid-based simulation
- **Breakables**: Fracture mechanics, stress accumulation, break callbacks
- **Granular Materials**: Cohesion, angle-of-repose, particle system
- **Aerodynamics**: Drag, lift, Bernoulli, barometric, Mach effects
- **Orbital Mechanics**: N-body, Lagrange points, Hohmann transfer, tidal locking
- **Plasma Physics**: Lorentz force, electromagnetic fields
- **Chemistry**: Reaction-diffusion, combustion, corrosion, pH, pattern formation

### Production Infrastructure
- **Profiling**: Frame/zone timing, memory tracking, Chrome trace export
- **Telemetry**: Lock-free ring buffer, per-event stats, JSON dump
- **Error Handling**: Standardized codes, thread-local context, diagnostic callbacks
- **Capability Discovery**: `upeGetCapabilities()` for runtime feature introspection
- **Validated Descriptors**: `UpeWorldDesc`, `UpeSolverDesc`, `UpeBodyDesc` with helpers
- **Deterministic Scalar Seam**: Q48.16 fixed-point (opt-in)
- **Scalar Collision Pipeline**: Deterministic SAP + narrowphase + batch collision

### Developer Experience
- **Dependency-Light Runtime**: C++20 core with a stable C API boundary (stdlib only, no mandatory third-party runtime deps)
- **Small Footprint**: ~700KB optimized
- **Language Bindings**: Python, C#, Rust, JavaScript/WASM, Lua
- **Proof Surface**: in-repo tests, determinism scripts, and buyer-verifiable logs

---

## 🏗️ Build & Quick Start

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

### Run Tests
```bat
ctest --test-dir build --output-on-failure -C Release
```

### Run Determinism Proof
```bat
tools\run_determinism_proof.bat build
```

### Minimal Example (C++)
```cpp
#include "upe/upe_api.h"

int main() {
    UpeWorldDesc desc = UpeWorldDesc_Default();
    desc.fixed_dt = 1.0f / 60.0f;
    desc.enable_determinism_mode = 1;
    UpeWorld *world = UpeWorld_Create(&desc);
    for (int i = 0; i < 100; i++) UpeWorld_Step(world);
    UpeWorld_Destroy(world);
    return 0;
}
```

### Python Example
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

---

## 🧑‍💻 API Surface & Extension Points

### C/C++ API
- **World/Body/Shape/Joints**: Creation, destruction, stepping, queries
- **Replay/Snapshot/Hashing**: Deterministic state, CRC64, delta encoding
- **Advanced Domains**: Soft body, fluids, particles, thermodynamics, breakables, etc.
- **Profiling/Telemetry**: Per-frame stats, Chrome trace, memory tracking
- **Error Handling**: Diagnostic callbacks, severity filtering
- **Capability Discovery**: Runtime feature introspection

See `core/include/upe/upe_api.h` and related headers for full API, plus [docs/API_COOKBOOK.md](docs/API_COOKBOOK.md) for copy/paste integration patterns.

### Python Bindings
- Full world/body/step API, SPH fluids, particles, emitters, CCD, buoyancy, spray, and more ([bindings/python/upe_bindings.py](bindings/python/upe_bindings.py))

### Other Bindings
- C#, Rust, JavaScript/WASM, Lua (see `bindings/`)

---

## 🧪 Test, QA, and Proof Artifacts

- **CTest / QA Lanes**: included in-repo with proof logs and CI-backed artifacts
- **Determinism Proof**: CI and local scripts, repeated-run and cross-compiler logs
- **Benchmark Baselines**: Canonical scenes, performance gates ([docs/BENCHMARK_BASELINES.md](docs/BENCHMARK_BASELINES.md))
- **Proof Index**: All buyer-verifiable evidence ([docs/PROOF_INDEX.md](docs/PROOF_INDEX.md))

---

## 📈 Performance & Reproducibility


### Benchmark Results (Intel i7-12700K, RTX 3080)

| Bodies | FPS  |
|--------|------|
| 128    | 487  |
| 256    | 170  |
| 512    | 72   |
| 1,024  | 34   |

*Tested on Intel i7-12700K, RTX 3080*

---

## 🛠️ Extensibility & Integration

- **Modular C API**: Add new domains, constraints, or solvers
- **Custom Telemetry/Profiling**: Integrate with your tools
- **Network/Replay Hooks**: Plug into netcode, rollback, or time-travel debugging
- **Bindings**: Extend or wrap for new languages

---

## 📚 Documentation & References

- [Start Here](START_HERE.md)
- [Buyer Package Contents](BUYER_PACKAGE_CONTENTS.md)
- [Module Stability](MODULE_STABILITY.md)
- [Limitations](LIMITATIONS.md)
- [Buyer 15-minute path](docs/BUYER_START_HERE.md)
- [Implementation Status](docs/IMPLEMENTATION_STATUS.md)
- [Feature Matrix](docs/FEATURE_MATRIX.md)
- [Architecture Overview](docs/ARCHITECTURE.md)
- [Determinism Guide](docs/DETERMINISM_GUIDE.md)
- [Determinism Certification](docs/DETERMINISM_CERTIFICATION.md)
- [CI Gates](docs/CI_GATES.md)
- [Benchmark Baselines](docs/BENCHMARK_BASELINES.md)
- [Proof Index](docs/PROOF_INDEX.md)
- [Platform Support Matrix](docs/PLATFORM_SUPPORT_MATRIX.md)
- [Platform Matrix](proof/PLATFORM_MATRIX.md)
- [CI Determinism Report](proof/CI_DETERMINISM_REPORT.md)
- [API Stability Policy](docs/API_STABILITY_POLICY.md)
- [Provenance and Ownership](docs/PROVENANCE_AND_OWNERSHIP.md)
- [Purchase Options](docs/PURCHASE_OPTIONS.md)
- [Build Status](BUILD_STATUS.md)
- [Performance Gates](docs/perf_gates.py)

---

## 📝 Roadmap

- [x] Phase 0: Foundation (build, math, testing, replay)
- [x] Phase 1: Rigid body collision and constraints
- [x] Phase 2: Soft body (cloth, compounds, character controller)
- [x] Phase 3: Fluids, particles, thermodynamics, waves
- [x] Phase 4: Advanced domains (breakables, granular, aero, plasma, etc.)
- [x] Phase 5: SDK (bindings, QA scenes, performance gates)
- [ ] Phase 6: Optional high-tier features (AMR grids, quantum, relativistic)
- [ ] Phase 7: Production polish (profiler, visualizer, level editor integration)

---

## 🤝 Contributing

Contributions welcome! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

---

## ⚖️ License

Proprietary / All Rights Reserved – see [LICENSE](LICENSE) for details.

---

## 📦 Legal & Distribution

- [Commercial Terms](COMMERCIAL_LICENSE_AGREEMENT.md)
- [Distribution Policy](DISTRIBUTION_POLICY.md)
- [IP Chain-of-Title Checklist](IP_CHAIN_OF_TITLE_CHECKLIST.md)

---

## 🙏 Acknowledgments

Inspired by:
- Box2D (Erin Catto)
- Bullet Physics (Erwin Coumans)
- PhysX (NVIDIA)
- Jolt Physics (Jorrit Rouwe)

---

## 📬 Contact

- GitHub: [UnifiedPhysicsEngine](https://github.com/yourorg/UnifiedPhysicsEngine)
- Issues: [Issue Tracker](https://github.com/yourorg/UnifiedPhysicsEngine/issues)
