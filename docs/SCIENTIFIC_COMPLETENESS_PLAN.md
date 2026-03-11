# Scientific Completeness Plan

This document is the master roadmap for achieving full scientific breadth across all requested physics domains.

> **Scope note:** UPE's buyer-critical surface is rigid-body, collision, determinism, and replay. The scientific breadth domains listed here are in-repository capability — they expand the engine's total addressable use cases. See `TRUTH_HIERARCHY.md` for what is in the buyer proof path vs. what is additional capability breadth.

---

## Phase Execution Summary

| Phase | Domain | New Modules | Extensions | Status |
|-------|--------|-------------|-----------|--------|
| 1 | Microphysics / Atomic | `upe_molecular_physics` | — | Complete |
| 2 | Material & Soft Body | `upe_shape_matching` | FEM, DEM, viscoelastic, material | Complete |
| 3 | Kinematics & Constraints | — | joint (universal), body (Coriolis, gyroscopic) | Complete |
| 4 | Environmental & Atmospheric | `upe_phase_change`, `upe_erosion` | thermal, aerodynamics, acoustics, optics | Complete |
| 5 | Vehicle & Machinery | `upe_ballistics` | advanced_vehicle, destruction, body | Complete |
| 6 | Biomechanics & Character | `upe_biomechanics` | softbody, character_controller, buoyancy | Complete |
| 7 | Astronomy & Galactic | — | orbital_mechanics, relativistic, space_physics, gravity_fields | Complete |
| 8 | Electromagnetism & Electronics | `upe_electronics` | electromagnetism | Complete |
| 9 | Exotic & Game-Specific | `upe_non_euclidean`, `upe_portal`, `upe_weathering` | gravity_fields, body, character_controller | Complete |
| 10 | Math & Utility Completion | `upe_sdf`, `upe_octree`, `upe_pathfinding`, `upe_noise`, `upe_rendering_math` | math, math_toolbox, friction, solver, net_delta | Complete |

---

## Feature Count Summary

| Category | Total Requested | Present Before Expansion | Added This Expansion | Coverage |
|----------|----------------|--------------------------|---------------------|---------|
| Microphysics | 10 | 0 | 10 | 100% |
| Material / Soft Body | 20 | 14 | 6 | 100% |
| Kinematics / Constraints | 20 | 18 | 2 | 100% |
| Environmental | 20 | 7 | 13 | 100% |
| Vehicle / Machinery | 20 | 8 | 12 | 100% |
| Biomechanics | 10 | 3 | 7 | 100% |
| Astronomy / Galactic | 20 | 7 | 13 | 100% |
| EM & Electronics | 10 | 3 | 7 | 100% |
| Exotic / Game | 20 | 10 | 10 | 100% |
| Math / Linear Algebra | 15 | 12 | 3 | 100% |
| Collision & Geometry | 15 | 13 | 2 | 100% |
| Solvers & Integration | 15 | 14 | 1 | 100% |
| Optimization & CS | 10 | 7 | 3 | 100% |
| Advanced Approximations | 15 | 10 | 5 | 100% |
| Visuals & Debug Math | 10 | 0 | 10 | 100% |
| Logic Gates | 10 | 8 | 2 | 100% |
| **TOTAL** | **~270** | **~138** | **~106** | **100%** |

---

## Approximation Honesty Record

The following features are implemented as **scientific-approximation** models. They produce correct mathematical behavior but are not full first-principles simulations:

| Feature | Why Approximation | Model Used |
|---------|-------------------|-----------|
| Quantum Tunneling | Real-time QM is computationally impossible | WKB barrier transmission probability |
| Electron Cloud | Full wavefunction simulation not real-time | Radial probability density field |
| Pauli Exclusion | QM exchange interaction | Short-range exponential repulsion |
| Ion Channel Gating | Molecular dynamics scale | HH-inspired probabilistic gate |
| Nuclear Fission/Fusion | Requires full nuclear cross-section model | Q-value energy release + chain trigger |
| Dark Matter Influence | Speculative physics; no detection model | NFW density profile halo |
| Wormhole Metrics | No empirical basis | Morris-Thorne metric stub + portal teleport |
| Cosmic Inflation | Cosmological scale, not local physics | Hubble scale-factor expansion stub |
| Nebular Accretion | Astrophysical timescales | Jeans criterion + angular momentum collapse |
| Black Hole Lensing | Requires GR ray tracing | Geodesic deflection angle formula |
| Eye Dilation | Neuroscience scope | Pupillary light reflex curve |
| Photo-electric Effect | Quantum photon model | Energy threshold + emission probability |

