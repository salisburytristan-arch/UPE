#include "test_framework.h"
#include "upe/upe_world_body_api.h"
#include "upe/upe_api.h"

TEST(TestWorldCreateBody) {
    UpeWorldDesc world_desc = {};
    world_desc.fixed_dt = 1.0f / 60.0f;
    world_desc.gravity[1] = -9.81f;
    UpeWorld *world = UpeWorld_Create(&world_desc);
    REQUIRE(world != nullptr);

    UpeBodyDesc body_desc = {};
    body_desc.type = UPE_BODY_DYNAMIC;
    body_desc.position = {0.0f, 10.0f, 0.0f};
    body_desc.rotation = UpeQuat_Identity();
    body_desc.mass = 1.0f;

    uint32_t body_id = UpeWorld_CreateBody(world, &body_desc);
    REQUIRE(body_id != 0xFFFFFFFF);

    UpeBody *body = UpeWorld_GetBody(world, body_id);
    REQUIRE(body != nullptr);
    REQUIRE(body->mass == 1.0f);

    UpeWorld_Destroy(world);
}

TEST(TestWorldCreateDestroyShape) {
    UpeWorldDesc world_desc = {};
    world_desc.fixed_dt = 1.0f / 60.0f;
    world_desc.gravity[1] = -9.81f;
    UpeWorld *world = UpeWorld_Create(&world_desc);
    REQUIRE(world != nullptr);

    UpeBodyDesc body_desc = {};
    body_desc.type = UPE_BODY_STATIC;
    body_desc.position = {0.0f, 0.0f, 0.0f};
    body_desc.rotation = UpeQuat_Identity();
    body_desc.mass = 0.0f;
    uint32_t body_id = UpeWorld_CreateBody(world, &body_desc);
    REQUIRE(body_id != 0xFFFFFFFF);

    UpeShapeDesc shape_desc = {};
    shape_desc.type = UPE_SHAPE_SPHERE;
    shape_desc.body_id = body_id;
    shape_desc.sphere.center = {0.0f, 0.0f, 0.0f};
    shape_desc.sphere.radius = 1.0f;

    uint32_t shape_id = UpeWorld_CreateShape(world, &shape_desc);
    REQUIRE(shape_id != 0xFFFFFFFF);

    UpeWorld_DestroyShape(world, shape_id);

    uint32_t shape_id_2 = UpeWorld_CreateShape(world, &shape_desc);
    REQUIRE(shape_id_2 != 0xFFFFFFFF);
    REQUIRE(shape_id_2 != shape_id);

    UpeWorld_Destroy(world);
}

TEST(TestWorldCollisionPipeline) {
    // Create world
    UpeWorldDesc world_desc = {};
    world_desc.fixed_dt = 1.0f / 60.0f;
    world_desc.gravity[1] = -9.81f;
    UpeWorld *world = UpeWorld_Create(&world_desc);
    REQUIRE(world != nullptr);

    // Create two dynamic bodies
    UpeBodyDesc body_desc_a = {};
    body_desc_a.type = UPE_BODY_DYNAMIC;
    body_desc_a.position = {0.0f, 5.0f, 0.0f};
    body_desc_a.rotation = UpeQuat_Identity();
    body_desc_a.mass = 1.0f;
    uint32_t body_a = UpeWorld_CreateBody(world, &body_desc_a);
    REQUIRE(body_a != 0xFFFFFFFF);

    UpeBodyDesc body_desc_b = {};
    body_desc_b.type = UPE_BODY_DYNAMIC;
    body_desc_b.position = {0.0f, 3.0f, 0.0f};
    body_desc_b.rotation = UpeQuat_Identity();
    body_desc_b.mass = 1.0f;
    uint32_t body_b = UpeWorld_CreateBody(world, &body_desc_b);
    REQUIRE(body_b != 0xFFFFFFFF);

    // Attach sphere shapes to both bodies
    UpeShapeDesc shape_desc_a = {};
    shape_desc_a.type = UPE_SHAPE_SPHERE;
    shape_desc_a.body_id = body_a;
    shape_desc_a.sphere.center = {0.0f, 0.0f, 0.0f};
    shape_desc_a.sphere.radius = 0.5f;
    uint32_t shape_a = UpeWorld_CreateShape(world, &shape_desc_a);
    REQUIRE(shape_a != 0xFFFFFFFF);

    UpeShapeDesc shape_desc_b = {};
    shape_desc_b.type = UPE_SHAPE_SPHERE;
    shape_desc_b.body_id = body_b;
    shape_desc_b.sphere.center = {0.0f, 0.0f, 0.0f};
    shape_desc_b.sphere.radius = 0.5f;
    uint32_t shape_b = UpeWorld_CreateShape(world, &shape_desc_b);
    REQUIRE(shape_b != 0xFFFFFFFF);

    // Get initial positions
    UpeBody *body_a_ptr = UpeWorld_GetBody(world, body_a);
    UpeBody *body_b_ptr = UpeWorld_GetBody(world, body_b);
    REQUIRE(body_a_ptr != nullptr);
    REQUIRE(body_b_ptr != nullptr);
    
    float initial_y_a = body_a_ptr->position.y;
    float initial_y_b = body_b_ptr->position.y;

    // Step simulation (bodies should fall due to gravity and contact)
    for (int i = 0; i < 60; ++i) {
        UpeWorld_Step(world);
    }

    // After 60 frames, bodies should have fallen but not passed through each other
    float final_y_a = body_a_ptr->position.y;
    float final_y_b = body_b_ptr->position.y;
    
    // Bodies should have fallen (y decreased)
    REQUIRE(final_y_a < initial_y_a);
    REQUIRE(final_y_b < initial_y_b);

    // Spheres shouldn't overlap (center distance >= sum of radii)
    float dist_y = final_y_a - final_y_b;
    float min_separation = 0.5f + 0.5f; // sum of radii
    REQUIRE(dist_y >= min_separation * 0.9f); // Allow 10% tolerance for solver settling

    UpeWorld_Destroy(world);
}
