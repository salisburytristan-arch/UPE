# Unified Physics Engine - Productionization Roadmap

This document maps the 150+ items from [EnhancementList.md](EnhancementList.md) into a strategic execution plan. The goal is to transform UPE from a working physics demo into a production-grade middleware solution with determinism guarantees, performance contracts, and comprehensive features.

## 1. Foundation Phase (In Progress)

### 1.1 Architecture & Productionization ✅ STARTED

**Goal**: Establish professional ABI boundaries and configuration infrastructure.

| Item | Status | File(s) | Notes |
|------|--------|---------|-------|
| Engine profiles (Game/Cinematic/Scientific) | ✅ Done | `upe_config.h/cpp` | Compile-time + runtime profile selection |
| Capability discovery (`upeGetCapabilities()`) | ✅ Done | `upe_config.h/cpp` | Feature bits, limits, threading model |
| Versioning infrastructure | ✅ Done | `upe_config.h/cpp` | Major.Minor.Patch with ABI checks |
| Standardized error handling | ✅ Done | `upe_error.h/cpp` | Error codes, thread-local context, diagnostic callback |
| Telemetry ring buffer | ✅ Done | `upe_telemetry.h/cpp` | Always-on performance counters, lock-free |
| Validated descriptor system | ✅ Done | `upe_descriptor.h/cpp` | `UpeWorldDesc`, `UpeSolverDesc`, `UpeBodyDesc` validation |

**Next Actions**:
- [ ] Wire error recording into existing API (body create, shape create, world step)
- [ ] Hook telemetry recording at broadphase/narrowphase/solve stages
- [ ] Update `UpeWorld_Create` to use `UpeWorldDesc` + validation
- [ ] Expose telemetry buffer in world handle

### 1.2 Module ABI Boundary 🔄 DESIGN PHASE

**Goal**: Hide implementation details behind opaque types; guarantee C API stability.

| Item | Status | File(s) | Notes |
|------|--------|---------|-------|
| Opaque world/body/shape handles | ⏳ Queued | TBD | Replace pointer exposure |
| Handle generation counting | ⏳ Queued | TBD | Prevent use-after-free |
| Function table versioning | ⏳ Queued | TBD | Enable future API evolution |
| Exclude STL from public headers | ⏳ Queued | `upe_api.h` | Already mostly clean |

### 1.3 Memory & Data Layout 🔄 IN PROGRESS

**Goal**: Replace malloc/free; introduce frame arenas for transient data.

| Item | Status | File(s) | Notes |
|------|--------|---------|-------|
| Deterministic scalar seam (Q48.16) | ✅ Done | `upe_scalar.h`, `upe_fixed_q48_16.h/cpp` | Float ↔ Q48.16 compile-time switch |
| Scalar math foundation | ✅ Done | `upe_math_scalar.h/cpp` | Vec2/Vec3 operations |
| Frame arena allocator | ⏳ Queued | TBD | Bump allocator, reset per step |
| SoA layouts for bodies/contacts | ⏳ Queued | TBD | Separate arrays for position/velocity/inertia |
| Chunked pool collections | ⏳ Queued | TBD | Fixed-size blocks + free lists |

## 2. Determinism & Validation Phase (Design)

### 2.1 Collision Pipeline ✅ FOUNDATION READY

**Goal**: Deterministic broadphase + narrowphase + batch collision.

| Item | Status | File(s) | Notes |
|------|--------|---------|-------|
| Scalar narrowphase kernels (6 pairs) | ✅ Done | `upe_narrowphase_scalar.h/cpp` | AABB/AABB, Sphere/Sphere, Sphere/AABB, Sphere/Capsule, Capsule/AABB, Capsule/Capsule |
| Deterministic SAP broadphase | ✅ Done | `upe_broadphase_scalar.h/cpp` | Lexicographic pair sorting |
| Collision layer/mask filtering | ✅ Done | `upe_broadphase_scalar.h/cpp` | SAP + pipeline filtered variants |
| Batch collision pipeline | ✅ Done | `upe_collision_pipeline_scalar.h/cpp` | BuildAabbPairs, SolveAabbPairs, DetectAabb |

