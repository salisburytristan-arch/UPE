# API Reference (Buyer Kit)

## Core World & Body Lifecycle

### World Creation and Control
- `UpeWorld *UpeWorld_Create(const UpeWorldDesc *desc)` — Create a physics world with configurable timestep, solver iterations, and feature flags.
- `void UpeWorld_Destroy(UpeWorld *world)` — Clean up all world resources.
- `void UpeWorld_Step(UpeWorld *world)` — Advance simulation by the configured fixed timestep. Deterministic when built with `DUPE_DETERMINISTIC=ON`.

### Body Management
- `uint32_t UpeWorld_CreateBody(UpeWorld *world, const UpeBodyDesc *desc)` — Create a dynamic, kinematic, or static body. Returns stable handle.
- `void UpeWorld_DestroyBody(UpeWorld *world, uint32_t body_id)` — Destroy body without reindexing (generation-based safety).
- `UpeBody *UpeWorld_GetBody(UpeWorld *world, uint32_t body_id)` — Retrieve body pointer by handle.
- `void UpeWorld_ApplyForce(UpeWorld *world, uint32_t body_id, UpeVec3 force)` — Apply force for next step.
- `void UpeWorld_ApplyTorque(UpeWorld *world, uint32_t body_id, UpeVec3 torque)` — Apply rotational torque.
- `void UpeWorld_ApplyImpulse(UpeWorld *world, uint32_t body_id, UpeVec3 impulse)` — Apply instantaneous impulse.

### Shape Management
- `uint32_t UpeWorld_CreateShape(UpeWorld *world, const UpeShapeDesc *desc)` — Attach sphere, capsule, box, trimesh, or compound shape to body.
- `void UpeWorld_DestroyShape(UpeWorld *world, uint32_t shape_id)` — Destroy shape.

### Joint & Constraint System
- `uint32_t UpeWorld_AddJoint(UpeWorld *world, const UpeJointDesc *desc)` — Add joint (hinge, slider, ball, 6DOF, distance, spring, cone-twist, gear, pulley) to world.
- `uint32_t UpeWorld_GetJointCount(UpeWorld *world)` — Query current joint count.

## Queries (Physics Raycasting & Shape Tests)

### Raycasting
- `int UpeWorld_Raycast(UpeWorld *world, UpeVec3 origin, UpeVec3 direction, float max_distance, UpeQueryFilter *filter, UpeRaycastResult *result)` — Single raycast. Returns closest hit.
- Supports collision layer/mask filtering and user-defined callbacks.

### Overlap & Sweep
- `int UpeWorld_OverlapShape(...)` — Test shape overlap with scene.
- `int UpeWorld_SweepShape(...)` — Sweep shape and find first contact.

## Articulated Bodies (Ragdolls, Skeletal Systems)

### Articulated Body Creation
- `UpeArticulatedBody *UpeArticulatedBody_Create(UpeWorld *world, const UpeArticulatedDesc *desc)` — Create skeletal chain. If world is provided, automatically creates bodies and joints; if NULL, creates topology-only structure.
- `void UpeArticulatedBody_Destroy(UpeWorld *world, UpeArticulatedBody *body)` — Clean up articulated body.

### Articulated Body Helpers
- `UpeVec3 UpeArticulatedBody_GetLinkPosition(UpeArticulatedBody *body, uint32_t link_idx, UpeWorld *world)` — Get world position of link.
- `void UpeArticulatedBody_ApplyLinkImpulse(UpeArticulatedBody *body, uint32_t link_idx, UpeVec3 impulse, UpeWorld *world)` — Apply impulse to specific link.
- `void UpeArticulatedBody_SetRootVelocity(UpeArticulatedBody *body, UpeVec3 velocity, UpeWorld *world)` — Set root body velocity (propagates to children via constraints).

## Vehicles

### Vehicle Setup
- `void UpeVehicle_Init(UpeVehicle *vehicle, const UpeVehicleDesc *desc)` — Initialize vehicle with chassis, wheel count, suspension stiffness/damping.

### Suspension & Ground Contact
- `void UpeVehicle_UpdateSuspension(UpeVehicle *vehicle, UpeBody *chassis, float dt)` — Legacy plane-based suspension (fallback behavior preserved).
- `void UpeVehicle_UpdateSuspensionWithWorld(UpeVehicle *vehicle, UpeBody *chassis, UpeWorld *world, float dt)` — World-aware suspension: raycasts per wheel to dynamically find ground contact point, normal, and compression.

### Tire Model & Forces
- `void UpeVehicle_UpdateTireForces(UpeVehicle *vehicle, UpeBody *chassis, float dt)` — Compute tire slip angles/ratios and apply Pacejka-based lateral/longitudinal forces at wheel contact points. Updates wheel angular velocity.
- `float UpeVehicle_CalculatePacejkaLateral(float slip_angle, float load, float friction)` — Simplified Pacejka magic formula for lateral tire grip.
- `float UpeVehicle_CalculatePacejkaLongitudinal(float slip_ratio, float load, float friction)` — Simplified Pacejka formula for longitudinal traction.

