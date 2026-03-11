# Integration Guide

## Quick-Start: Rigid Body + Collision

```c
#include "upe/upe_core.h"

// 1. Create world
UpeWorldDesc world_desc = {0};
world_desc.timestep = 0.016f;  // 60 Hz
world_desc.gravity = (UpeVec3){0, -9.81f, 0};
UpeWorld *world = UpeWorld_Create(&world_desc);

// 2. Create static ground
UpeBodyDesc ground_desc = {0};
ground_desc.type = UPE_BODY_STATIC;
ground_desc.position = (UpeVec3){0, -5, 0};
uint32_t ground_id = UpeWorld_CreateBody(world, &ground_desc);

UpeShapeDesc ground_shape = {0};
ground_shape.type = UPE_SHAPE_BOX;
ground_shape.box = (UpeBox){1000, 1, 1000};
UpeWorld_AddShape(world, ground_id, &ground_shape);

// 3. Create dynamic sphere
UpeBodyDesc ball_desc = {0};
ball_desc.type = UPE_BODY_DYNAMIC;
ball_desc.position = (UpeVec3){0, 5, 0};
ball_desc.mass = 1.0f;
uint32_t ball_id = UpeWorld_CreateBody(world, &ball_desc);

UpeShapeDesc ball_shape = {0};
ball_shape.type = UPE_SHAPE_SPHERE;
ball_shape.sphere_radius = 0.5f;
UpeWorld_AddShape(world, ball_id, &ball_shape);

// 4. Step loop
for (int frame = 0; frame < 600; frame++) {
    UpeWorld_Step(world);
    
    // Read position back
    UpeBodyState state = {0};
    UpeWorld_GetBodyState(world, ball_id, &state);
    printf("Frame %d: ball at (%.2f, %.2f, %.2f)\n", 
           frame, state.position.x, state.position.y, state.position.z);
}

// 5. Cleanup
UpeWorld_Destroy(world);
```

## Integration Pattern: Host-Stepping-World

1. **Create world** with desired timestep (e.g., 0.016s for 60 Hz).
2. **Create bodies** (static ground, dynamic objects) and add shapes.
3. **Configure layers/masks** to control collisions between body pairs.
4. **Each frame**: Call `UpeWorld_Step(world)` once.
5. **Read results** via `UpeWorld_GetBodyState()` or `UpeWorld_RaycastFirst()`.
6. **Cleanup** in reverse order of creation.

### Deterministic Execution

For rollback/netcode scenarios, initialize world with:
```c
world_desc.deterministic = UPE_DETERMINISTIC_ENABLED;
world_desc.rng_seed = 0xABCD1234;  // Same seed → same results
```

Then step world identically across instances:
```c
for (int i = 0; i < frame_count; i++) {
    UpeWorld_Step(world);
    // All clients produce bit-identical state
}
```

## Advanced: Vehicles with Suspension

```c
// Create vehicle with 4 wheels
UpeVehicleDesc vehicle_desc = {0};
vehicle_desc.chassis_body_index = chassis_id;
vehicle_desc.num_wheels = 4;
vehicle_desc.wheel_friction = 1.5f;
vehicle_desc.suspension_stiffness = 50000.0f;
vehicle_desc.suspension_damping = 2000.0f;
vehicle_desc.suspension_rest_length = 0.5f;

// Set wheel contact points (relative to chassis origin)
vehicle_desc.wheel_local_pos[0] = (UpeVec3){-0.8f, -0.5f, 1.2f};   // Front-left
vehicle_desc.wheel_local_pos[1] = (UpeVec3){0.8f, -0.5f, 1.2f};    // Front-right
vehicle_desc.wheel_local_pos[2] = (UpeVec3){-0.8f, -0.5f, -1.2f};  // Rear-left
vehicle_desc.wheel_local_pos[3] = (UpeVec3){0.8f, -0.5f, -1.2f};   // Rear-right

uint32_t vehicle_id = UpeWorld_CreateVehicle(world, &vehicle_desc);

// Each frame: update suspension (raycast) and tire forces
UpeVehicle_UpdateSuspensionWithWorld(world, vehicle_id);

// Apply steering/throttle inputs
float steering = 0.3f;  // radians
float throttle = 5000.0f;  // force (N)
UpeVehicle_SetSteeringAndThrottle(vehicle_id, steering, throttle);

// Forces are applied to chassis automatically during suspension update
```

