#include "upe/upe_api.h"
#include "upe/upe_world_body_api.h"

#include <cstdio>

static UpeWorld *CreateDemoWorld() {
    UpeWorldDesc desc = UpeWorldDesc_Default();
    desc.fixed_dt = 1.0f / 60.0f;
    desc.enable_determinism_mode = 1;

    UpeWorld *world = UpeWorld_Create(&desc);
    if (!world) {
        return nullptr;
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
    ground_shape.box.min = {-5.0f, -0.5f, -5.0f};
    ground_shape.box.max = {5.0f, 0.5f, 5.0f};
    UpeWorld_CreateShape(world, &ground_shape);

    UpeBodyDesc ball = {};
    ball.type = UPE_BODY_DYNAMIC;
    ball.position = {0.0f, 5.0f, 0.0f};
    ball.rotation = UpeQuat_Identity();
    ball.mass = 1.0f;
    uint32_t ball_id = UpeWorld_CreateBody(world, &ball);

    UpeShapeDesc sphere = {};
    sphere.type = UPE_SHAPE_SPHERE;
    sphere.body_id = ball_id;
    sphere.sphere.center = {0.0f, 0.0f, 0.0f};
    sphere.sphere.radius = 0.35f;
    UpeWorld_CreateShape(world, &sphere);

    return world;
}

static uint64_t RunWorld(UpeWorld *world, uint32_t steps) {
    for (uint32_t i = 0; i < steps; ++i) {
        UpeWorld_Step(world);
    }

    UpeWorldStepMetrics metrics = {};
    UpeWorld_GetStepMetrics(world, &metrics);
    return metrics.state_hash;
}

int main() {
    std::printf("=== UPE Fixed Timestep Demo ===\n");

    UpeWorld *world_a = CreateDemoWorld();
    UpeWorld *world_b = CreateDemoWorld();
    if (!world_a || !world_b) {
        std::printf("Failed to create demo worlds\n");
        if (world_a) UpeWorld_Destroy(world_a);
        if (world_b) UpeWorld_Destroy(world_b);
        return 1;
    }

    uint64_t hash_a = RunWorld(world_a, 120u);
    uint64_t hash_b = RunWorld(world_b, 120u);

    std::printf("Fixed dt: 1/60 s\n");
    std::printf("Run A hash: 0x%llX\n", (unsigned long long)hash_a);
    std::printf("Run B hash: 0x%llX\n", (unsigned long long)hash_b);
    std::printf("Match: %s\n", (hash_a == hash_b) ? "yes" : "no");

    UpeWorld_Destroy(world_a);
    UpeWorld_Destroy(world_b);
    return 0;
}