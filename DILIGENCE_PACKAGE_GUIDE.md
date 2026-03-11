# DILIGENCE PACKAGE GUIDE
## Unified Physics Engine v1.0.0 – Exclusive Evaluation

**Package Date**: February 26, 2026  
**Version**: 1.0.0 (Buyer Evaluation Package)  
**Evaluation Period**: [INSERT: 30-90 days from date]  
**Confidentiality**: Subject to signed NDA

Validation authority: treat these documents as canonical for current claim scope:
- `docs/PROOF_INDEX.md`
- `docs/CI_GATES.md`
- `docs/DETERMINISM_CERTIFICATION.md`
- `docs/PLATFORM_SUPPORT_MATRIX.md`
- `docs/TRUTH_HIERARCHY.md`

---

## 📦 WHAT YOU'VE RECEIVED

Thank you for signing the NDA and advancing to the diligence phase. This package contains everything you need to perform a comprehensive technical and business evaluation of the Unified Physics Engine.

### Included Archives

| File | Size | Contents |
|------|------|----------|
| **UnifiedPhysicsEngine-1.0.0-buyout-2026-02-26.zip** | 0.93 MB | Complete source code, examples, tests, build system, and documentation |

**Verification**:
- Extract and verify SHA256 hash: [INSERT HASH] 
- Expected file count: 694 files across 30+ directories
- All files are plain text or small binary assets (no obfuscation or license checks)

---

## 🎯 QUICK START (60 MINUTES)

Follow this path to build and run your first validation:

### Step 1: Extract the Package (5 min)

```bash
# Windows (PowerShell)
Expand-Archive -Path UnifiedPhysicsEngine-1.0.0-buyout-2026-02-26.zip -DestinationPath C:\Physics

# Linux/macOS
unzip UnifiedPhysicsEngine-1.0.0-buyout-2026-02-26.zip -d ~/Physics
cd ~/Physics/UnifiedPhysicsEngine
```

### Step 2: Read BUYER_START_HERE.md (15 min)

```
docs/BUYER_START_HERE.md  ← START HERE FIRST
```

This is your 15-minute executive roadmap. It covers:
- What the engine includes
- How to build it
- Which examples to run first
- What to conclude from the evaluation

### Step 3: Build the Engine (20 min)

**Windows (MSVC)**:
```batch
cd C:\Physics\UnifiedPhysicsEngine
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUPE_BUILD_EXAMPLES=ON -DUPE_BUILD_TESTS=ON -DUPE_DETERMINISTIC=ON
cmake --build build --parallel
```

**Linux/macOS**:
```bash
cd ~/Physics/UnifiedPhysicsEngine
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUPE_BUILD_EXAMPLES=ON -DUPE_BUILD_TESTS=ON -DUPE_DETERMINISTIC=ON
cmake --build build -j$(nproc)
```

**Expected Output**:
```
[114/114] Linking CXX executable...
Build files have been written to: <path>/build
```

No errors, no warnings. If you see build errors, check **TROUBLESHOOTING.md**.

### Step 4: Run Tests (10 min)

```bash
ctest --test-dir build --output-on-failure -C Release
```

**Expected Output**:
```
100% tests passed, 0 tests failed out of 1
Total Test time (real) = 0.13 sec
```

### Step 5: Run Determinism Proof (10 min)

```bash
# Windows
tools\run_determinism_proof.bat build

# Linux/macOS
bash tools/run_determinism_proof.sh build
```

**Expected Output**:
```
[UPE] Determinism Proof: PASS
Frame hashes match across runs: ✅
```

---

## 📚 WHAT'S IN THE PACKAGE

### Directory Structure

