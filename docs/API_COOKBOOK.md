# API Cookbook

This document collects short, copy-paste-ready examples for the most common UPE integration tasks.

## 1. Create a world and step it

```cpp
#include "upe/upe_api.h"

int main() {
    UpeWorldDesc desc = UpeWorldDesc_Default();
    desc.fixed_dt = 1.0f / 60.0f;
    desc.enable_determinism_mode = 1;

    UpeWorld *world = UpeWorld_Create(&desc);
    for (int i = 0; i < 120; ++i) {
        UpeWorld_Step(world);
    }
    UpeWorld_Destroy(world);
    return 0;
}
```

## 2. Create a rigid body stack

```cpp
#include "upe/upe_api.h"
#include "upe/upe_world_body_api.h"

UpeBodyDesc body = {};
body.type = UPE_BODY_DYNAMIC;
body.position = {0.0f, 2.0f, 0.0f};
body.rotation = UpeQuat_Identity();
body.mass = 1.0f;

UpeShapeDesc shape = {};
shape.type = UPE_SHAPE_BOX;
shape.box.min = {-0.5f, -0.5f, -0.5f};
shape.box.max = {0.5f, 0.5f, 0.5f};
```

See [../examples/rigidbody_stack_demo/main.cpp](../examples/rigidbody_stack_demo/main.cpp) for a complete scene.

## 3. Deterministic replay and state hash

```cpp
#include "upe/upe_api.h"
#include "upe/upe_replay.h"
#include "upe/upe_state_hash.h"

UpeReplayStream replay = {};
UpeReplay_Begin(&replay, 0xD1A5EEDULL);

UpeStateHash hash = {};
UpeStateHash_Begin(&hash, 0xD1A5EEDULL);

UpeReplay_NextStep(&replay);
UpeReplay_PushU32(&replay, 1234u);
UpeStateHash_PushU32(&hash, 1234u);

uint64_t digest = UpeReplay_Digest(&replay);
uint64_t final_hash = UpeStateHash_Final(&hash);
```

## 4. Snapshot and restore a state blob

```cpp
#include "upe/upe_snapshot.h"

struct SavedState {
    float time;
    uint32_t frame;
};

SavedState state = {1.5f, 42u};
UpeSnapshot *snapshot = UpeSnapshot_Create(&state, sizeof(state), 42u);

SavedState restored = {};
UpeSnapshot_Restore(snapshot, &restored, sizeof(restored));
UpeSnapshot_Destroy(snapshot);
```

## 5. Character controller plus advanced movement bridge

```cpp
#include "upe/upe_advanced_movement.h"
#include "upe/upe_character_controller.h"

UpeCharacterControllerDesc config = UpeCharacterController_GetDefaultConfig();
UpeCharacterController *controller = UpeCharacterController_Create(world, (UpeVec3){0.0f, 1.8f, 0.0f}, &config);
UpeAdvancedMovement *movement = UpeAdvancedMovement_Create(controller);

UpeCharacterController_SetMovementInput(controller, (UpeVec2){1.0f, 0.0f}, UPE_CHARCTRL_LOCOMOTION_WALKING);
UpeAdvancedMovement_UpdateSlopes(movement, world, 1.0f / 60.0f);
UpeAdvancedMovement_UpdateStairs(movement, world, 1.0f / 60.0f);
UpeCharacterController_Update(controller, 1.0f / 60.0f, world);
UpeCharacterController_ApplyVelocity(controller, world);
```

## 6. Debug draw output

```cpp
#include "upe/upe_debug_draw.h"

UpeDebugDrawSystem debug = {};
UpeDebug_Init(&debug);
UpeDebug_BeginFrame(&debug);
UpeDebug_Line(&debug, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 1.0f, 0.2f, 0.2f, 0.0f);
UpeDebug_Text(&debug, {0.0f, 1.0f, 0.0f}, "UPE", 1.0f, 1.0f, 1.0f, 0.0f);
UpeDebug_EndFrame(&debug);
```

## 7. Serialize arbitrary binary data

```cpp
#include "upe/upe_serialize.h"

uint8_t buffer[128] = {};
UpeSerializeContext ctx = {};
UpeSerialize_Init(&ctx, buffer, sizeof(buffer));
UpeSerialize_WriteU32(&ctx, 7u);
UpeSerialize_WriteF32(&ctx, 3.5f);
```

## 8. Where to go next

- [../examples/hello_world/main.cpp](../examples/hello_world/main.cpp)
- [../examples/fixed_timestep_demo/main.cpp](../examples/fixed_timestep_demo/main.cpp)
- [../examples/constraints_bridge_demo/main.cpp](../examples/constraints_bridge_demo/main.cpp)
- [../docs/INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md)
- [../API_REFERENCE.md](../API_REFERENCE.md)