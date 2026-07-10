# Unified Physics Engine (UPE) — Buyer License Package

**Prepared:** April 6, 2026  
**Package Version:** 2026.04  
**Status:** Private Buyer Evaluation Ready  
**Confidentiality:** NDA / authorized commercial review only  
**Audience:** Buyer technical leads, product leadership, procurement, and legal counsel

Validation authority: if this package conflicts with current proof, maturity, or platform docs, defer to:
- `docs/PROOF_INDEX.md`
- `docs/TRUTH_HIERARCHY.md`
- `docs/MATURITY_MATRIX.md`
- `docs/PLATFORM_SUPPORT_MATRIX.md`
- `docs/FEATURE_STATUS_MATRIX.md`

---

## 1) Executive Summary

This document is the **top-level buyer index** for UPE licensing, technical evaluation, diligence, and package delivery.
It is meant to give a serious buyer a single place to understand:

- **what UPE is**,
- **what has actually been verified**,
- **what rights are and are not being offered at this stage**,
- **which transaction structures are available**,
- **which repository artifacts matter most for technical and legal review**.

### Short form positioning

UPE is best evaluated as a **deterministic, dependency-light native simulation runtime** with a **stable C API**, **replay/snapshot/state-hash workflows**, and a **proof-backed diligence surface**.

### What makes the package commercially credible

- a verified build and test path
- determinism proof tooling with repeat-run hash output
- a clean documentation trail for buyers
- explicit maturity boundaries rather than overclaiming
- clear ownership / provenance / licensing posture

> **Core-first posture:** the most defensible buyer story is the deterministic core path and validated integration surface. Broader scientific or exotic modules are real in-repo assets, but should be evaluated by fit and documented maturity rather than assumed equal to the core production path.

---

## 2) Fresh Verification Snapshot

This package refresh is grounded in a fresh local verification run on **2026-04-06**.

### Verification commands used

```bat
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUPE_BUILD_TESTS=ON -DUPE_BUILD_EXAMPLES=ON -DUPE_DETERMINISTIC=ON
cmake --build build --config Release --parallel
ctest --test-dir build --output-on-failure -C Release
tools\run_determinism_proof.bat build
```

### Verified results

- CMake configure completed successfully
- Release build completed successfully
- `100% tests passed, 0 tests failed out of 1`
- Determinism proof log: `proof/RUNS/determinism_20260406_064627.log`
- Determinism result: `hash_run_1=0x64D24F0A3FB9921E`, `hash_run_2=0x64D24F0A3FB9921E`, `MATCH=true`, `[UPE] SUCCESS`

### Current inventory snapshot

| Item | Current count |
|---|---:|
| Public headers | 183 |
| Implementation files | 173 |
| Test source files | 80 |
| Example directories | 33 |

### Why this matters to a buyer

This is not a packaging-only story. The package is backed by:
- a successful current build,
- a passing test run,
- a current determinism proof artifact,
- and a repository structure large enough to support real engineering diligence.

---

## 3) What the Buyer Is Evaluating

UPE should be evaluated as a **source-available commercial technology package** centered on the following core value:

### A. Deterministic core runtime
- rigid-body world stepping
- collision broadphase / narrowphase
- constraint solving
- snapshot / restore
- replay workflows
- state hashing for repeatability checks

### B. Clean integration posture
- stable C API boundary
- no mandatory third-party runtime physics engine dependency for standard C API use
- source-level reviewability and modification potential under commercial terms

### C. Supporting proof and diligence surface
- examples and tests
- proof-oriented documentation
- benchmark reference material
- platform and maturity classification docs
- provenance and ownership references

### D. Broader technical upside
The repository also contains broader capability areas such as:
- vehicles and articulated systems
- ragdolls / character systems
- cloth / soft body / fluid systems
- spatial and math utility modules
- additional scientific or approximation-heavy domains

These broaden the asset’s upside, but they are **not all represented as equal buyer-critical production commitments**.

---

## 4) Maturity and Scope Classification

