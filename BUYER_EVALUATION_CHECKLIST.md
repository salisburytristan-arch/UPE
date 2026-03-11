# UNIFIED PHYSICS ENGINE - BUYER EVALUATION CHECKLIST

**Purpose**: Structured evaluation framework for qualified buyers  
**Duration**: 2-4 weeks typical  
**Audience**: Technical teams & decision makers

---

## PRE-EVALUATION PHASE (Before Repository Access)

- [ ] **Executive Sponsor**: Management approval and budget allocation
- [ ] **Technical Lead**: Assigned owner for evaluation
- [ ] **Use Case Definition**: 3-5 primary application scenarios identified
- [ ] **Platform Requirements**: Identify target OS/architecture
- [ ] **Scale Requirements**: Estimate max bodies/constraints needed
- [ ] **Timeline**: Determine integration window (typical: 3-6 months)
- [ ] **Support Model**: Confirm self-support acceptance

---

## INITIAL SETUP (Week 1)

### Access & Environment
- [ ] **NDA Execution**: Sign non-disclosure agreement
- [ ] **Repository Access**: Receive GitHub/private repo credentials
- [ ] **Documentation Review**: Read all `.md` files in root directory
  - [ ] README.md (5 min)
  - [ ] QUICK_START.md (10 min)
  - [ ] ARCHITECTURE.md (30 min)
  - [ ] COMMERCIALIZATION_GUIDE.md (20 min)
  - [ ] API_REFERENCE.md (ongoing reference)
  - [ ] DUE_DILIGENCE_PACK.md (40 min) ← Start here
  - [ ] TECHNICAL_ASSESSMENT.md (30 min)

### Environment Setup
- [ ] **Windows MSVC Build**:
  ```batch
  cd d:\UnifiedPhysicsEngine
  build_msvc.bat
  ```
  - Expected: ✅ All tests pass
  - Time: 5-10 minutes
  - Output: `build/` directory with compiled binaries

- [ ] **Alternative: CMake Build**:
  ```bash
  cmake -B cmake-build -DCMAKE_BUILD_TYPE=Release \
    -DUPE_BUILD_TESTS=ON -DUPE_BUILD_BENCHMARKS=ON
  cmake --build cmake-build --config Release
  ```

### Initial Testing
- [ ] **Run CTest Suite**:
  ```bash
  cd cmake-build
  ctest --output-on-failure
  ```
  - Expected: 66+ tests passing
  - Time: 1-2 minutes
  - Verify: Zero failures (`100% tests passed`)

- [ ] **Run Example Application**:
  ```bash
  cmake-build/Release/upe_minimal_3d.exe
  ```
  - Expected: Window opens with physics simulation
  - Verify: Smooth animation, no crashes

---

## TECHNICAL EVALUATION (Weeks 1-2)

### Code Quality Assessment

**Build System**:
- [ ] CMakeLists.txt exists and is modern (CMake 3.15+)
- [ ] Cross-platform support (Windows, Linux, macOS clauses)
- [ ] Compiler flags appropriate (`/fp:precise`, `-ffp-contract=off`)
- [ ] No external dependencies (verify `find_package` calls)
- [ ] CI/CD configuration exists (`.github/workflows/`)

**Code Organization**:
- [ ] Clear module structure in `core/include/upe/`
- [ ] 143+ headers (count: `ls core/include/upe/ | wc -l`)
- [ ] 130+ implementation files (count: `find core/src -name "*.cpp" | wc -l`)
- [ ] Consistent naming conventions (snake_case, prefixes)
- [ ] Documentation inline with functions

**Compilation**:
- [ ] **Zero compiler warnings** (or verify with):
  ```bash
  cmake --build cmake-build --config Release --verbose 2>&1 | grep -i warning
  # Should return no results
  ```
- [ ] Strict conformance enabled (`/permissive-`)
- [ ] No use of deprecated C++ features

### Test Coverage Review

**Test Execution**:
- [ ] Run full test suite with verbose output:
  ```bash
  ctest --output-on-failure --verbose
  ```
- [ ] Verify: **All 66+ tests pass**
- [ ] Check test categories:
  - [ ] Unit tests (memory, math, collision, solver)
  - [ ] Integration tests (world pipeline, multi-body)
  - [ ] Determinism tests (reproducibility)
  - [ ] Serialization tests (snapshots)

**Test Results Analysis**:
- [ ] Review `proof/RUNS/tests_*.log` for detailed results
- [ ] Identify any flaky tests (run 2-3 times)
- [ ] Check test execution time (should be <5 seconds total)

### Architecture Deep-Dive