## Advanced: Articulated Bodies (Ragdolls/Skeletons)

```c
// Create articulated body with 3 links (head, torso, legs)
UpeArticulatedDesc artic_desc = {0};
artic_desc.num_links = 3;
artic_desc.world = world;  // Attach to world for auto body/joint creation

// Setup link transforms (relative to root)
artic_desc.link_mass[0] = 2.0f;   // Head
artic_desc.link_mass[1] = 8.0f;   // Torso
artic_desc.link_mass[2] = 5.0f;   // Legs

artic_desc.link_position[0] = (UpeVec3){0, 0.8f, 0};  // Head above torso
artic_desc.link_position[1] = (UpeVec3){0, 0, 0};     // Torso at origin
artic_desc.link_position[2] = (UpeVec3){0, -0.6f, 0}; // Legs below torso

uint32_t ragdoll_id = UpeArticulatedBody_Create(world, &artic_desc);

// Query link position in world space
UpeVec3 head_pos = UpeArticulatedBody_GetLinkPosition(world, ragdoll_id, 0);

// Apply impulse to a link (e.g., hit by falling object)
UpeVec3 impulse = {0, 100, 0};
UpeArticulatedBody_ApplyLinkImpulse(world, ragdoll_id, 1, impulse);  // Torso

// Set root velocity (e.g., character jump)
UpeVec3 velocity = {0, 10, 0};
UpeArticulatedBody_SetRootVelocity(world, ragdoll_id, velocity);
```

## Advanced: Custom Joints

```c
// Create a point constraint between two bodies
UpeJointDesc joint_desc = {0};
joint_desc.type = UPE_JOINT_POINT;
joint_desc.body_a_index = body1_id;
joint_desc.body_b_index = body2_id;
joint_desc.anchor_pos_a = (UpeVec3){0, 0.5f, 0};  // Local to body1
joint_desc.anchor_pos_b = (UpeVec3){0, -0.5f, 0}; // Local to body2
joint_desc.max_force = 1000.0f;
joint_desc.constraint_cfm = 1e-6f;  // Compliance (lower = stiffer)

uint32_t joint_id = UpeWorld_AddJoint(world, &joint_desc);

// Later, if needed, query joint count
uint32_t total_joints = UpeWorld_GetJointCount(world);
printf("World has %u joints\n", total_joints);
```

## Character Controller Integration

```c
// Create capsule character
UpeBodyDesc char_desc = {0};
char_desc.type = UPE_BODY_DYNAMIC;
char_desc.position = (UpeVec3){0, 2, 0};
char_desc.mass = 80.0f;  // kg
uint32_t char_id = UpeWorld_CreateBody(world, &char_desc);

UpeShapeDesc capsule_shape = {0};
capsule_shape.type = UPE_SHAPE_CAPSULE;
capsule_shape.capsule = (UpeCapsule){0.4f, 1.8f};  // radius, height
UpeWorld_AddShape(world, char_id, &capsule_shape);

// Each frame: detect ground, apply movement
UpeRaycastQuery raycast = {0};
raycast.origin = state.position;
raycast.direction = (UpeVec3){0, -1, 0};
raycast.distance = 0.5f;  // Cast down 0.5m

UpeRaycastResult result = {0};
bool grounded = UpeWorld_RaycastFirst(world, &raycast, &result);

if (grounded) {
    // Apply movement force
    UpeVec3 move_input = {0.3f, 0, 0};  // Normalized input
    UpeWorld_ApplyForce(world, char_id, move_input, 1000.0f);
}
```

