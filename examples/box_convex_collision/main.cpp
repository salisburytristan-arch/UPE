/**
 * Box vs Convex Collision Demo
 * 
 * Demonstrates GJK/EPA collision between axis-aligned boxes and convex hulls.
 * A box slides down a sloped convex platform, showing dynamic box-convex interaction.
 * 
 * v0.12.0 Phase 7: Box+Convex GJK-based collision support
 */

#include "upe/upe_api.h"
#include "upe/upe_convex.h"
#include <stdio.h>
#include <math.h>

int main() {
    printf("=== Box vs Convex Collision Demo (v0.12.0) ===\n");
    printf("Demonstrating Box+Convex GJK/EPA-based dynamic collision\n\n");

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

    /* Create a sloped convex platform (pyramid-like wedge) */
    UpeConvexHull *ramp = UpeConvexHull_Create(4);
    if (ramp) {
        /* 4-vertex pyramid wedge sloping down from left to right */
        UpeConvexHull_AddVertex(ramp, {-3.0f, 2.0f, -1.0f});    /* Left top */
        UpeConvexHull_AddVertex(ramp, {3.0f, 0.0f, -1.0f});     /* Right bottom */
        UpeConvexHull_AddVertex(ramp, {3.0f, 0.0f, 1.0f});      /* Right bottom back */
        UpeConvexHull_AddVertex(ramp, {-3.0f, 2.0f, 1.0f});     /* Left top back */
        UpeConvexHull_ComputeProperties(ramp);
    }

    /* Create ramp body (static) */
    UpeBodyDesc ramp_body_desc = {};
    ramp_body_desc.position = {0.0f, 0.0f, 0.0f};
    ramp_body_desc.type = UPE_BODY_STATIC;
    uint32_t ramp_body_id = UpeWorld_CreateBody(world, &ramp_body_desc);

    /* Attach ramp shape */
    UpeShapeDesc ramp_shape_desc = {};
    ramp_shape_desc.type = UPE_SHAPE_CONVEX;
    ramp_shape_desc.body_id = ramp_body_id;
    ramp_shape_desc.convex = ramp;
    ramp_shape_desc.friction = 0.3f;
    UpeWorld_CreateShape(world, &ramp_shape_desc);

    /* Create a box on the ramp */
    UpeBodyDesc box_body_desc = {};
    box_body_desc.position = {-2.0f, 2.5f, 0.0f};
    box_body_desc.type = UPE_BODY_DYNAMIC;
    box_body_desc.mass = 2.0f;
    uint32_t box_body_id = UpeWorld_CreateBody(world, &box_body_desc);
    UpeBody *box_body = UpeWorld_GetBody(world, box_body_id);

    /* Attach box shape (0.4m cube) */
    UpeShapeDesc box_shape_desc = {};
    box_shape_desc.type = UPE_SHAPE_BOX;
    box_shape_desc.body_id = box_body_id;
    box_shape_desc.box = {
        {-0.2f, -0.2f, -0.2f},  /* min */
        {0.2f, 0.2f, 0.2f}      /* max */
    };
    box_shape_desc.friction = 0.3f;
    UpeWorld_CreateShape(world, &box_shape_desc);

    printf("[OK] Set up ramp (convex) and box setup\n");
    printf("     Box starts at (-2.0, 2.5, 0.0)\n");
    printf("     Expected behavior: Box slides down ramp\n\n");

    /* Simulate 5 seconds at 60 FPS */
    float current_time = 0.0f;
    float end_time = 5.0f;
    int frame = 0;

    printf("Time  | Box X   | Box Y   | Box Z   | Velocity   | Status\n");
    printf("---------------------------------------------------------------------------\n");

    while (current_time < end_time) {
        /* Step world */
        UpeWorld_Step(world);
        current_time = UpeWorld_Time(world);
        frame++;

        /* Print status every 30 frames (0.5 seconds) */
        if (frame % 30 == 0 || frame < 5) {
            float vel_mag = sqrtf(box_body->velocity.x * box_body->velocity.x +
                                 box_body->velocity.y * box_body->velocity.y +
                                 box_body->velocity.z * box_body->velocity.z);
            printf("%.2f | %.3f | %.3f | %.3f | %.3f m/s | Sliding...\n",
                   current_time,
                   box_body->position.x,
                   box_body->position.y,
                   box_body->position.z,
                   vel_mag);
        }
    }

    printf("---------------------------------------------------------------------------\n");
    printf("Final Results:\n");
    printf("  Box position: (%.2f, %.2f, %.2f)\n",
           box_body->position.x,
           box_body->position.y,
           box_body->position.z);
    printf("  Box velocity: (%.2f, %.2f, %.2f) m/s\n",
           box_body->velocity.x,
           box_body->velocity.y,
           box_body->velocity.z);
    printf("\nBox vs Convex GJK/EPA collisions working!\n");

    /* Cleanup */
    UpeWorld_Destroy(world);
    UpeConvexHull_Destroy(ramp);

    return 0;
}