**Core Systems** (Verify each exists):
- [ ] Allocation system (`upe_allocator.h`, `upe_arena.h`, etc.)
  - Arena, Pool, Freelist, Linear, Hybrid allocators
- [ ] Container system (`upe_array.h`, `upe_hashmap.h`, etc.)
  - Array, Hashmap, Ringbuffer types
- [ ] Math library (`upe_math.h`, `upe_vec3.h`, `upe_quat.h`)
  - Vectors, quaternions, matrices, transforms

**Physics Pipeline** (Review flow):
- [ ] `UpeWorld_Step()` implementation (main entry point)
  - Broadphase collision detection
  - Narrowphase contact generation
  - Constraint solver execution
  - Numerical integration
  - Post-step validation

**Advanced Domains** (Spot-check 3-5):
- [ ] Rigid body dynamics (`upe_rigid_body.h`)
- [ ] Soft body/cloth (`upe_cloth.h`, `upe_soft_body.h`)
- [ ] Fluids (`upe_fluid_sph.h`)
- [ ] Vehicles (`upe_vehicle_physics.h`)
- [ ] Ragdolls (`upe_ragdoll.h`)

### API Review

**Entry Point**:
- [ ] Understand `UpeWorld_Create()` → `UpeWorld_Step()` → `UpeWorld_Destroy()`
- [ ] Review 200+ API functions in `API_REFERENCE.md`
- [ ] Identify integration points for your use cases

**Key APIs by Use Case**:
- [ ] **Rigid bodies**: `UpeWorld_CreateBody()`, `UpeBody_SetVelocity()`, etc.
- [ ] **Constraints/Joints**: `UpeWorld_CreateConstraint()`, motor/limit setup
- [ ] **Queries**: `UpeWorld_Raycast()`, `UpeWorld_Overlap()`, etc.
- [ ] **Callbacks**: Trigger callbacks, contact callbacks
- [ ] **Serialization**: `UpeWorld_Snapshot()`, `UpeWorld_Restore()`

---

## PERFORMANCE EVALUATION (Week 2)

### Benchmark Suite Execution

**Run Canonical Scenes**:
```bash
cmake-build/Release/upe_canonical_scenes.exe
```

**Capture Results**:
- [ ] Sphere Stack (1000 bodies): Verify ~60+ FPS
- [ ] Ragdoll (50 joints): Verify ~60+ FPS
- [ ] Vehicle (10 bodies + suspension): Verify ~60+ FPS
- [ ] Cloth (10k particles): Verify ~60 FPS
- [ ] Fluid SPH (5000 particles): Verify ~45-60 FPS

**Generate Plots** (if Python available):
```bash
python scripts/plot_benchmarks.py benchmark_results.csv
# Generates HTML report with interactive charts
```

### Performance Profiling (Optional)

**Enable Profiler**:
```cpp
UpeWorldDesc desc = {};
desc.enable_profiling = true;
world = UpeWorld_Create(&desc);

// Run simulation...

// Dump Chrome tracing
UpeProfiler_DumpChromeTracing("trace.json");
```

**Analyze with Chrome**:
- Open `chrome://tracing` in Google Chrome
- Load `trace.json` file
- Identify performance bottlenecks

**Performance Targets vs. Your Requirements**:
- [ ] Does performance meet your FPS target?
- [ ] Memory usage within budget?
- [ ] Appropriate for your max body count?

### Memory Profiling

**Measure Memory Usage**:
- [ ] Baseline: Empty world
- [ ] Populated: 1000 bodies
- [ ] Large scene: 5000+ bodies
- [ ] Verify: Allocations tracked, no leaks

---

## DETERMINISM VALIDATION (Week 2)

### Determinism Certification Verification

**Manual Test** (Cross-run Reproducibility):
```cpp
// Create world
UpeWorld* world = UpeWorld_Create(&desc);

// Simulate and capture state hash
uint64_t hash1 = UpeWorld_GetStateHash(world);

// Destroy and recreate
UpeWorld_Destroy(world);
world = UpeWorld_Create(&desc);

// Simulate again and verify hash
uint64_t hash2 = UpeWorld_GetStateHash(world);

// Assert hash1 == hash2
assert(hash1 == hash2);
```

**Automated Test Suite**:
- [ ] Run `test_determinism_certification.cpp`
- [ ] Verify: All 4 determinism tests pass
- [ ] Review: `proof/RUNS/determinism_*.log`

**Results Expected**:
- [ ] ✅ Bit-exact reproducibility over 10 runs
- [ ] ✅ Deterministic after world reset
- [ ] ✅ No numerical drift detected
- [ ] ✅ Scalability maintained (5000+ bodies)

### Compiler Flags Verification