```
UnifiedPhysicsEngine-1.0.0-buyout/
├── core/                    # Physics engine implementation (55k LOC)
│   ├── include/upe/         # 143+ public API headers
│   └── src/                 # 130+ implementation files
├── examples/                # 16+ working examples (vehicles, ragdolls, fluids, etc.)
├── tests/                   # 60+ unit tests
├── benchmarks/              # Performance validation suite
├── bindings/                # Language bindings (Python, C#, Rust, JavaScript, Lua)
├── docs/                    # 19 buyer-facing documentation files
├── cmake/                   # Build configuration
├── tools/                   # Validation & utility scripts
├── proof/                   # Determinism validation logs and benchmarks
├── CMakeLists.txt           # Top-level build configuration
├── README.md                # Project overview
├── LICENSE                  # IP and licensing terms
└── [other supporting docs]
```

### Core Implementation Files

**130+ C/C++ implementation files** (~55,000 lines of production-quality code):

| Directory | Purpose |
|-----------|---------|
| `core/src/base/` | Memory allocators, containers, math primitives |
| `core/src/math/` | Vec2/3/4, Mat2/3/4, Quaternions, Transforms |
| `core/src/collision/` | Broadphase (SAP), Narrowphase (GJK/EPA), Raycasts, Overlap queries |
| `core/src/constraints/` | PGS solver, Friction, Joints, CCD, Warm-starting |
| `core/src/world/` | World API, Body/Shape management, Integration, Vehicle, Articulated bodies |
| `core/src/tools/` | Character controller, Animation/IK, Snapshot/Replay, Profiler |
| `core/src/domains/` | Soft bodies (cloth), Fluids (SPH), Particles, Thermal, Waves, Breakables |

### 16+ Examples

Each example demonstrates a key capability:

| Example | Purpose | File |
|---------|---------|------|
| minimal_3d | Basic sphere collision | `examples/minimal_3d/` |
| collision_demo | PGS solver tuning | `examples/collision_demo/` |
| vehicle_demo | Vehicle suspension & tire forces | `examples/advanced_vehicle_demo/` |
| character_controller | Capsule character with ground detection | `examples/character_controller_demo/` |
| ragdoll | Articulated bodies with IK | `examples/ragdoll_character_demo/` |
| cloth | Position-based dynamics | `examples/pbd_cloth_demo/` |
| fluid | SPH with buoyancy | `examples/fluid_demo/` |
| [+9 more] | Various specialized domains | See `examples/` |

Build and run any example:
```bash
build/bin/examples/minimal_3d.exe
build/bin/examples/advanced_vehicle_demo/main.exe
# etc.
```

### 60+ Unit Tests

```bash
# Run all tests
ctest --test-dir build --output-on-failure -C Release

# Run specific test category
ctest --test-dir build -R "Collision" --output-on-failure
ctest --test-dir build -R "Joint" --output-on-failure
ctest --test-dir build -R "Determinism" --output-on-failure
```

**Test Categories**:
- Collision detection (broadphase, narrowphase, EPA)
- Joint constraint solving
- Vehicles (suspension, tires, forces)
- Articulated bodies (ragdolls, hierarchies)
- Character controller (walking, stepping, platforms)
- Animation IK
- Soft bodies (cloth)
- Fluids (SPH, buoyancy)
- Memory management
- Snapshot/Replay/Hashing
- Determinism (record/playback validation)

---

## 📖 DOCUMENTATION ROADMAP

Read in this order:

### Phase 1: Quick Understanding (30 min)

1. **[docs/BUYER_START_HERE.md](docs/BUYER_START_HERE.md)** ← **START HERE** (15 min)
   - What's in the engine
   - Quick build and run
   - Seven core examples
   - What to conclude

2. **[README.md](README.md)** (5 min)
   - Project overview and status

3. **[QUICK_START.md](QUICK_START.md)** (10 min)
   - Fast entry point, common use cases

### Phase 2: Technical Evaluation (2-3 hours)

4. **[docs/INTEGRATION_GUIDE.md](docs/INTEGRATION_GUIDE.md)** (1 hour)
   - How to embed engine in your codebase
   - Code examples: rigid bodies, vehicles, articulated bodies, joints, queries
   - Memory & threading model
   - Serialization/snapshots