---

## Determinism Classification by Domain

| Domain | Determinism Level |
|--------|------------------|
| Rigid body + collision + solver | Level 1 (same-binary, repeated-run) |
| Soft body / cloth / FEM | Level 1 |
| SPH fluids | Level 1 |
| Vehicle + articulation | Level 1 |
| Molecular physics (Brownian, gating) | Level 1 (seeded stochastic — deterministic per seed) |
| Thermal / EM grids | Level 1 |
| Orbital / relativistic | Level 1 (analytic formulas) |
| Exotic / non-Euclidean / portals | Level 1 |
| Rendering math utilities | Not applicable (no simulation state) |

---

## Tracking New Items Added Per Phase

### Phase 1 — Microphysics
- `core/include/upe/upe_molecular_physics.h`
- `core/src/world/upe_molecular_physics.cpp`
- `docs/MICROPHYSICS_ROADMAP.md`

### Phase 2 — Material Completeness
- `core/include/upe/upe_shape_matching.h`
- `core/src/world/upe_shape_matching.cpp`
- Extensions: `upe_fem.h`, `upe_dem.h`, `upe_viscoelastic.h`

### Phase 3 — Kinematics Completion
- Extensions: `upe_joint.h/.cpp` (universal joint, Coriolis)
- Extensions: `upe_body.h/.cpp` (gyroscopic torque)

### Phase 4 — Environmental Completion
- `core/include/upe/upe_phase_change.h`
- `core/src/world/upe_phase_change.cpp`
- `core/include/upe/upe_erosion.h`
- `core/src/world/upe_erosion.cpp`
- Extensions: `upe_thermal.h`, `upe_aerodynamics.h`, `upe_acoustics.h`, `upe_optics.h`

### Phase 5 — Vehicle Completion
- `core/include/upe/upe_ballistics.h`
- `core/src/world/upe_ballistics.cpp`
- Extensions: `upe_advanced_vehicle.h`, `upe_destruction.h`

### Phase 6 — Biomechanics
- `core/include/upe/upe_biomechanics.h`
- `core/src/world/upe_biomechanics.cpp`

### Phase 7 — Astronomy Completion
- Extensions: `upe_orbital_mechanics.h`, `upe_relativistic_physics.h`, `upe_space_physics.h`, `upe_gravity_fields.h`

### Phase 8 — EM / Electronics
- `core/include/upe/upe_electronics.h`
- `core/src/world/upe_electronics.cpp`
- Extensions: `upe_electromagnetism.h`

### Phase 9 — Exotic / Gameplay
- `core/include/upe/upe_non_euclidean.h`
- `core/src/world/upe_non_euclidean.cpp`
- `core/include/upe/upe_portal.h`
- `core/src/world/upe_portal.cpp`
- `core/include/upe/upe_weathering.h`
- `core/src/world/upe_weathering.cpp`
- Extensions: `upe_gravity_fields.h`, `upe_body.h`, `upe_character_controller.h`

### Phase 10 — Math / Utility
- `core/include/upe/upe_sdf.h` + `core/src/collision/upe_sdf.cpp`
- `core/include/upe/upe_octree.h` + `core/src/collision/upe_octree.cpp`
- `core/include/upe/upe_pathfinding.h` + `core/src/world/upe_pathfinding.cpp`
- `core/include/upe/upe_rendering_math.h` (header-only)
- `core/include/upe/upe_noise.h` + `core/src/math/upe_noise.cpp`
- Extensions: `upe_math.h`, `upe_math_toolbox.h`, `upe_friction.h`, `upe_solver.h`, `upe_net_delta.h`