**MSVC Flags Check**:
```bash
cmake --build cmake-build --config Release --verbose 2>&1 | grep /fp:precise
# Should appear in output
```

**GCC/Clang Flags Check** (if Linux):
```bash
cmake --build cmake-build --config Release --verbose 2>&1 | grep ffp-contract
# Should appear in output
```

---

## EXAMPLE EVALUATION (Week 1)

### Run & Understand Example Applications

**Minimal Examples** (Start here):
- [ ] **minimal_3d**: Simple falling sphere
  - Lines: ~100
  - Concepts: World creation, body creation, stepping
  - Time: 10 min to understand
  
- [ ] **minimal_2d**: 2D physics (if applicable)
  - Alternative starting point for 2D projects

**Domain-Specific Examples** (Evaluate for your use case):
- [ ] **collision_demo**: Broadphase/narrowphase visualization
- [ ] **fluid_demo**: SPH fluid simulation
- [ ] **vehicle_demo**: Vehicle physics with suspension
- [ ] **cloth_demo**: Soft body cloth simulation
- [ ] **ragdoll_character_demo**: Articulated systems
- [ ] **advanced_movement_demo**: Complex dynamics

**Example Evaluation Template**:
```
Example Name: ___________
Purpose: ___________
Complexity Level: ☐ Beginner ☐ Intermediate ☐ Advanced
Code Lines: ___________
Compilation Time: ________ seconds
Execution Error: ☐ None ☐ Minor (describe) ☐ Critical
API Clarity: ☐ Clear ☐ Confusing
Applicability to Your Use Case: ☐ High ☐ Medium ☐ Low
Notes: ___________
```

### Code Walkthrough (Pick 1-2)

- [ ] Understand physics pipeline (trace `UpeWorld_Step()` call)
- [ ] Understand collision detection (review GJK/EPA)
- [ ] Understand constraint solver (review PGS iterations)
- [ ] Understand your domain of interest (rigid/soft/fluid/etc.)

---

## INTEGRATION FEASIBILITY (Week 2)

### Use Case Mapping

**For Each Key Use Case**:

**Use Case #1**: ___________
- [ ] **Complexity**: ☐ Low ☐ Medium ☐ High
- [ ] **Required Systems**: [List physics domains needed]
- [ ] **Integration Roadmap**: [Timeline estimate]
  - Phase 1 (Core): _____ weeks
  - Phase 2 (Optimization): _____ weeks
  - Phase 3 (Advanced): _____ weeks
- [ ] **Resource Estimate**: _____ FTE engineers
- [ ] **Risk Assessment**: [Technical risks identified]

**Use Case #2-5**: [Repeat above]

### Integration Prototype (Optional, Week 3)

If time permits, attempt minimal integration:
- [ ] Link UPE library to your test application
- [ ] Create simple physics world
- [ ] Add a few bodies and constraints
- [ ] Verify stepping works as expected
- [ ] Verify output matches expectations

**Expected Time**: 2-4 hours for experienced developer

---

## PLATFORM VERIFICATION (Week 2-3)

### Target Platform Testing

**Windows (MSVC)** - Expected: ✅ Pass
- [ ] Compile with `build_msvc.bat`
- [ ] Expected: 100% success
- [ ] Time: 5-10 minutes

**Linux (GCC/Clang)** - Expected: ⚠️ May need fixes
- [ ] Clone repo to Linux machine
- [ ] Run CMake and build
- [ ] If build fails:
  - [ ] Identify missing dependencies (likely none)
  - [ ] Identify compiler compatibility issues
  - [ ] Document fixes required
  - [ ] Estimate time to fix (typically 1-2 days)
- [ ] Run tests: `ctest --output-on-failure`
- [ ] Run benchmarks: `./upe_canonical_scenes`

**macOS (Clang)** - Expected: ⚠️ Unverified, likely minor issues
- [ ] Similar process to Linux
- [ ] Watch for path separators, endianness issues
- [ ] Estimate time to fix (typically 2-3 days)

**Document Findings**:
- [ ] Build success/failure
- [ ] Test pass rate
- [ ] Performance baseline
- [ ] Platform-specific issues found

---

## LICENSING & COMMERCIAL TERMS (Ongoing)

- [ ] **Review**: Existing licenses in `LICENSE` file
- [ ] **Clarify**: Licensing terms you require
  - [ ] Perpetual license?
  - [ ] Source code ownership?
  - [ ] Derivative rights?
  - [ ] Commercial usage rights?
  - [ ] Multi-platform rights?
  - [ ] Team size limits?
  - [ ] Revenue sharing?
- [ ] **Identify**: Support needs (if any)
- [ ] **Negotiate**: Terms with copyright holder