**Next Actions**:
- [ ] Implement continuous collision detection (sweep shapes, TOI computation)
- [ ] Add persistent contact manifolds with feature-based clipping
- [ ] Integrate with warm-starting infrastructure

### 2.2 Determinism Validation 🔄 IN PROGRESS

**Goal**: Hash every frame; replay & cross-version compatibility.

| Item | Status | File(s) | Notes |
|------|--------|---------|-------|
| Simulation hashing | ⏳ Queued | TBD | Deterministic state canonicalization |
| Replay system | ⏳ Queued | TBD | Record input stream, playback |
| Cross-version strategy | ⏳ Queued | TBD | "strict" pinned mode vs "compatible" upgrade mode |

## 3. Collision & Constraint Expansion

### 3.1 Advanced Collision 📋 QUEUED

| Item | Effort | Impact | Priority |
|------|--------|--------|----------|
| Hybrid broadphase (SAP + BVH) | Medium | High | P1 |
| Convex decomposition pipeline | Medium | High | P1 |
| Continuous collision detection | Medium | High | P1 |
| GJK/EPA robustness | Medium | Medium | P2 |
| Signed distance fields | High | medium | P2 |
| Heightfield collision | Medium | High | P2 |

### 3.2 Constraint Solving 📋 QUEUED

| Item | Effort | Impact | Priority |
|------|--------|--------|----------|
| Unify constraints (Jacobian-based) | High | High | P1 |
| Warm-starting infrastructure | Medium | High | P1 |
| Shock propagation / split impulse | Medium | Medium | P2 |
| Motor constraints with limits | Medium | High | P2 |
| Articulation solver (Featherstone) | High | High | P3 |
| Contact graph sleeping | Medium | Medium | P3 |

## 4. Multiphysics & Advanced Features

### 4.1 Particle Systems 📋 QUEUED

| Item | Effort | Impact | Priority |
|------|--------|--------|----------|
| Shared neighbor system (SPH base) | High | High | P2 |
| Pressure projection variants | High | Medium | P3 |
| Rigid ↔ Fluid coupling | Medium | High | P2 |
| Granular materials | Medium | Medium | P3 |

### 4.2 Deformable Objects 📋 QUEUED

| Item | Effort | Impact | Priority |
|------|--------|--------|----------|
| XPBD cloth/hair | High | High | P2 |
| Fracture + debris | High | High | P3 |
| FEM deformable solids | Very High | Medium | P4 |

### 4.3 Specialized Domains 📋 QUEUED

| Item | Effort | Impact | Priority |
|------|--------|--------|----------|
| Vehicle dynamics (tire/suspension) | High | High | P2 |
| Character controller | Medium | High | P2 |
| Ragdoll authoring | Medium | High | P3 |
| 2D physics fast-path | Medium | Medium | P2 |

## 5. Performance & Parallelism

### 5.1 SIMD & Vectorization 📋 QUEUED

| Item | Effort | Impact | Priority |
|------|--------|--------|----------|
| SIMD math backends (SSE2/AVX2/NEON) | Medium | High | P3 |
| Packet collision queries | Medium | Medium | P4 |
| SIMD transpose layers | Medium | Medium | P4 |

### 5.2 Threading & GPU 📋 QUEUED

| Item | Effort | Impact | Priority |
|------|--------|--------|----------|
| Job system with work-stealing | High | High | P3 |
| GPU broadphase (compute shader) | High | High | P4 |
| Deterministic task graph scheduling | Medium | High | P3 |

## 6. Profiling, Debugging & Tools

### 6.1 Visualization & Inspection 📋 QUEUED

**Goal**: In-engine debugging tools for developers.

| Item | Effort | Impact | Priority |
|------|--------|--------|----------|
| Debug visualization API | Low | High | P1 |
| Physics inspector (JSON dump) | Low | High | P1 |
| Constraint force reporting | Low | Medium | P2 |

