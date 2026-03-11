# UnifiedPhysicsEngine Architecture

Trust note: this document describes architectural intent and subsystem design.
For current externally validated guarantees and confidence levels, defer to:
- `docs/PROOF_INDEX.md`
- `docs/DETERMINISM_CERTIFICATION.md`
- `docs/PLATFORM_SUPPORT_MATRIX.md`
- `docs/MATURITY_MATRIX.md`
- `docs/TRUTH_HIERARCHY.md`

## Overview

The UnifiedPhysicsEngine (UPE) is a broad physics simulation framework with deterministic tooling and cross-platform build paths.

## Core Design Philosophy

**Everything is Bodies, Constraints, Particles, or Fields**

- **Bodies**: Rigid bodies, soft bodies, articulated systems
- **Constraints**: Joints, contacts, user constraints  
- **Particles**: Fluids, granular materials, gases, hair, cloth particles
- **Fields**: Temperature, pressure, wind, density, EM fields, etc.

## Foundational Systems

### 1. Field System (`upe_field_system.h`)
**Purpose**: Unified infrastructure for all scalar/vector/tensor fields

**Discretizations**:
- Dense grids (fast, uniform)
- Sparse grids (memory efficient)
- Particles (Lagrangian, meshless)
- SDF volumes (signed distance fields)
- Mesh-based (FEM elements)

**Key Features**:
- Write permissions (coupling contracts)
- Multiple interpolation modes
- Advection-diffusion-reaction operators

### 2. Material System (`upe_material_system.h`)
**Purpose**: Comprehensive material property database

**100+ Material Presets**:
- 12 Woods (Oak, Pine, Bamboo, etc.)
- 25 Metals & Alloys (Steel, Aluminum, Titanium, etc.)
- 15 Polymers/Elastomers (ABS, PLA, Rubber, etc.)
- 10 Ceramics/Glass
- 8 Composites (Carbon fiber, Kevlar, etc.)
- 10 Soils/Granular
- 12 Liquids
- 8 Gases

**Property Categories**:
- **Bulk**: Density, elasticity, plasticity, fracture, anisotropy
- **Surface**: Friction (static/kinetic/rolling/torsional), restitution, adhesion, wear
- **Thermal**: Conductivity, capacity, expansion, phase transitions
- **Electrical/Magnetic**: Conductivity, permittivity, permeability
- **Chemical**: Corrosion, flammability, toxicity, reactivity

**Combine Rules**: Average, Min, Max, Multiply, Custom

### 3. Multiphysics Coupling Contract (`upe_coupling_contract.h`)
**Purpose**: Deterministic, conflict-free multi-domain interaction

**Contract Declaration**:
- Each domain declares: inputs, outputs, execution stage, priority
- Write permissions validated at runtime
- Force aggregation with deterministic reduction

**Coupling Stages**:
1. **Pre-Step**: Initialization, field updates
2. **Forces**: Force accumulation across domains
3. **Constraints**: Solver execution
4. **Post-Step**: State synchronization
5. **Query**: Read-only access

**Common Patterns**:
- Fluid → Rigid: Buoyancy, drag
- Rigid → Fluid: Displacement, splashing
- Thermal → Rigid: Expansion forces
- EM → Rigid: Lorentz forces
- Fluid → Thermal: Convection

### 4. Scene Query System (`upe_scene_query.h`)
**Purpose**: Spatial queries across all geometry types

**Query Types**:
- **Raycast**: Ray-geometry intersection
- **Sweep**: Convex shape cast (continuous collision)
- **Overlap**: Static overlap test
- **Closest Point**: Nearest surface point

**Features**:
- Layer filtering (bitfield masks)
- Custom filter callbacks
- Backface culling
- Trigger-only vs solid-only
- Deterministic sorting

**Geometry Support**:
- Primitives: Box, sphere, capsule, cylinder
- Convex hulls
- Triangle meshes
- Heightfields
- SDF volumes

### 5. Trigger/Sensor System (`upe_trigger_system.h`)
**Purpose**: Non-solid volumes for gameplay events

**Trigger Types**:
- Geometric: Box, sphere, capsule, convex
- Field-based: Temperature zones, radiation zones, sound zones

**Events** (deterministically ordered):
- Enter, Stay, Exit
- Sub-frame timing (for CCD)

**Field Triggers**:
- Above/below threshold
- Within range
- High gradient (rapid change)

### 6. Debug Visualization (`upe_debug_draw.h`)
**Purpose**: Engine-native debug rendering (dependency-free)

**Primitives**: Lines, arrows, points, spheres, boxes, capsules, text, axes, grids