The current maturity framing from `docs/MATURITY_MATRIX.md` should guide commercial interpretation.

| Capability Area | Current posture | Buyer interpretation |
|---|---|---|
| Core rigid-body world step pipeline | `production-ready` | Primary commercial and diligence baseline |
| Collision + solver pipeline | `production-ready` | Central technical trust surface |
| Replay / snapshot / state hashing | `production-ready` | Major differentiator for rollback and auditable workflows |
| Build / test / release gating | `production-ready` | Strong process and packaging signal |
| Cross-platform build matrix | `validated` | Valuable and real, but still interpreted through CI and platform docs |
| Vehicle and articulated systems | `validated` | Buyer-relevant depending on use case |
| Soft body / cloth / fluids | `validated` | Included and evaluable; not necessarily every buyer’s critical path |
| Advanced science domains | `present-not-buyer-critical` or `experimental` | Upside / R&D breadth, not default acceptance baseline |

### Practical reading rule

A buyer should prioritize:
1. `production-ready` capabilities first,
2. then `validated` systems,
3. and only then treat `experimental` or `present-not-buyer-critical` areas as roadmap upside.

---

## 5) Platform and Dependency Posture

### Platform support summary

Per `docs/PLATFORM_SUPPORT_MATRIX.md`:

| Platform | Toolchain | Status |
|---|---|---|
| Windows x64 | MSVC | `verified-in-ci` |
| Linux x64 | GCC | `verified-in-ci` |
| Linux x64 | Clang | `verified-in-ci` |
| macOS | Apple Clang | `verified-in-ci` |
| Windows x64 | MinGW/GCC | `experimental` |
| Linux ARM64 | GCC/Clang | `not-yet-validated` |

### Dependency posture

Per `docs/DEPENDENCY_INVENTORY.md`:

- core runtime target: **no mandatory third-party runtime dependency** for standard C API integration
- typical build/tooling stack: **CMake + native compiler toolchain + Python for scripts**
- bindings for Python, C#, Rust, JavaScript/WASM, and Lua are present as **integration convenience layers**, not core runtime requirements

### Why this matters commercially

This posture reduces dependency risk during diligence and makes integration review more straightforward for buyer engineering teams.

---

## 6) Commercial Paths

UPE can support more than one commercial structure depending on buyer intent and transaction size.

| Path | Best fit | Typical purpose | Primary references |
|---|---|---|---|
| **Pilot / Integration License** | Teams validating UPE inside a real product branch or prototype | Short evaluation-to-integration path | `docs/PURCHASE_OPTIONS.md`, `COMMERCIAL_LICENSE_AGREEMENT.md` |
| **Internal-Use Source License** | Internal simulators, tools, digital twins, or non-redistributed proprietary use | Controlled source access without full transfer | `docs/PURCHASE_OPTIONS.md` |
| **Enterprise Source License** | Studios or platform teams needing broader rights and negotiated scope | Long-term product integration | `COMMERCIAL_LICENSE_AGREEMENT.md` |
| **Field / Vertical-Exclusive License** | Buyers seeking exclusivity in a defined market | Strategic differentiation by domain | `COMMERCIAL_LICENSE_AGREEMENT.md` |
| **Full Acquisition / IP Transfer** | Buyers seeking outright ownership | Full strategic control under a separate signed agreement | `COMMERCIAL_LICENSE_AGREEMENT.md`, `IP_CHAIN_OF_TITLE_CHECKLIST.md` |

### Commercial framing rule

At evaluation stage, these are **available paths**, not automatically granted rights.
Final pricing, scope, exclusivity, indemnities, support, and ownership transfer are defined only in signed transaction documents.

---

## 7) Rights Boundary and Restrictions

Current package posture is intentionally strict and clear.

### What the buyer may do during evaluation
- review documentation
- inspect source materials provided for evaluation
- build and test the package internally
- assess technical fit, risk, and commercial viability

