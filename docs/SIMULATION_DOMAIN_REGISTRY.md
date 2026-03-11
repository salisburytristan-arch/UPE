# Simulation Domain Registry

This file is the canonical registry of all simulation domains in UPE, what source module covers each domain, and where to find the authoritative API.

> **Authority note:** Implementation status details live in `FEATURE_STATUS_MATRIX.md`. Platform and determinism scope lives in `TRUTH_HIERARCHY.md`. This file covers domain-to-module mapping only.

---

## Domain Map

| Domain | Module Header(s) | Source File(s) | Maturity |
|--------|-----------------|----------------|----------|
| **Rigid Body Dynamics** | `upe_body.h`, `upe_world.h` | `world/upe_body.cpp`, `world/upe_world.cpp` | production-ready |
| **Collision Broadphase** | `upe_broadphase_scalar.h` | `collision/upe_broadphase_sap.cpp` | production-ready |
| **Collision Narrowphase** | `upe_narrowphase_scalar.h`, `upe_gjk.h`, `upe_epa.h` | `collision/upe_gjk.cpp`, `upe_epa.cpp` | production-ready |
| **Contact Manifolds** | `upe_manifold_cache.h` | `collision/upe_manifold_cache.cpp` | production-ready |
| **Continuous Collision Detection** | `upe_ccd.h`, `upe_ccd_advanced.h` | `collision/upe_ccd.cpp` | production-ready |
| **PGS Constraint Solver** | `upe_solver.h` | `constraints/upe_solver_pgs.cpp` | production-ready |
| **XPBD Solver** | `upe_xpbd_solver.h` | `constraints/upe_xpbd_solver.cpp` | validated |
| **Joint System** | `upe_joint.h` | `constraints/upe_joint.cpp` | production-ready |
| **Articulated Bodies / Ragdoll** | `upe_articulated.h`, `upe_articulation.h` | `world/upe_articulated.cpp` | validated |
| **IK / FK** | `upe_ik_solver.h` | `world/upe_ik_solver.cpp` | validated |
| **Character Controller** | `upe_character_controller.h` | `world/upe_character_controller.cpp` | validated |
| **Vehicle Physics (Core)** | `upe_vehicle.h` | `world/upe_vehicle.cpp` | validated |
| **Vehicle Physics (Advanced)** | `upe_advanced_vehicle.h` | `world/upe_advanced_vehicle.cpp` | validated |
| **Cloth / PBD** | `upe_cloth.h` | `world/upe_cloth.cpp` | validated |
| **Soft Body (Mass-Spring)** | `upe_softbody.h` | `softbody/upe_softbody.cpp` | validated |
| **FEM Deformables** | `upe_fem.h` | `world/upe_fem.cpp` | validated |
| **Rope / Cable** | `upe_rope.h` | `world/upe_rope.cpp` | validated |
| **Hair / Strands** | `upe_hair.h` | `world/upe_hair.cpp` | validated |
| **Fracture / Destruction** | `upe_fracture.h`, `upe_destruction.h`, `upe_breakable.h` | `world/upe_fracture.cpp`, `upe_destruction.cpp` | validated |
| **Granular / DEM** | `upe_dem.h` | `world/upe_dem.cpp` | validated |
| **SPH Fluids (Lagrangian)** | `upe_fluid_sph.h` | `fluids/upe_sph.cpp` | validated |
| **FLIP Fluids (Eulerian)** | `upe_flip.h` | `world/upe_flip.cpp` | validated |
| **Fluid Structure Interaction** | `upe_fluid_coupling.h` | `fluids/upe_fluid_coupling.cpp` | validated |
| **Buoyancy** | `upe_buoyancy.h` | `world/upe_buoyancy.cpp` | validated |
| **Aerodynamics** | `upe_aerodynamics.h`, `upe_aerodynamics_advanced.h` | `world/upe_aerodynamics.cpp` | validated |
| **Thermodynamics** | `upe_thermal.h` | `world/upe_thermal.cpp` | present-not-buyer-critical |
| **Acoustics** | `upe_acoustics.h` | `world/upe_acoustics.cpp` | present-not-buyer-critical |
| **Electromagnetism / Plasma** | `upe_electromagnetism.h`, `upe_plasma.h` | `world/upe_electromagnetism.cpp`, `upe_plasma.cpp` | present-not-buyer-critical |
| **Orbital Mechanics** | `upe_orbital_mechanics.h` | `world/upe_orbital_mechanics.cpp` | present-not-buyer-critical |
| **Relativistic Physics** | `upe_relativistic_physics.h` | `world/upe_relativistic_physics.cpp` | present-not-buyer-critical |
| **Gravity Fields** | `upe_gravity_fields.h` | `world/upe_gravity_fields.cpp` | present-not-buyer-critical |
| **Combustion / Chemistry** | `upe_combustion.h`, `upe_chemistry.h` | `world/upe_combustion.cpp` | present-not-buyer-critical |
| **Microphysics / Molecular** | `upe_molecular_physics.h` | `world/upe_molecular_physics.cpp` | scientific-approximation |
| **Phase Change** | `upe_phase_change.h` | `world/upe_phase_change.cpp` | scientific-approximation |
| **Erosion / Sedimentation** | `upe_erosion.h` | `world/upe_erosion.cpp` | scientific-approximation |
| **Shape Matching** | `upe_shape_matching.h` | `world/upe_shape_matching.cpp` | validated |
| **Ballistics** | `upe_ballistics.h` | `world/upe_ballistics.cpp` | validated |
| **Biomechanics** | `upe_biomechanics.h` | `world/upe_biomechanics.cpp` | scientific-approximation |
| **Electronics / Circuits** | `upe_electronics.h` | `world/upe_electronics.cpp` | scientific-approximation |
| **Non-Euclidean Space** | `upe_non_euclidean.h` | `world/upe_non_euclidean.cpp` | experimental |
| **Portal Mechanics** | `upe_portal.h` | `world/upe_portal.cpp` | experimental |
| **Dynamic Weathering** | `upe_weathering.h` | `world/upe_weathering.cpp` | experimental |
| **Space Physics (Stellar/Cosmological)** | `upe_space_physics.h` | *(extended in orbital/relativistic)* | scientific-approximation |
| **SDF Volumes** | `upe_sdf.h` | `collision/upe_sdf.cpp` | validated |
| **Octree Spatial Partition** | `upe_octree.h` | `collision/upe_octree.cpp` | validated |
| **Pathfinding (Physics-Aware)** | `upe_pathfinding.h` | `world/upe_pathfinding.cpp` | validated |
| **Procedural Noise** | `upe_noise.h` | `math/upe_noise.cpp` | utility |
| **Rendering Math Utilities** | `upe_rendering_math.h` | *(header-only)* | utility |
| **Math Toolbox** | `upe_math_toolbox.h` | *(header / sparse impls)* | production-ready |
| **Determinism / Replay** | `upe_determinism.h`, `upe_replay.h`, `upe_snapshot.h` | `world/upe_determinism.cpp`, `base/upe_replay.cpp` | production-ready |
| **Networking Delta** | `upe_net_delta.h` | `world/upe_net_delta.cpp` | validated |

---

## Maturity Label Definitions

| Label | Meaning |
|-------|---------|
| `production-ready` | CI-tested, determinism-safe, buyer critical path |
| `validated` | Tests and examples exist; not on primary buyer proof path |
| `present-not-buyer-critical` | In repo, functional, but not in diligence baseline |
| `scientific-approximation` | Mathematically grounded model; not full first-principles simulation |
| `experimental` | Scaffolded, usable, subject to interface change |
| `utility` | Math/rendering helper with no simulation state |

---

## Conflict Resolution

If two documents disagree about domain scope or status, the authority order is:

1. `FEATURE_STATUS_MATRIX.md` (item-level status)
2. `MATURITY_MATRIX.md` (subsystem maturity)
3. This file (domain-to-module mapping)
4. Individual header file comments