**Physics Visualizations**:
- Contacts (position, normal, impulse magnitude)
- Constraint errors
- Rigid body states (velocity, angular velocity)
- Scalar fields (height/color maps)
- Vector fields (arrow grids)
- Temperature maps (color-coded)
- EM field lines
- Particles, cloth, ropes
- Collision shapes, AABBs
- Solver islands (colored)
- Fracture patterns
- Wind flow
- Stress/strain tensors

### 7. Adaptive Multi-Resolution Grids (`upe_amr_grid.h`)
**Purpose**: Variable-resolution spatial discretization

**Structure**: Octree-based (8 children per interior node)

**Refinement Criteria**:
- Gradient-based (high field gradient)
- Curvature-based
- Distance from surface/point
- Manual region specification

**Key Features**:
- Deterministic refinement (seeded tie-breaking)
- Boundary stitching (no seams, no energy spikes)
- Morton code ordering (cache-friendly iteration)
- Efficient leaf cell iteration

**Use Cases**:
- Fluid simulation (high res near splashes)
- Gas diffusion (high res near sources)
- Temperature fields (high res near heat sources)
- EM fields (high res near charges)

### 8. Persistent Contact Manifolds (`upe_manifold.h`)
**Purpose**: Stable, feature-based contact caching

**Feature Types**:
- Vertex (point)
- Edge (line segment)
- Face (planar region)

**Manifold Structure**:
- Up to 4 contact points per manifold
- Feature IDs for cross-frame matching
- Accumulated impulses (warm-starting)
- Static vs dynamic friction state

**Benefits**:
- Stable stacking (no jitter)
- Reduced penetration drift
- Better friction behavior
- Faster convergence

**Contact Generation**:
- Sphere-sphere: 1 point
- Box-box: Up to 4 points (face-face, edge-edge, vertex-face)
- Capsule-capsule: Up to 2 points

## Advanced Physics Domains

### Soft Bodies
- **PBD** (`upe_pbd.h`): Position-based dynamics (cloth, ropes)
- **FEM** (`upe_fem.h`): Finite element method (tetrahedral meshes)
- **MPM** (`upe_mpm.h`): Material point method (snow, sand)

### Fluids
- **SPH**: Smoothed particle hydrodynamics (existing)
- **FLIP** (`upe_flip.h`): Hybrid grid-particle (splashes, foam)
- **Shallow Water** (`upe_shallow_water.h`): Height-field waves, flooding

### Gases & Smoke
- **Gas** (`upe_gas.h`): Navier-Stokes, buoyancy, vorticity confinement
- **Fire** (in multiphysics): Combustion, fuel consumption, heat release

### Granular & Terrain
- **DEM** (`upe_dem.h`): Discrete element method (sand, gravel)
- **Erosion** (`upe_erosion.h`): Hydraulic, thermal, sediment transport

### Destruction
- **Fracture** (`upe_fracture.h`): Voronoi, radial, stress-based
- **Continuum Damage** (`upe_continuum_damage.h`): Progressive failure, crack propagation

### Environmental
- **Wind** (field-based): Turbulence, boundary layers, obstacles
- **Thermal**: Temperature, phase change, thermal stress
- **Acoustics** (`upe_acoustics.h`): Sound propagation, occlusion, modal synthesis
- **Optics** (`upe_optics.h`): Caustics, refraction, scattering

### Forces & Fields
- **Electromagnetism** (`upe_electromagnetism.h`): Coulomb, Lorentz, eddy currents
- **Chemistry** (`upe_chemistry.h`): Reaction-diffusion, Gray-Scott patterns

### Characters & Crowds
- **Articulation** (`upe_articulation.h`): Ragdolls, IK (CCD, FABRIK), biomechanics
- **Crowd** (`upe_crowd.h`): Social forces, density fields, panic
- **Swarm** (`upe_swarm.h`): Boids, flocking, predator-prey

### Vehicles
- **Advanced Vehicles** (`upe_vehicle_advanced.h`): Tank tracks, differentials, AWD

### Space Physics
- **Orbital Mechanics** (`upe_space_physics.h`): Kepler orbits, Lagrange points
- **Black Hole** (future): Schwarzschild metrics, tidal forces

## Performance & Determinism

### Memory Management
**Memory Pools** (planned):
- Arena allocator per frame
- Persistent pools for bodies/constraints
- Zero malloc() during simulation

**Allocation Strategy**:
- Pre-allocate solver scratch buffers
- Grow capacity exponentially (2x)
- Compact/defrag during quiescent periods