### What the buyer may not assume
- no commercialization rights are granted by access alone
- no ownership transfer is implied by file delivery, repository access, or package possession
- no sublicensing, resale, reposting, mirroring, or redistribution is authorized without signed written permission
- no support, warranty, indemnity, or exclusivity exists unless specifically stated in final signed agreements

### Governing references
- `LICENSE`
- `COMMERCIAL_LICENSE_AGREEMENT.md`
- `docs/PROVENANCE_AND_OWNERSHIP.md`
- `IP_CHAIN_OF_TITLE_CHECKLIST.md`

---

## 8) Buyer-Facing Technical and Legal Document Map

The following documents make up the primary diligence surface.

| Category | Key files | Why the buyer should read them |
|---|---|---|
| **Fast orientation** | `docs/BUYER_START_HERE.md`, `EXECUTIVE_SUMMARY.md`, `ONE_PAGE_OVERVIEW.md` | Fast path to understanding the asset and transaction posture |
| **Core technical review** | `API_REFERENCE.md`, `ARCHITECTURE.md`, `TECHNICAL_ASSESSMENT.md`, `QUICK_START.md` | Helps engineering leads assess build flow, API shape, and architecture |
| **Proof and verification** | `docs/PROOF_INDEX.md`, `docs/DETERMINISM_CERTIFICATION.md`, `docs/BENCHMARK_BASELINES.md`, `docs/CI_GATES.md`, `docs/VERIFICATION_STATUS_DASHBOARD.md` | Grounds claims in artifacts, logs, and acceptance surfaces |
| **Scope and maturity** | `docs/MATURITY_MATRIX.md`, `docs/FEATURE_STATUS_MATRIX.md`, `docs/KNOWN_LIMITATIONS.md`, `docs/PLATFORM_SUPPORT_MATRIX.md`, `docs/UPE_CORE_SCOPE.md` | Prevents over-interpretation and keeps review honest |
| **Ownership and provenance** | `PROVENANCE_AUDIT.md`, `docs/PROVENANCE_AND_OWNERSHIP.md`, `IP_CHAIN_OF_TITLE_CHECKLIST.md`, `docs/DEPENDENCY_INVENTORY.md` | Supports legal and IP diligence |
| **Commercial process support** | `BUYER_EVALUATION_CHECKLIST.md`, `DILIGENCE_PACKAGE_GUIDE.md`, `DILIGENCE_PACKAGE_EMAIL_TEMPLATE.md`, `SALES_PROCESS_GUIDE.md` | Helps structure internal buyer review and communication |

---

## 9) Recommended Buyer Evaluation Sequence

### Phase 1 — 15-minute orientation
1. Open `docs/BUYER_START_HERE.md`
2. Read `EXECUTIVE_SUMMARY.md`
3. Review this `BUYER_LICENSE_PACKAGE.md`

### Phase 2 — 30 to 60 minute technical confidence check
1. Configure and build the repository on the target platform
2. Run `ctest`
3. Run `tools\run_determinism_proof.bat build` or the shell equivalent
4. Review `docs/PROOF_INDEX.md`

### Phase 3 — engineering diligence
1. Review `API_REFERENCE.md` and `ARCHITECTURE.md`
2. Inspect `docs/MATURITY_MATRIX.md` and `docs/KNOWN_LIMITATIONS.md`
3. Map required buyer use cases to the core and validated subsystems

### Phase 4 — legal / procurement diligence
1. Review `COMMERCIAL_LICENSE_AGREEMENT.md`
2. Review `LICENSE`
3. Review `PROVENANCE_AUDIT.md` and `docs/PROVENANCE_AND_OWNERSHIP.md`
4. Select the intended commercial path for negotiation

---

## 10) Buyer Acceptance Questions

A qualified buyer should be able to answer the following after reviewing this package.

### Technical
- Can your team build and run UPE in the intended target environment?
- Does the deterministic core workflow match your netcode, replay, QA, or simulation requirements?
- Is the C API integration boundary suitable for your stack and engineering process?
- Are the provided proof materials sufficient for internal technical sign-off?