---

## FINAL DECISION PHASE (Week 3-4)

### Technical Assessment Summary

**Scorecard** (Rate 1-5, 5 = excellent):

| Category | Score | Notes |
|----------|-------|-------|
| Code Quality | ___/5 | |
| Functionality | ___/5 | |
| Performance | ___/5 | |
| Documentation | ___/5 | |
| Platform Support | ___/5 | |
| Determinism | ___/5 | |
| Extensibility | ___/5 | |
| **Overall** | ___/5 | |

**Total Score Recommendation**:
- ✅ 35-40: Recommend acquisition
- ✅ 30-34: Recommend acquisition with conditions
- ⚠️ 25-29: Recommend further evaluation
- ❌ <25: Recommend alternative

### Risk Assessment

**Technical Risks** (Rate 1-5, 1 = low risk):

| Risk | Rating | Mitigation |
|------|--------|-----------|
| Platform Porting | ___/5 | |
| Integration Complexity | ___/5 | |
| Performance Adequacy | ___/5 | |
| Maintenance Burden | ___/5 | |
| Customization Difficulty | ___/5 | |

### Stakeholder Sign-Off

- [ ] **Technical Lead**: Evaluation complete, sign-off
  ```
  Technical Lead: __________________ Date: __________
  Recommendation: ☐ Acquire ☐ Conditional ☐ Reject
  ```

- [ ] **Engineering Manager**: Resource allocation approved
  ```
  Manager: _________________________ Date: __________
  Budget Approved: ☐ Yes ☐ No
  Timeline: ________________________
  ```

- [ ] **Executive Sponsor**: Business case approval
  ```
  Executive: _______________________ Date: __________
  Approved: ☐ Yes ☐ No
  ```

---

## POST-ACQUISITION PHASE

Once acquired, follow this timeline:

**Month 1: Integration Kickoff**
- [ ] Form integration team (2-4 FTE recommended)
- [ ] Deep-dive architecture review (1 week)
- [ ] Design integration architecture (1 week)
- [ ] Begin API wrapper/binding layer (2 weeks)

**Months 2-3: Core Integration**
- [ ] Integrate core rigid body physics
- [ ] Test with minimal scenes
- [ ] Performance profiling on target platform
- [ ] Begin feature-specific integration

**Months 4-6: Advanced Features & Optimization**
- [ ] Integrate specialized domains (fluid, cloth, etc.)
- [ ] Performance optimization
- [ ] Multi-platform verification
- [ ] Production testing

**Months 6+: Launch & Maintenance**
- [ ] Internal support team training
- [ ] Production deployment
- [ ] Ongoing maintenance and updates

---

## QUESTIONS TO ASK (If Available)

**Architecture**:
- [ ] How does the coupling contract system handle circular dependencies?
- [ ] What happens if a domain exceeds its time budget in a pipeline stage?
- [ ] Can physics systems be hot-swapped at runtime?

**Physics**:
- [ ] What's the trade-off curve between solver iterations and accuracy?
- [ ] How does CCD work with sleeping bodies?
- [ ] What constraints are available for articulated systems?

**Performance**:
- [ ] What's the maximum recommended scene complexity?
- [ ] How does multithreading work, if available?
- [ ] Are there any algorithmic alternatives (e.g., direct solver vs. PGS)?

**Integration**:
- [ ] How do we integrate with [your game engine]?
- [ ] What's the recommended approach for custom physics?
- [ ] Can we integrate only core systems without full feature set?

**Support**:
- [ ] What post-acquisition support is available?
- [ ] How are bug reports handled?
- [ ] What's the update/maintenance plan?

---

## COMPLETION CHECKLIST

✅ **Complete evaluation when you've checked ALL of these**:

**Week 1**:
- [ ] Documentation reviewed
- [ ] Environment set up
- [ ] Build successful
- [ ] Tests passing
- [ ] Examples running

**Week 2**:
- [ ] Architecture understood
- [ ] Performance benchmarked
- [ ] Determinism validated
- [ ] Use cases mapped
- [ ] Platform verified

**Week 3-4**:
- [ ] Risk assessment complete
- [ ] Scorecard filled
- [ ] Stakeholder sign-offs obtained
- [ ] Licensing terms clarified
- [ ] Go/no-go decision made

---

**Evaluation Package Distributed**: February 2026  
**Recommended Evaluation Duration**: 2-4 weeks  
**Status**: Ready for buyer assessment teams

**Positioning Note**: Prioritize `production-ready` and `validated` capabilities for purchase-critical decisions; treat broader scientific/exotic modules as optional upside unless they map directly to your roadmap.