5. **[docs/API_REFERENCE.md](docs/API_REFERENCE.md)** (ongoing reference)
   - Complete function signatures and parameter descriptions
   - All major systems documented with examples

6. **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)** (30 min)
   - Internal design: broadphase, narrowphase, solver, memory layout
   - Data flow per frame
   - Extension points for custom forces/constraints

7. **[docs/IMPLEMENTATION_STATUS.md](docs/IMPLEMENTATION_STATUS.md)** (15 min)
   - Feature matrix: what's complete, what's phase-scoped
   - Detailed subsystem status (vehicles, articulated, cloth, fluids, etc.)

### Phase 3: Risk & Limitations (1 hour)

8. **[docs/KNOWN_LIMITATIONS.md](docs/KNOWN_LIMITATIONS.md)** (20 min)
   - Hard boundaries: max objects, solver behavior, joint types
   - Floating-point precision constraints
   - Platform assumptions

9. **[docs/DETERMINISM.md](docs/DETERMINISM.md) + [docs/DETERMINISM_GUIDE.md](docs/DETERMINISM_GUIDE.md)** (20 min)
   - How determinism is evidenced and reproduced
   - Replay/snapshot infrastructure
   - Cross-platform drift visibility and platform-specific validation lanes
   - Performance vs. determinism trade-offs

10. **[docs/SECURITY_AND_SAFETY.md](docs/SECURITY_AND_SAFETY.md)** (10 min)
    - Input validation, bounds checking
    - Protection against NaN/overflow
    - Robustness for untrusted data

11. **[docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md)** (reference as needed)
    - Common build/runtime issues and solutions

### Phase 4: Business & IP (1 hour)

12. **[COMMERCIAL_LICENSE_AGREEMENT.md](COMMERCIAL_LICENSE_AGREEMENT.md)** (20 min)
    - IP ownership and distribution terms
    - Exclusive buyout path
    - Warranty disclaimer

13. **[TECHNICAL_ASSESSMENT.md](TECHNICAL_ASSESSMENT.md)** (20 min)
    - Independent risk analysis
    - Subsystem quality scoring
    - Known issues and trading cards

14. **[RELEASE_NOTES_v1.0.0.md](docs/RELEASE_NOTES_v1.0.0.md)** (10 min)
    - Feature set summary
    - Everything in v1.0.0 core
    - Optional/experimental modules

### Optional Deep Dives