### Determinism Mechanisms (Design Intent)

**Deterministic Ordering**:
- Sort contacts/pairs/constraints by stable keys
- Deterministic broadphase (sweep-and-prune with tie-breaking)
- Deterministic PRNG (PCG-based, per-system streams)

**Floating-Point Control**:
- Consistent FP rounding modes
- Denormals-are-zero on supported platforms
- Overflow/NaN guards in hot paths

**State Hashing and Replay Signals**:
- Frame-by-frame state hashes (Blake3 or similar)
- Compare within and across selected compilers/platforms as a visibility signal
- Replay validation

### Certification Harness (planned, `upe_certification.h`)

**Test Suites**:
- **Torture Tests**: 10k stack, bullet-through-paper, ragdoll pile
- **Long-Run Drift**: 1M steps, energy tracking
- **Cross-Compiler Matrix**: MSVC, GCC, Clang outputs compared
- **Repro Packs**: Binary state + inputs for bug reproduction

**Golden Results**:
- Blessed reference outputs per test
- Hash comparison (bitwise identical)
- Regression gates (no test may degrade)

## Scalability

### LOD (Level of Detail)

Each domain declares:
- **LOD Ladder**: Low/Med/High/Scientific fidelity modes
- **Error Metric**: How to decide when to upgrade/downgrade
- **Cost Model**: CPU/memory budget per mode

**Example (Fluid)**:
- LOD 0: Disabled
- LOD 1: 32³ grid, 5 iterations
- LOD 2: 64³ grid, 10 iterations
- LOD 3: 128³ grid, 20 iterations, vorticity confinement
- LOD 4: AMR grid, sparse solver, subgrid turbulence

### Parallelism (future)

**Constraint Graph Coloring**:
- Color constraints by independence
- Solve each color in parallel
- Deterministic reduction across threads

**Island Parallelism**:
- Independent islands solve in parallel
- No inter-island communication

**Task Scheduler**:
- Deterministic job ordering mode
- Work-stealing with overflow to single-thread fallback

## Bindings & Tools

### Bindings (planned)
- **C**: Native API
- **C++**: RAII wrappers, operator overloads
- **Python**: NumPy integration, Jupyter notebooks
- **C#**: .NET bindings for Unity/Godot

### Authoring Tools ("Physics Recipe System")

**Designer Interface**:
- Material assignment (drag-drop presets)
- Field emitters (temperature, wind, radiation)
- Trigger placement (zone editors)
- Environmental presets (desert, ocean, space, arctic)

**Export Format**:
- Deterministic binary blobs
- JSON/YAML for human-readable
- Hot-reload safe (diff + rollback)

## Module Dependency Graph

```
Core Layer:
  upe_math.h
  upe_allocator.h
  
Infrastructure:
  upe_field_system.h
  upe_material_system.h
  upe_coupling_contract.h
  upe_amr_grid.h
  
Collision:
  upe_broadphase.h
  upe_narrowphase.h
  upe_manifold.h
  upe_scene_query.h
  upe_convex.h
  
Rigid Bodies:
  upe_rigidbody.h
  upe_joint_advanced.h
  upe_manifold.h
  
Soft Bodies:
  upe_pbd.h → upe_field_system.h
  upe_fem.h → upe_material_system.h
  upe_mpm.h → upe_amr_grid.h
  
Fluids:
  upe_sph.h → upe_field_system.h
  upe_flip.h → upe_amr_grid.h
  upe_shallow_water.h
  
Environmental:
  upe_thermal.h → upe_field_system.h
  upe_wind.h → upe_field_system.h
  upe_gas.h → upe_amr_grid.h
  
Gameplay:
  upe_trigger_system.h → upe_scene_query.h
  upe_crowd.h → upe_field_system.h
  
Debug:
  upe_debug_draw.h (no dependencies)
  upe_profiler.h (no dependencies)
```

## Future Extensions

### Quantum Mechanics Toy Model
- Wavefunction collapse approximations
- Entanglement "spooky action" for gameplay

### Biological Systems
- Cellular automata (Conway's Life, reaction-diffusion)
- Neural signal propagation (axon potentials)
- Muscle fiber contraction (Hill muscle model)

### Economic/Social Systems
- Supply/demand flow fields
- Influence propagation (viral spread, opinion dynamics)

### Metamaterials
- Negative index of refraction
- Programmable stiffness/damping
- Active shape-changing structures

---

**License**: Proprietary / Exclusive evaluation (see `LICENSE`)

**Contributors**: List core contributors

**Citation**: If used in research/papers