### 6.2 Testing & Benchmarking 📋 QUEUED

| Item | Effort | Impact | Priority |
|------|--------|--------|----------|
| Regression test suite (golden scenes) | Medium | High | P1 |
| Benchmark dashboards (CSV/JSON) | Low | High | P2 |
| Fuzzing harness | Medium | Medium | P3 |

### 6.3 Bindings & Integration 📋 QUEUED

| Item | Effort | Impact | Priority |
|------|--------|--------|----------|
| Auto-generated C#/Rust/Python bindings | Medium | High | P1 |
| Conformance test suite per binding | Medium | High | P1 |
| Engine integration examples (Unity/Unreal/Godot) | Medium | High | P2 |

### 6.4 CI/CD Infrastructure 📋 QUEUED

| Item | Effort | Impact | Priority |
|------|--------|--------|----------|
| Regression gate (determinism hashes) | Medium | High | P1 |
| Sanitizer builds (ASan/UBSan/TSan) | Low | High | P2 |
| Performance regression detection | Medium | Medium | P3 |

## 7. Educational & Documentation

### 7.1 Developer Documentation ✅ IN PROGRESS

| Item | Status | File(s) | Notes |
|------|--------|---------|-------|
| Build guide | ✅ Done | `docs/BUILD_GUIDE.md` | CMake + platform-specific |
| Architecture overview | ✅ Done | `docs/ARCHITECTURE.md` | Modular design, data flow |
| Determinism guide | ✅ Done | `docs/DETERMINISM.md` | Guarantees + challenges |
| Fixed-point migration | ✅ Done | `docs/FIXED_POINT_MIGRATION.md` | Q48.16 seam explanation |
| API reference | ⏳ Queued | `docs/API_REFERENCE.md` | Auto-generated from headers |
| Integration guide | ⏳ Queued | TBD | Real-world usage patterns |

### 7.2 Reference Examples 📋 QUEUED

| Item | Effort | Impact | Priority |
|------|--------|--------|----------|
| Minimal 2D hello-world | Low | High | P1 |
| Minimal 3D hello-world | Low | High | P1 |
| Stack stability | Low | High | P1 |
| Ragdoll physics | Medium | Medium | P2 |
| Vehicle dynamics | Medium | Medium | P2 |
| Fluid interaction | Medium | Medium | P2 |
| Determinism certification | Medium | High | P3 |

## Execution Priority Ranking

### Phase A: Immediate (This Sprint)
High-visibility professionalization that unblocks bindings and buyer confidence:

1. **Error handling integration** - Wire into all API boundaries (1-2 days)
2. **Telemetry wiring** - Record at each pipeline stage (1 day)
3. **Descriptor wiring** - Replace hard-coded defaults with validated descriptors (1 day)
4. **API header cleanup** - Expose new modules through `upe_api.h` (0.5 day)
5. **Documentation updates** - Reflect new modules in README + API reference (1 day)

### Phase B: Foundation Hardening (Week 2-3)
| Item | Effort | Blocker | ROI |
|------|--------|---------|-----|
| Contact manifold caching | Medium | CCD | High |
| Warm-starting implementation | Medium | Solver | High |
| Continuous collision detection | Medium | Stability | High |
| Frame arena refactoring | Medium | Performance | High |
| Handle indirection layer | Medium | Bindings | High |

### Phase C: Feature Expansion (Week 4+)
- Vehicle dynamics
- Character controller
- Ragdoll authoring
- Fluid coupling
- 2D fast-path

## Execution Metrics

Track progress via:

1. **Coverage**: Number of EnhancementList items complete / 150
2. **Determinism**: Commit-to-hash consistency across platforms
3. **Performance**: Benchmark FPS @ (100 bodies, 1000 contacts, 4 solver iterations)
4. **Quality**: Test pass rate across unit + regression + fuzzing suites
5. **Documentation**: Code coverage % with API docs

---

**Last Updated**: After foundational infrastructure (config, error, telemetry, descriptor) committed.

**Next Review**: After Phase A completion and telemetry integration into world step.

