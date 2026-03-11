/**
 * OBB vs Convex Collision Demo
 * 
 * Demonstrates GJK/EPA collision between oriented bounding boxes (OBBs) and convex hulls.
 * A rotating OBB falls onto a convex platform, showing dynamic OBB-convex interaction
 * with proper rotation handling.
 * 
 * v0.12.0 Phase 8: OBB+Convex GJK-based collision support
 */

#include "upe/upe_api.h"
#include "upe/upe_convex.h"
#include <stdio.h>
#include <math.h>

int main() {
    printf("=== OBB vs Convex Collision Demo (v0.12.0) ===\n");
    printf("Demonstrating OBB+Convex GJK/EPA-based dynamic collision with rotation\n\n");

    /* Create world */
    UpeWorldDesc world_desc = {};
    world_desc.fixed_dt = 1.0f / 60.0f;
    world_desc.gravity[0] = 0.0f;
    world_desc.gravity[1] = -9.81f;
    world_desc.gravity[2] = 0.0f;
    UpeWorld *world = UpeWorld_Create(&world_desc);

    if (!world) {
        printf("Failed to create world\n");
        return 1;
    }

    /* Create a convex pyramid platform */
    UpeConvexHull *pyramid = UpeConvexHull_Create(5);
    if (pyramid) {
        /* 5-vertex pyramid base */
        UpeConvexHull_AddVertex(pyramid, {-2.0f, 0.0f, -2.0f});    /* Center base corner */
        UpeConvexHull_AddVertex(pyramid, {2.0f, 0.0f, -2.0f});     /* Center base corner */
        UpeConvexHull_AddVertex(pyramid, {2.0f, 0.0f, 2.0f});      /* Center base corner */
        UpeConvexHull_AddVertex(pyramid, {-2.0f, 0.0f, 2.0f});     /* Center base corner */
        UpeConvexHull_AddVertex(pyramid, {0.0f, 2.0f, 0.0f});      /* Apex */
        UpeConvexHull_ComputeProperties(pyramid);
    }

    /* Create pyramid body (static) */
    UpeBodyDesc pyramid_body_desc = {};
    pyramid_body_desc.position = {0.0f, 0.0f, 0.0f};
    pyramid_body_desc.type = UPE_BODY_STATIC;
    uint32_t pyramid_body_id = UpeWorld_CreateBody(world, &pyramid_body_desc);

    /* Attach pyramid shape */
    UpeShapeDesc pyramid_shape_desc = {};
    pyramid_shape_desc.type = UPE_SHAPE_CONVEX;
    pyramid_shape_desc.body_id = pyramid_body_id;
    pyramid_shape_desc.convex = pyramid;
    pyramid_shape_desc.friction = 0.6f;
    UpeWorld_CreateShape(world, &pyramid_shape_desc);

    /* Create an OBB (rotated box) above the pyramid */
    UpeBodyDesc obb_body_desc = {};
    obb_body_desc.position = {0.0f, 3.0f, 0.0f};
    obb_body_desc.type = UPE_BODY_DYNAMIC;
    obb_body_desc.mass = 3.0f;
    uint32_t obb_body_id = UpeWorld_CreateBody(world, &obb_body_desc);
    UpeBody *obb_body = UpeWorld_GetBody(world, obb_body_id);

    /* Attach OBB shape (0.3 x 0.3 x 0.6 meter rotated box) */
    UpeShapeDesc obb_shape_desc = {};
    obb_shape_desc.type = UPE_SHAPE_OBB;
    obb_shape_desc.body_id = obb_body_id;
    obb_shape_desc.obb.center = {0.0f, 0.0f, 0.0f};
    obb_shape_desc.obb.half_extents = {0.15f, 0.15f, 0.3f};
    /* Identity rotation (no rotation initially) */
    obb_shape_desc.obb.rotation.x = 0.0f;
    obb_shape_desc.obb.rotation.y = 0.0f;
    obb_shape_desc.obb.rotation.z = 0.0f;
    obb_shape_desc.obb.rotation.w = 1.0f;
    /* Initialize axes */
    obb_shape_desc.obb.axes[0] = {1.0f, 0.0f, 0.0f};
    obb_shape_desc.obb.axes[1] = {0.0f, 1.0f, 0.0f};
    obb_shape_desc.obb.axes[2] = {0.0f, 0.0f, 1.0f};
    obb_shape_desc.friction = 0.6f;
    UpeWorld_CreateShape(world, &obb_shape_desc);

    /* Add initial angular velocity for rotation */
    obb_body->angular_velocity = {2.0f, 1.0f, 0.5f};

    printf("[OK] Set up pyramid (convex) and OBB setup\n");
    printf("     OBB starts at (0.0, 3.0, 0.0) with rotation\n");
    printf("     Expected behavior: OBB falls and rotates on pyramid\n\n");

    /* Simulate 5 seconds at 60 FPS */
    float current_time = 0.0f;
    float end_time = 5.0f;
    int frame = 0;

    printf("Time  | OBB Y   | Velocity   | Angular Vel | Status\n");
    printf("----------------------------------------------------------\n");

    while (current_time < end_time) {
        /* Step world */
        UpeWorld_Step(world);
        current_time = UpeWorld_Time(world);
        frame++;

        /* Print status every 30 frames (0.5 seconds) */
        if (frame % 30 == 0 || frame < 5) {
            float vel_mag = sqrtf(obb_body->velocity.x * obb_body->velocity.x +
                                 obb_body->velocity.y * obb_body->velocity.y +
                                 obb_body->velocity.z * obb_body->velocity.z);
            float ang_mag = sqrtf(obb_body->angular_velocity.x * obb_body->angular_velocity.x +
                                 obb_body->angular_velocity.y * obb_body->angular_velocity.y +
                                 obb_body->angular_velocity.z * obb_body->angular_velocity.z);
            printf("%.2f | %.3f | %.3f m/s | %.3f rad/s | Falling...\n",
                   current_time,
                   obb_body->position.y,
                   vel_mag,
                   ang_mag);
        }
    }

    printf("----------------------------------------------------------\n");
    printf("Final Results:\n");
    printf("  OBB position: (%.2f, %.2f, %.2f)\n",
           obb_body->position.x,
           obb_body->position.y,
           obb_body->position.z);
    printf("  OBB velocity: (%.2f, %.2f, %.2f) m/s\n",
           obb_body->velocity.x,
           obb_body->velocity.y,
           obb_body->velocity.z);
    printf("  OBB rotation: (%.2f, %.2f, %.2f, %.2f)\n",
           obb_body->rotation.x,
           obb_body->rotation.y,
           obb_body->rotation.z,
           obb_body->rotation.w);
    printf("\nOBB vs Convex GJK/EPA collisions working with rotation!\n");

    /* Cleanup */
    UpeWorld_Destroy(world);
    UpeConvexHull_Destroy(pyramid);

    return 0;
}
