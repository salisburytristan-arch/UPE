#include "upe/upe_api.h"
#include "upe/upe_world_body_api.h"

#include <cstdio>

int main() {
    std::printf("=== UPE Hello World ===\n");

    UpeWorldDesc desc = UpeWorldDesc_Default();
    desc.fixed_dt = 1.0f / 60.0f;
    desc.enable_determinism_mode = 1;

    UpeWorld *world = UpeWorld_Create(&desc);
    if (!world) {
        std::printf("Failed to create world\n");
        return 1;
    }

    UpeBodyDesc ground = {};
    ground.type = UPE_BODY_STATIC;
    ground.position = {0.0f, -0.5f, 0.0f};
    ground.rotation = UpeQuat_Identity();
    ground.mass = 0.0f;
    uint32_t ground_id = UpeWorld_CreateBody(world, &ground);

    UpeShapeDesc ground_shape = {};
    ground_shape.type = UPE_SHAPE_BOX;
    ground_shape.body_id = ground_id;
    ground_shape.box.min = {-1.0f, -0.5f, -1.0f};
    ground_shape.box.max = {1.0f, 0.5f, 1.0f};
    UpeWorld_CreateShape(world, &ground_shape);

    UpeWorld_Step(world);

    UpeWorldStepMetrics metrics = {};
    UpeWorld_GetStepMetrics(world, &metrics);

    std::printf("Created a world, stepped once, state hash = 0x%llX\n",
                (unsigned long long)metrics.state_hash);

    UpeWorld_Destroy(world);
    return 0;
}