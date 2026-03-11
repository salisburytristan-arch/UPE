# UPE Architecture

Status: NOT IN ACCEPTANCE SCOPE
Last verified: 2026-02-18
Script to validate: N/A

## High-Level Design

```
┌─────────────────────────────────────────────────────────────┐
│                     User Application                        │
└───────────────────────────┬─────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                   C API (upe_*.h)                           │
│  World, Body, Shape, Joint, Material, Cloth, Fluid, etc.   │
└───────────────────────────┬─────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                 Physics Kernel (Core Loop)                  │
│  Broadphase → Narrowphase → Solver → Integrator            │
└───────────────────────────┬─────────────────────────────────┘
                            │
        ┌───────────────────┼───────────────────┐
        ▼                   ▼                   ▼
┌──────────────┐   ┌──────────────┐   ┌──────────────┐
│  Collision   │   │  Constraints │   │  Integrators │
│   System     │   │   & Solver   │   │   & Forces   │
└──────────────┘   └──────────────┘   └──────────────┘
        │                   │                   │
        └───────────────────┴───────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│              Base Systems (Math, Memory, Tasks)             │
│  Vec3, Mat3, Quat, Transform, Allocator, TaskGraph, RNG    │
└─────────────────────────────────────────────────────────────┘
```

## Module Breakdown

### Base (`core/src/base/`)
- **upe_allocator.h**: Stack, pool, and arena allocators
- **upe_array.h**: Dynamic array container
- **upe_hash_map.h**: Deterministic hash table
- **upe_task.h**: Task graph for parallelism
- **upe_rng.h**: Xorshift32 PRNG
- **upe_robustness.h**: NaN guards, safe math

### Math (`core/src/math/`)
- **upe_vec2/3/4.h**: Vector operations
- **upe_mat2/3/4.h**: Matrix operations
- **upe_quat.h**: Quaternion rotation
- **upe_transform.h**: SE2/SE3 spatial transforms
- **upe_units.h**: Unit conversions
- **upe_epsilon.h**: Floating-point comparison policy

### Collision (`core/src/collision/`)
- **upe_broadphase_sap.h**: Sweep-and-prune broadphase
- **upe_broadphase_bvh.h**: BVH acceleration structure
- **upe_narrowphase.h**: Primitive collision tests
- **upe_gjk.h**: GJK distance algorithm
- **upe_epa.h**: EPA penetration depth
- **upe_trimesh.h**: Triangle mesh collision
- **upe_heightfield.h**: Terrain heightfield
- **upe_queries.h**: Raycast, sphere cast, shape cast
- **upe_triggers.h**: Overlap volumes without resolution

### Constraints (`core/src/constraints/`)
- **upe_solver_pgs.h**: PGS iterative solver
- **upe_friction.h**: Friction constraint
- **upe_islands.h**: Sleep/wake connectivity
- **upe_ccd.h**: Continuous collision detection
- **upe_joint.h**: Hinge, slider, ball, 6DOF joints
- **upe_joint_limits.h**: Min/max limits, motors
- **upe_warmstart.h**: Lambda caching for convergence
- **upe_solver_stabilization.h**: Baumgarte stabilization

### World (`core/src/world/`)
- **upe_world.h**: Simulation world container with bodies, shapes, joints
- **upe_world_body_api.h** / **upe_world_body_api.cpp**: Dynamic joint storage with UpeWorld_AddJoint() and UpeWorld_GetJointCount()
- **upe_body.h**: Rigid body state and properties
- **upe_shape.h**: Collision shape definitions (sphere, capsule, box, compound)
- **upe_mass.h**: Inertia tensor computation
- **upe_integrator.h**: Euler, semi-implicit Euler with momentum conservation
- **upe_material.h**: Friction/restitution properties
- **upe_physics_kernel.h**: Core simulation loop orchestration
- **upe_vehicle.h** / **upe_vehicle.cpp**: Vehicle system with raycast-based suspension per wheel (UpeVehicle_UpdateSuspensionWithWorld), Pacejka tire model, force application at contact points (UpeVehicle_ApplyForceAtPoint)
- **upe_articulated.h** / **upe_articulated.cpp**: Articulated bodies with automatic world integration, link position queries (UpeArticulatedBody_GetLinkPosition), impulse application (UpeArticulatedBody_ApplyLinkImpulse), velocity control (UpeArticulatedBody_SetRootVelocity)

