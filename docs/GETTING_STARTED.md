# Getting Started with Unified Physics Engine

Welcome! This guide will get you up and running with the Unified Physics Engine in minutes.

## Quick Start (5 Minutes)

### 1. Build the Engine

**Windows:**
```bat
build_msvc.bat
```

**Linux/macOS:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --output-on-failure
```

### 2. Run Tests

After building, verify everything works:

**Windows:**
```bat
build\bin\upe_tests.exe
```

**Linux/macOS:**
```bash
./build/bin/upe_tests
```

Expected output: All tests pass ✓

### 3. Try an Example

```bash
# Windows
build\bin\examples\basic_simulation.exe

# Linux/macOS
./build/bin/examples/basic_simulation
```

## Your First Simulation (10 Lines)

```cpp
#include "upe/upe_engine.h"

int main() {
    // Create physics engine
    UpeEngine *engine = UpeEngine_Create();
    UpeWorld *world = UpeEngine_GetWorld(engine);
    
    // Create a falling sphere
    UpeBodyDesc body = {0};
    body.type = UPE_BODY_TYPE_DYNAMIC;
    body.position = {0, 10, 0};
    body.mass = 1.0f;
    uint32_t sphere_id = UpeWorld_CreateBody(world, &body);
    
    // Simulate 100 frames (60 FPS)
    for (int i = 0; i < 100; i++) {
        UpeEngine_Step(engine, 1.0f/60.0f);
    }
    
    // Cleanup
    UpeEngine_Destroy(engine);
    return 0;
}
```

## What's Included

### ✅ Core Features (Phases 1-7 Complete)

- **Rigid Body Dynamics**: Full 6DOF simulation with constraints
- **Collision Detection**: Broadphase (SAP), narrowphase (GJK/EPA)
- **Advanced Physics**: Soft bodies, cloth, fluids (SPH), particles
- **Specialized Systems**: Vehicles, ragdolls, character controllers
- **Determinism**: Bit-exact reproducible across platforms
- **Performance**: Optimized for 60 FPS with thousands of bodies

### 📁 Repository Structure

```
UnifiedPhysicsEngine/
├── core/
│   ├── include/upe/        # 143+ public API headers
│   └── src/                # 130+ implementation files
├── examples/               # Working code samples
├── tests/                  # 66+ unit tests
├── docs/                   # Documentation (you are here)
└── CMakeLists.txt         # Build configuration
```

## Key Documentation

1. **[IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md)** - Complete feature list
2. **[BUILD_GUIDE.md](BUILD_GUIDE.md)** - Detailed build instructions
3. **[INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md)** - How to use in your project
4. **[../API_REFERENCE.md](../API_REFERENCE.md)** - API documentation
5. **[DETERMINISM.md](DETERMINISM.md)** - Reproducibility guarantees

## Common Use Cases

### Game Development
- Character physics and movement
- Vehicle simulation
- Ragdoll effects
- Destructible environments
- Particle effects

### Research & Education
- Physics simulation experiments
- Algorithm testing
- Educational demonstrations
- Comparative studies

### Engineering
- Mechanical simulation
- Fluid dynamics modeling
- Material stress testing
- Multi-physics coupling

## Configuration Options

The engine can be customized via `UpeEngineConfig`:

```cpp
UpeEngineConfig config = {
    .enable_rigid_bodies = 1,
    .enable_soft_bodies = 1,
    .enable_fluids = 1,
    .solver_iterations = 8,
    .gravity = {0, -9.81, 0},
    .deterministic_math = 1,
};

UpeEngine *engine = UpeEngine_CreateCustom(&config);
```

## Performance Expectations

Typical performance on a modern CPU (single-threaded):
- **1,000 bodies**: ~5ms per frame (200 FPS)
- **5,000 bodies**: ~15ms per frame (60 FPS)
- **10,000+ bodies**: Enable multithreading or islands

## Platform Support

- ✅ **Windows**: MSVC (tested)
- ✅ **Linux**: GCC, Clang
- ✅ **macOS**: Clang (Apple Silicon & Intel)
- ⚠️ **GPU Acceleration**: Not available (CPU-only)

## Build Requirements

- **CMake**: 3.15 or higher
- **C++ Compiler**: C++20 support required
  - MSVC 2019+
  - GCC 10+
  - Clang 10+

## Troubleshooting

**Build fails?**
- Ensure you have C++20 compiler
- Check [TROUBLESHOOTING.md](TROUBLESHOOTING.md)

**Tests fail?**
- Use Release build: `cmake --build build --config Release`
- See [TEST_GUIDE.md](TEST_GUIDE.md)

**Integration issues?**
- Review [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md)
- Check [KNOWN_LIMITATIONS.md](KNOWN_LIMITATIONS.md)

## Next Steps

1. ✅ **Build & test** - Verify everything compiles
2. 📖 **Read docs** - Check [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md) for features
3. 🔨 **Try examples** - Run example programs in `examples/`
4. 🚀 **Integrate** - Add to your project using [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md)

## Support & License

- **License**: See [../COMMERCIAL_LICENSE_AGREEMENT.md](../COMMERCIAL_LICENSE_AGREEMENT.md)
- **Documentation**: Start with [INDEX.md](INDEX.md)
- **No official support**: Self-service via documentation

---

**Ready to get started? Build the engine and run the tests!**
