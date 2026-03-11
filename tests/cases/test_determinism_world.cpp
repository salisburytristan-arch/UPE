#include "test_framework.h"
#include "upe/upe_api.h"
#include "upe/upe_world_body_api.h"
#include "upe/upe_body.h"
#include "upe/upe_shape.h"
#include "upe/upe_determinism.h"

static void setup_world(UpeWorld *world) {
    UpeBodyDesc body_desc = {};
    body_desc.type = UPE_BODY_DYNAMIC;
    body_desc.position = {0.0f, 1.0f, 0.0f};
    body_desc.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    body_desc.mass = 1.0f;
    body_desc.collision_layer = 1;
    body_desc.collision_mask = 0xFFFFFFFFu;

    uint32_t body_id = UpeWorld_CreateBody(world, &body_desc);
    UpeBody *body = UpeWorld_GetBody(world, body_id);
    if (body) {
        body->velocity = {1.0f, 0.0f, 0.5f};
    }

    UpeShapeDesc shape_desc = {};
    shape_desc.type = UPE_SHAPE_SPHERE;
    shape_desc.body_id = body_id;
    shape_desc.sphere.center = {0.0f, 0.0f, 0.0f};
    shape_desc.sphere.radius = 0.5f;
    shape_desc.friction = 0.6f;
    shape_desc.restitution = 0.2f;
    UpeWorld_CreateShape(world, &shape_desc);

    UpeBodyDesc body_desc2 = body_desc;
    body_desc2.position = {2.0f, 1.0f, 0.0f};
    uint32_t body_id2 = UpeWorld_CreateBody(world, &body_desc2);
    UpeBody *body2 = UpeWorld_GetBody(world, body_id2);
    if (body2) {
        body2->velocity = {-0.3f, 0.0f, -0.2f};
    }

    UpeShapeDesc shape_desc2 = shape_desc;
    shape_desc2.body_id = body_id2;
    shape_desc2.sphere.radius = 0.4f;
    UpeWorld_CreateShape(world, &shape_desc2);
}

TEST(TestDeterminismWorldHash) {
    UpeWorldDesc desc = {};
    desc.fixed_dt = 1.0f / 60.0f;

    // Per-frame hash compare over 1000 steps (playback-equivalent)
    UpeWorld *world_a = UpeWorld_Create(&desc);
    UpeWorld *world_b = UpeWorld_Create(&desc);
    REQUIRE(world_a != nullptr);
    REQUIRE(world_b != nullptr);

    setup_world(world_a);
    setup_world(world_b);

    for (int i = 0; i < 1000; ++i) {
        UpeWorld_Step(world_a);
        UpeWorld_Step(world_b);
        UpeStateHash ha = UpeDeterminism_HashState(world_a);
        UpeStateHash hb = UpeDeterminism_HashState(world_b);
        REQUIRE(UpeDeterminism_CompareHash(ha, hb) == 1);
    }

    UpeWorld_Destroy(world_a);
    UpeWorld_Destroy(world_b);

    // 10-run consistency check
    UpeStateHash hashes[10];
    for (int run = 0; run < 10; ++run) {
        UpeWorld *world = UpeWorld_Create(&desc);
        REQUIRE(world != nullptr);
        setup_world(world);
        for (int i = 0; i < 120; ++i) {
            UpeWorld_Step(world);
        }
        hashes[run] = UpeDeterminism_HashState(world);
        UpeWorld_Destroy(world);
    }
    for (int run = 1; run < 10; ++run) {
        REQUIRE(UpeDeterminism_CompareHash(hashes[0], hashes[run]) == 1);
    }
}