### Tools (`core/src/tools/`)
- **upe_frame.h**: Frame counter and time tracking
- **upe_replay.h**: Deterministic replay stream
- **upe_snapshot.h**: World state capture/restore
- **upe_serialize.h**: Binary serialization
- **upe_parallel.h**: Deterministic task graph
- **upe_vehicle.h**: Vehicle suspension system
- **upe_character.h**: Character controller with raycast ground detection and step-up logic
- **upe_animation.h**: Animation controller with state machine and IK solver (CCD-based iterative solver)
- **upe_net_delta.h**: Network delta compression

### Domains (`core/src/domains/`)
- **upe_cloth.h**: Position-based cloth (XPBD)
- **upe_particles.h**: Particle system with emitters
- **upe_fluid_sph.h**: SPH fluid dynamics
- **upe_thermal.h**: Heat diffusion simulation
- **upe_waves.h**: 2D shallow water waves
- **upe_breakable.h**: Fracture mechanics
- **upe_granular.h**: Granular material dynamics
- **upe_aerodynamics.h**: Drag/lift forces
- **upe_radiation.h**: Thermal radiation
- **upe_combustion.h**: Reactive combustion
- **upe_orbit.h**: Orbital mechanics (n-body gravity)
- **upe_plasma.h**: Plasma physics (Lorentz force)

## Data Flow

### Frame N Execution
```
1. Broadphase Update
   ├─ Update AABB bounds for all shapes
   ├─ Sort endpoints (SAP) or refit tree (BVH)
   └─ Generate potential collision pairs

2. Narrowphase Collision Detection
   ├─ Test each pair with primitive tests or GJK
   ├─ Generate contact manifolds
   └─ Apply trigger callbacks (no resolution)

3. Island Detection
   ├─ Build connectivity graph from contacts/joints
   ├─ Find connected components
   └─ Mark sleeping islands

4. Constraint Solver (PGS)
   ├─ Warm-start from previous frame lambdas
   ├─ Iterate N times:
   │  ├─ Solve contacts (penetration, friction)
   │  └─ Solve joints (limits, motors)
   └─ Apply impulses to bodies

5. Integration
   ├─ Apply gravity and external forces
   ├─ Integrate velocity: v' = v + (F/m)*dt
   ├─ Integrate position: p' = p + v'*dt
   ├─ Integrate rotation: q' = q + 0.5*ω*q*dt
   └─ Clear forces

6. Post-Step
   ├─ Update transforms
   ├─ Write replay frame (hash, inputs)
   └─ Increment frame counter
```

## Memory Layout

### World Structure
```c
struct UpeWorld {
    float time;
    float fixed_dt;
    UpeVec3 gravity;
    
    UpeBody* bodies;
    uint32_t body_count;
    uint32_t body_capacity;
    
    UpeShape* shapes;
    uint32_t shape_count;
    
    UpeContactManifold* contacts;
    uint32_t contact_count;
    
    UpeJoint* joints;
    uint32_t joint_count;
    
    UpeBroadphaseSap* broadphase;
    UpeSolverPgs* solver;
    UpeIntegrator* integrator;
    
    UpeReplayStream* replay;
};
```

### Cache-Friendly Layout
- Bodies stored as SoA (Structure of Arrays) for SIMD-friendly access
- Contacts/joints grouped by island for cache locality
- Hot data (position, velocity) separated from cold data (metadata)

## Extension Points

### Custom Shapes
```c
void MyCustomShape_Collide(UpeShape* a, UpeShape* b, UpeContactManifold* out);
UpeWorld_RegisterShapeCollider(world, MY_CUSTOM_SHAPE, MyCustomShape_Collide);
```

### Custom Constraints
```c
void MyConstraint_Solve(UpeConstraint* constraint, float dt);
UpeWorld_AddConstraint(world, MY_CONSTRAINT_TYPE, MyConstraint_Solve);
```

### Custom Forces
```c
void MyForceField_Apply(UpeBody* body, float dt);
UpeWorld_RegisterForceCallback(world, MyForceField_Apply);
```

## Compilation Units
- **upe.lib**: Single static library containing all modules
- **upe_tests.exe**: Test harness with 60+ unit tests
- **minimal_2d.exe** / **minimal_3d.exe**: Example programs