- **[docs/GETTING_STARTED.md](docs/GETTING_STARTED.md)** — 10-line code example
- **[docs/BINDINGS_STATUS.md](docs/BINDINGS_STATUS.md)** — Language binding status (Python, C#, Rust, etc.)
- **[docs/FLUID_SIMULATION_GUIDE.md](docs/FLUID_SIMULATION_GUIDE.md)** — SPH fluid mechanics
- **[docs/ACCEPTANCE_CHECKLIST.md](docs/ACCEPTANCE_CHECKLIST.md)** — Structured evaluation framework
- **[docs/CHANGELOG.md](docs/CHANGELOG.md)** — Version history and changes
- **[docs/units_and_frames.md](docs/units_and_frames.md)** — Unit conventions and coordinate systems

---

## ✅ EVALUATION MILESTONES

Use this timeline to structure your evaluation:

### Week 1: Build & Initial Validation
- [ ] Extract package and verify SHA256
- [ ] Read BUYER_START_HERE.md (15 min)
- [ ] Build on Windows/Linux/macOS (20 min)
- [ ] Run tests and determinism proof (20 min)
- [ ] Run 3-4 examples, observe behavior (15 min)
- **Deliverable**: "Build works, tests pass, no surprises"

### Week 2: Technical Deep Dive
- [ ] Read INTEGRATION_GUIDE.md and study code examples
- [ ] Read API_REFERENCE.md for systems relevant to your use case
- [ ] Walk through ARCHITECTURE.md, understand data flow
- [ ] Review IMPLEMENTATION_STATUS.md for feature matrix
- [ ] Check KNOWN_LIMITATIONS.md for any blockers
- [ ] Read TECHNICAL_ASSESSMENT.md for risk analysis
- **Deliverable**: "Architecture understood, no show-stopping limitations"

### Week 3: Integration Feasibility
- [ ] Create a minimal integration mockup (1-2 days of work)
  - Load engine into your build system
  - Create a simple scene (2-3 bodies, basic collision)
  - Verify API surface meets your needs
- [ ] Benchmark performance against your baseline (if applicable)
- [ ] Evaluate determinism (run same scene 10 times, verify hash matches)
- [ ] Document any needed adaptations or custom integrations
- **Deliverable**: "Integration plan draft, estimated effort for full integration"

### Week 4: Executive Summary & Decision
- [ ] Compile findings into internal technical recommendation
- [ ] Discuss risk/benefit analysis with stakeholders
- [ ] Determine go/no-go for acquisition
- [ ] If GO: Schedule acquisition discussion call
- **Deliverable**: "Board-ready recommendation and next steps"

---

## 🤝 SUPPORT & COMMUNICATION

### Your Technical Contact

**Name**: [YOUR NAME]  
**Title**: [YOUR TITLE]  
**Email**: [YOUR EMAIL]  
**Phone**: [YOUR PHONE]  
**Availability**: [e.g., "Mon-Fri 9AM-5PM EST, responding within 24 hours"]

### How to Ask Questions

**Technical questions** (API, performance, architecture):
- Email with subject: `[UPE] <question>`
- Include: error message, code snippet, expected vs. actual behavior
- Response time: Within 24 hours

**Blockers or showstoppers**:
- Call/Zoom immediately
- Email: `[UPE URGENT] <issue>`
- Response time: Within 4 hours

**Non-technical questions** (licensing, commercialization, support):
- Email subject: `[UPE] Licensing / Business Question`
- Will loop in appropriate stakeholders

### Common Q&A Responses

See ["Sample Q&A responses" section in SALES_PROCESS_GUIDE.md](SALES_PROCESS_GUIDE.md#sample-qa-responses).

---

## 📊 PROOF ARTIFACTS

All evaluation results and validation logs are included:

### Determinism Validation

```
proof/RUNS/determinism_*.log
```

Shows:
- Frame-by-frame state hashing (CRC64)
- Multi-run reproducibility verification
- Cross-platform proof lanes and drift visibility artifacts

### Test Results

```
proof/RUNS/tests_*.log
```

Shows:
- All 60+ unit tests passing
- Execution times per test
- Coverage across subsystems

### Example Execution

```
proof/RUNS/examples_smoketest_*.log
```

Shows:
- All 16 examples build and run without crash
- No assertions, no memory errors

### Performance Benchmarks

```
proof/RUNS/
  └─ science_export_perf_*.json
```

Contains:
- Per-subsystem timing (broadphase, narrowphase, solve, integrate)
- Scaling with body count
- Comparison to baseline

### Reference Runs

```
proof/REFERENCE_RUNS/
  ├─ windows_reference_run_*.log
  ├─ linux_reference_run_*.log
  └─ hashes_reference_run_*.json
```

Shows:
- Original validation on development platform
- Expected output signatures
- Bit-exact hashes for comparison

---

## 🔍 WHAT TO LOOK FOR

### Signs of Quality

✅ **Positive indicators**:
- Build succeeds with zero compiler warnings
- All tests pass on first run
- Determinism validation consistent across runs
- Code is readable, well-organized, proper separation of concerns
- Documentation covers all major APIs
- Examples run without crashing
- Performance is reasonable (broadphase < 0.5ms, narrowphase < 1ms for typical scenes)

### Red Flags

⚠️ **Concerns to investigate**:
- Build warnings (indicates incomplete error handling or platform assumptions)
- Test failures or timeouts (indicates algorithm bugs or numerical instability)
- Determinism mismatch across runs (indicates floating-point ordering issues)
- Undocumented APIs or missing function signatures
- Performance degradation at scale (indicates broadphase/solver inefficiency)
- Unexplained crashes or segfaults (indicates memory safety issues)

### If You Find Issues

1. **Reproduce** the issue with minimal test case
2. **Document** exact steps and error output
3. **Email** technical contact with:
   - Issue description
   - Reproduction steps
   - Full error log/trace
   - OS, compiler version, configuration used
4. **Expect response** within 24 hours

---

## 💾 FILE HANDLING & SECURITY

### Security Best Practices

✅ **DO**:
- Store package on encrypted, access-controlled workstation
- Extract only to local disk (not network share)
- Limit access to core evaluation team
- Delete after evaluation if passing ("no-go")
- Securely delete if no acquisition (see NIST guidelines)
- Keep signed NDA for legal records

❌ **DO NOT**:
- Upload to GitHub, GitLab, or public repositories
- Share with teammates outside core evaluation group  
- Commit to your company's version control system
- Run on cloud platforms (AWS, Azure, GCP) without written approval
- Email source code to external consultants
- Discuss performance results or architecture publicly

### Return/Destruction

Upon termination of this evaluation:
1. Within 10 business days, destroy or return all Confidential Information
2. Provide written attestation: "All files have been [destroyed/returned]"
3. Keep only one archival copy for legal compliance (stay confidential)

(See NDA Section 6 for formal requirements)

---

## 🎯 EVALUATION SUCCESS CRITERIA

**Your evaluation is complete** when you can answer:

1. **Can we build it?** ✅ Yes, clean build with no issues
2. **Does it work?** ✅ Yes, tests pass, examples run, determinism validates
3. **Can we integrate it?** ⚠️ Estimate 2-6 weeks depending on your engine architecture
4. **Does it meet our requirements?** (Score: A, B, C, or D)
   - A = Exceeds expectations
   - B = Meets requirements
   - C = Has limitations but acceptable with workarounds
   - D = Significant gaps, likely not suitable
5. **What are the risks?** (Score: Low, Medium, High)
   - Low = Mature, well-documented, strong validation
   - Medium = Some concerns but mitigatable
   - High = Blockers or unknowns requiring resolution
6. **What's our recommendation?** (GO, CONDITIONAL, NO-GO)
   - GO: Proceed to acquisition discussion
   - CONDITIONAL: Interested but need X resolved first
   - NO-GO: Not a good fit for our roadmap

**Example Success Outcome**:
```
✅ Build: Clean, no warnings
✅ Tests: 60+ tests pass, determinism validates  
✅ Integration: Estimate 4 weeks for character controller + ragdoll
✅ Requirements: Meets 95% of needs, 5% workarounds acceptable
✅ Risk: Low—mature, well-tested, clear documentation
🎯 Recommendation: GO — Proceed to acquisition discussion
```

---

## 📞 WHAT HAPPENS NEXT

### At Day 30 of Evaluation

We'll reach out for a **30-minute status check call** to discuss:
- Any blockers or questions that arose
- Your initial recommendation (GO / CONDITIONAL / NO-GO)
- Timeline for final decision

### At Day 45 (End of Evaluation Period)

1. **You submit final recommendation** (email or call)
2. **If GO**: Schedule acquisition discussion with executive team
3. **If NO-GO**: End evaluation, both parties move on
4. **If CONDITIONAL**: Discuss specific blockers and next steps

### If GO → Acquisition Path

1. Seller prepares executive summary and term options
2. Discussion of deal structure (acquisition, licensing, support)
3. Buyer's legal team reviews acquisition terms
4. Negotiation phase (typically 2-4 weeks)
5. Closing and IP transfer (typically 1-2 weeks)

**Total time from NDA to closing**: ~2-3 months typical

---

## 📋 CHECKLIST: YOUR FIRST DAY

- [ ] Extract ZIP to secure local workstation
- [ ] Verify SHA256 hash matches
- [ ] Read BUYER_START_HERE.md (15 min)
- [ ] Build on your primary development platform (20 min)
- [ ] Run tests and determinism proof (10 min)
- [ ] Run one example and observe output (5 min)
- [ ] Bookmark docs and create evaluation timeline (5 min)
- [ ] Email technical contact: "Package received, build successful"

**Time estimate for this checklist**: 60 minutes

---

## ❓ FREQUENTLY ASKED QUESTIONS

### Q: Can we modify the source code?
**A**: During evaluation, no. Evaluation is for assessment only. Any modifications after acquisition/licensing are negotiated separately.

### Q: Can we share this with our game engine team?
**A**: Only with people on the core evaluation team (specified in NDA). Broader sharing requires NDA amendment.

### Q: How long does integration typically take?
**A**: 2-6 weeks depending on your engine architecture and target systems. See INTEGRATION_GUIDE.md for time estimates per use case.

### Q: What if we find a bug?
**A**: Report it to your technical contact with reproduction steps. We'll investigate and discuss fixes as part of acquisition terms.

### Q: Can we benchmark performance against our current engine?
**A**: Yes, that's encouraged. Benchmarks are internal use only (confidential). See docs/perf_gates.py for methodology.

### Q: What about licensing models?
**A**: Options include outright acquisition, exclusive license per industry, or support contract. Discussed after GO decision.

### Q: Do you offer multi-year support?
**A**: Support terms are negotiable in acquisition/license agreement. Typical: 2 years included, extensions available.

### Q: Is Linux/macOS/ARM64 supported?
**A**: Yes, CMake cross-compiles to Windows (MSVC), Linux (GCC/Clang), macOS (AppleClang). Full platform support in v1.0.0.

### Q: Can we use this in a shipped game?
**A**: Not during evaluation (NDA restricts to internal assessment only). After acquisition/licensing, yes.

### Q: What if we can't decide in 90 days?
**A**: Extension timelines are negotiable. Contact [YOUR NAME] to discuss.

---

## 🚀 READY TO START?

1. **Open**: `docs/BUYER_START_HERE.md`
2. **Build**: Follow the steps in Step 3 above
3. **Run**: Execute tests and an example
4. **Read**: Follow the Documentation Roadmap
5. **Ask**: Email [YOUR EMAIL] with questions
6. **Schedule**: Calendar your Week 1-4 milestones

**You've got this!**

---

## 🔒 COMPLIANCE REMINDER

This package and all its contents are **CONFIDENTIAL** per signed NDA.

- Do not republish, screenshot, or record demonstrations
- Do not discuss with unauthorized personnel
- Do not commit source to public repositories
- Do not present or publish findings without written approval
- Destruction required within 10 days of evaluation end

---

## 📮 CONTACT & NEXT STEPS

**Technical Lead**: [YOUR NAME]  
**Email**: [YOUR EMAIL]  
**Phone**: [YOUR PHONE]  
**Timezone**: [YOUR TIMEZONE]  
**Office Hours**: [e.g., Mon-Fri 9AM-5PM EST]

**Quick Start Email Template**:
```
Subject: [UPE] Package received, evaluation beginning

Hi [YOUR NAME],

Thank you for the Unified Physics Engine v1.0.0 evaluation package. 
My team and I have received it and are beginning the technical assessment today.

Initial plan:
- Build: [OS/compiler] by [DATE]
- Testing: [Benchmark plans] by [DATE]
- Executive summary: [DATE]

Will reach out if we hit any blockers. Thanks for being available!

Best,
[BUYER TECHNICAL LEAD]
```

---

**Evaluation Package Contents Last Updated**: 2026-02-26  
**UPE Version**: 1.0.0  
**Status**: Production Ready

---

**END OF DILIGENCE PACKAGE GUIDE**