### Input & Steering
- `void UpeVehicle_ApplyInput(UpeVehicle *vehicle, float steering_angle, float drive_power, float brake_power)` — Set steering, drive, and brake inputs for next update.
- `void UpeVehicle_ApplyAckermannSteering(UpeVehicle *vehicle, float steering_input, float wheelbase, float track_width)` — Apply realistic Ackermann steering geometry.

### Diagnostics
- `float UpeVehicle_GetWheelCompressionRatio(const UpeVehicle *vehicle, uint32_t wheel_idx)` — Read wheel compression as ratio of max suspension travel.

## Snapshot & Replay

### State Capture
- `UpeSnapshot *UpeSnapshot_Create(UpeWorld *world)` — Capture world state (bodies, velocities, shapes).
- `void UpeSnapshot_Apply(UpeWorld *world, UpeSnapshot *snapshot)` — Restore world to captured state.
- `void UpeSnapshot_Destroy(UpeSnapshot *snapshot)` — Free snapshot.

### Deterministic Replay
- `UpeReplayRecorder *UpeReplayRecorder_Create()` — Create replay stream for input/output logging.
- `void UpeWorld_SetReplayRecorder(UpeWorld *world, UpeReplayRecorder *replay)` — Attach recorder to world.
- `void UpeReplayRecorder_Play(UpeWorld *world, UpeReplayRecorder *replay)` — Play back recorded steps.

### State Hashing
- `UpeStateHash UpeStateHash_Begin(UpeStateHash *hash, uint64_t seed)` — Begin state hash.
- `void UpeStateHash_PushBytes(UpeStateHash *hash, const void *data, uint32_t len)` — Add data to hash.
- `uint64_t UpeStateHash_Final(UpeStateHash *hash)` — Finalize and return CRC64 hash (for determinism validation).

## Animation & Character Control

### Character Controller
- `void UpeCharacterController_Init(UpeCharacterController *controller, const UpeCharacterDesc *desc)` — Initialize character with raycast-driven ground detection, step-up logic.
- `void UpeCharacterController_Update(UpeCharacterController *controller, UpeWorld *world, UpeVec3 desired_velocity, float dt)` — Update character movement, ground contact, stepping, and platform velocity inheritance.

### IK Solver
- `void UpeAnimationController_AddIKConstraint(UpeAnimationController *controller, UpeIKConstraint constraint)` — Add IK target constraint to limb chain.
- `void UpeAnimationController_SolveIK(UpeAnimationController *controller)` — Solve all active IK constraints (iterative CCD-style solver with per-constraint damping and convergence checks).

## Fluid Simulation (SPH)

### SPH Setup & Stepping
- `UpeFluidSystem *UpeFluid_Create(const UpeFluidDesc *desc)` — Create SPH fluid system.
- `void UpeFluid_Step(UpeFluidSystem *fluid, UpeWorld *world, float dt)` — Advance fluid particles (FLIP/PIC hybrid with boundary handling, deterministic RNG).
- `void UpeFluid_Destroy(UpeFluidSystem *fluid)` — Clean up fluid resources.

## Memory & Diagnostics

### Memory Context
- `UpeMemoryContext *UpeMemoryContext_Create()` — Create memory tracking context.
- `UpeMemoryDiag *UpeMemoryDiag_GetSnapshot(UpeMemoryContext *ctx)` — Capture current memory usage (live, peak, fragments).

### Profiler
- `UpeProfiler *UpeProfiler_Create()` — Begin profiling.
- `void UpeProfiler_ExportChromeTrace(UpeProfiler *p, const char *filename)` — Export to Chrome DevTools timeline.

## Call Ordering Guarantees

1. **Create before use**: Bodies, shapes, joints, and other resources must be created before referenced in queries or step operations.
2. **Query after step**: Raycasts and overlap tests return valid data after `UpeWorld_Step()` has completed.
3. **Destroy after use**: Clean up resources in reverse-creation order (destroy shapes before bodies, destroy bodies before world).

## Error Handling Conventions

- **Invalid IDs**: Queries with invalid body/shape/joint IDs return sentinel values (typically `0xFFFFFFFF`) or NULL.
- **Null pointers**: Passing NULL to functions expecting valid pointers is treated as a hard error (may cause immediate crash or undefined behavior). Always validate inputs before calling API.
- **Out-of-bounds**: Array/buffer indices are not checked; ensure bounds before calling indexed access functions.

## Header Locations

- Public C API headers: `core/include/upe/`
- Example integration: `examples/*/main.cpp`
- Test usage: `tests/cases/` and `tests/test_*.cpp`