### Product / Strategy
- Is the deterministic core itself valuable even before considering broader feature breadth?
- Which secondary systems matter to your roadmap, and which are simply upside?
- Does the current maturity profile match your delivery risk tolerance?

### Legal / Commercial
- Which transaction path best fits your intended rights: evaluation, source license, exclusivity, or acquisition?
- What additional diligence artifacts, if any, are required for internal approval?
- Are support, transition, or exclusivity terms needed for the next phase of negotiation?

---

## 11) Important Disclosure Notes

The package should be interpreted with the following disclosures kept explicit:

- maturity varies by subsystem
- the **core deterministic path** is the primary diligence baseline
- broader scientific or exotic modules should be interpreted through their documented status, not assumed equal to the core path
- Linux/macOS flows exist and CI evidence is present, but buyers should still validate on their own target environment and toolchain
- support, warranties, indemnities, exclusivity, and final transfer scope are defined only in signed agreements
- the buyer is expected to perform independent technical and legal diligence before closing

### What this package is not claiming

This package does **not** claim:
- that every repository subsystem is equally production-mature
- that all possible platforms are production-guaranteed
- that evaluation access alone grants shipping or transfer rights
- that roadmap breadth should be treated as immediate delivery scope without diligence

---

## 12) Commercial Source Delivery Artifact

Current delivery package name:

- `UnifiedPhysicsEngine-1.0.0-commercial-source.zip`

### Delivery manifest

| Path | Included | Purpose |
|---|---|---|
| `core/` | Yes | Source tree for direct engineering review |
| `docs/BUYER_START_HERE.pdf` | Yes | Executive technical orientation |
| `docs/COMMERCIAL_LICENSE.pdf` | Yes | Licensing and transaction posture |
| `docs/RELEASE_NOTES.pdf` | Yes | Release summary and buyer-facing change context |
| `docs/PROOF_INDEX.pdf` | Yes | Proof and evidence entry point |
| `checksums/sha256.txt` | Yes | Integrity verification for delivered files |

### Explicit exclusions

The delivery package intentionally excludes:
- `.git`
- `.venv`
- `build/`
- `build_verify/`
- `build_warn/`
- `build_windows/`
- local `proof/RUNS/` logs not meant for customer delivery
- nested or older release ZIP files inside the product ZIP

### Integrity verification

The commercial source ZIP currently has the following SHA256:

```text
40F7B0C8005B585CAD55369ABB60F0C8C34F986CDA1992E923ACFF651171DFFB
```

Buyers can independently verify integrity using standard tooling, for example:

```powershell
Get-FileHash .\UnifiedPhysicsEngine-1.0.0-commercial-source.zip -Algorithm SHA256
```

---

## 13) Seller Delivery Checklist

When sending this package to a qualified buyer, include:

- NDA confirmation or equivalent written confidentiality coverage
- private repo access or the approved packaged source archive
- package checksum and, where useful, commit hash reference
- `BUYER_LICENSE_PACKAGE.md` as the top-level package index
- the commercial-source ZIP and its checksum record
- proof logs or additional release artifacts only if specifically requested for diligence
- the correct commercial contact path for licensing or acquisition discussion

### Recommended internal seller check before sending
- verify the ZIP opens cleanly
- verify the included PDFs are readable
- verify `checksums/sha256.txt` is present
- confirm excluded directories and local-only artifacts are absent
- confirm buyer-facing docs are the current versions

---

## 14) Bottom Line

UPE is best presented to buyers as a **proof-backed, deterministic, source-available runtime** with a clear legal boundary and multiple commercial transaction paths.

The strongest near-term package is **not** an “everything engine” pitch. It is a **credible core technology package** with:
- current build/test evidence,
- reproducible determinism proof,
- clear maturity and platform framing,
- and an organized diligence trail for both engineering and legal review.

That framing improves buyer trust, shortens internal approval cycles, and keeps the commercial discussion anchored in what is real and verifiable today.