## Memory Ownership Rules

- **World owns**: All bodies, shapes, joints, vehicles, articulated bodies created via `UpeWorld_Create*` APIs.
- **Caller owns**: External buffers passed to serialization/snapshot functions.
- **Destruction order**: Destroy bodies/joints before world; destroy world last.
- **Reference safety**: Body/joint IDs are stable within world lifetime; invalid after `UpeWorld_Destroy()`.

## Threading Model

- **Deterministic**: All physics steps are deterministic if same inputs are applied in same order.
- **Serialization-safe**: Call `UpeWorld_CaptureSnapshot()` only between `UpeWorld_Step()` calls, not during.
- **Mutation safety**: Do not add/remove bodies or change properties during a `UpeWorld_Step()` call.
- **Query safety**: Raycast/overlap queries are safe to call anytime (read-only).

## Serialization / Snapshot / Rollback

```c
// Capture snapshot
UpeSnapshotDesc snap_desc = {0};
snap_desc.world = world;
uint32_t snap_size = UpeSnapshot_CalculateSize(world);
uint8_t *snap_buffer = malloc(snap_size);
UpeSnapshot_Capture(world, snap_buffer, snap_size);

// ... Later, rollback to snapshot ...
UpeSnapshot_Restore(world, snap_buffer, snap_size);

// Hash world state (for netcode verification)
UpeHashDesc hash_desc = {0};
hash_desc.world = world;
uint64_t state_hash = UpeWorld_GetStateHash(world);
printf("Frame hash: 0x%016llx\n", state_hash);
free(snap_buffer);
```

## Queries (Raycast, Overlap, Sweep)

```c
// Raycast for single hit
UpeRaycastQuery ray = {0};
ray.origin = (UpeVec3){0, 10, 0};
ray.direction = (UpeVec3){0, -1, 0};
ray.distance = 20.0f;
ray.layer_mask = 0xFFFFFFFF;  // Hit all layers

UpeRaycastResult result = {0};
bool hit = UpeWorld_RaycastFirst(world, &ray, &result);
if (hit) {
    printf("Hit body %u at distance %.2f\n", result.body_index, result.distance);
}

// Overlap query (all shapes in sphere)
UpeOverlapQuery overlap = {0};
overlap.center = (UpeVec3){0, 0, 0};
overlap.radius = 5.0f;

UpeBodyIndex *bodies = malloc(1024 * sizeof(UpeBodyIndex));
uint32_t hit_count = UpeWorld_OverlapSphere(world, &overlap, bodies, 1024);
for (uint32_t i = 0; i < hit_count; i++) {
    printf("Overlapped body %u\n", bodies[i]);
}
free(bodies);
```

## Sub-System Controls

```c
// Enable only required systems
UpeWorldDesc world_desc = {0};
world_desc.enable_vehicles = true;       // If using vehicles
world_desc.enable_cloth = false;         // Disable if not needed
world_desc.enable_fluids = false;        // Disable if not needed
world_desc.deterministic = true;         // For rollback games

UpeWorld *world = UpeWorld_Create(&world_desc);
```

Disabling unused systems reduces memory overhead and step time.

## Best Practices

1. **Timestep consistency**: Use same `timestep` value across platforms and builds.
2. **Layer/mask configuration**: Set collision layers early; avoid runtime changes.
3. **Fixed stepping**: Accumulate wall-clock time and step world at fixed intervals.
4. **Snapshot frequency**: Capture snapshots every 2–10 frames; higher frequency = more memory.
5. **Determinism validation**: Use `UpeWorld_GetStateHash()` to verify netcode sync.
6. **Error handling**: Check return codes (negative = error) from create/add APIs.
7. **Memory budgets**: Use `UpeMemory_GetStats()` to track allocation patterns.
