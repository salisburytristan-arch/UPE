#include "upe/upe_api.h"
#include "upe/upe_world_body_api.h"

#include <cstdio>

static uint32_t AddBox(UpeWorld *world, float x, float y, float z, float half_extent) {
    UpeBodyDesc body = {};
    body.type = UPE_BODY_DYNAMIC;
    body.position = {x, y, z};
    body.rotation = UpeQuat_Identity();
    body.mass = 1.0f;
    uint32_t body_id = UpeWorld_CreateBody(world, &body);

    UpeShapeDesc shape = {};
    shape.type = UPE_SHAPE_BOX;
    shape.body_id = body_id;
    shape.box.min = {-half_extent, -half_extent, -half_extent};
    shape.box.max = {half_extent, half_extent, half_extent};
    UpeWorld_CreateShape(world, &shape);
    return body_id;
}

int main() {
    std::printf("=== UPE Rigidbody Stack Demo ===\n");

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
    ground_shape.box.min = {-5.0f, -0.5f, -5.0f};
    ground_shape.box.max = {5.0f, 0.5f, 5.0f};
    UpeWorld_CreateShape(world, &ground_shape);

    uint32_t top_body_id = 0;
    for (int i = 0; i < 10; ++i) {
        top_body_id = AddBox(world, 0.0f, 0.6f + (float)i * 1.05f, 0.0f, 0.5f);
    }

    for (int step = 0; step < 180; ++step) {
        UpeWorld_Step(world);
    }

    UpeBody *top_body = UpeWorld_GetBody(world, top_body_id);
    UpeWorldStepMetrics metrics = {};
    UpeWorld_GetStepMetrics(world, &metrics);

    if (top_body) {
        std::printf("Top body position: (%.3f, %.3f, %.3f)\n",
                    top_body->position.x,
                    top_body->position.y,
                    top_body->position.z);
    }
    std::printf("Bodies active: %u, contacts: %u, state hash: 0x%llX\n",
                metrics.active_dynamic_bodies,
                metrics.contacts,
                (unsigned long long)metrics.state_hash);

    UpeWorld_Destroy(world);
    return 0;
}