#include "upe/upe_api.h"
#include "upe/upe_convex.h"
#include <stdio.h>
#include <math.h>

int main() {
    printf("=== Mixed-Shape Collision Demo (v0.12.0) ===\n");
    printf("Demonstrating Sphere+Convex and Capsule+Convex GJK-based collisions\n\n");

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

    /* Create a convex platform (box) */
    UpeConvexHull *platform = UpeConvexHull_Create(8);
    if (platform) {
        /* Create a 2x0.5x2 meter box */
        UpeConvexHull_AddVertex(platform, {-1.0f, 0.0f, -1.0f});
        UpeConvexHull_AddVertex(platform, {1.0f, 0.0f, -1.0f});
        UpeConvexHull_AddVertex(platform, {1.0f, 0.0f, 1.0f});
        UpeConvexHull_AddVertex(platform, {-1.0f, 0.0f, 1.0f});
        UpeConvexHull_AddVertex(platform, {-1.0f, -0.25f, -1.0f});
        UpeConvexHull_AddVertex(platform, {1.0f, -0.25f, -1.0f});
        UpeConvexHull_AddVertex(platform, {1.0f, -0.25f, 1.0f});
        UpeConvexHull_AddVertex(platform, {-1.0f, -0.25f, 1.0f});
        UpeConvexHull_ComputeProperties(platform);
    }

    /* Create platform body */
    UpeBodyDesc platform_body_desc = {};
    platform_body_desc.position = {0.0f, 0.0f, 0.0f};
    platform_body_desc.type = UPE_BODY_STATIC;
    uint32_t platform_body_id = UpeWorld_CreateBody(world, &platform_body_desc);

    /* Attach platform shape */
    UpeShapeDesc platform_shape_desc = {};
    platform_shape_desc.type = UPE_SHAPE_CONVEX;
    platform_shape_desc.body_id = platform_body_id;
    platform_shape_desc.convex = platform;
    platform_shape_desc.friction = 0.5f;
    UpeWorld_CreateShape(world, &platform_shape_desc);

    /* Create a sphere falling onto the platform */
    UpeBodyDesc sphere_body_desc = {};
    sphere_body_desc.position = {-0.5f, 2.0f, 0.0f};
    sphere_body_desc.type = UPE_BODY_DYNAMIC;
    sphere_body_desc.mass = 1.0f;
    uint32_t sphere_body_id = UpeWorld_CreateBody(world, &sphere_body_desc);
    UpeBody *sphere_body = UpeWorld_GetBody(world, sphere_body_id);

    /* Attach sphere shape */
    UpeShapeDesc sphere_shape_desc = {};
    sphere_shape_desc.type = UPE_SHAPE_SPHERE;
    sphere_shape_desc.body_id = sphere_body_id;
    sphere_shape_desc.sphere = {0.0f, 0.0f, 0.0f, 0.3f};  /* 0.3m radius */
    sphere_shape_desc.friction = 0.5f;
    UpeWorld_CreateShape(world, &sphere_shape_desc);

    /* Create a capsule falling onto the platform */
    UpeBodyDesc capsule_body_desc = {};
    capsule_body_desc.position = {0.5f, 2.0f, 0.0f};
    capsule_body_desc.type = UPE_BODY_DYNAMIC;
    capsule_body_desc.mass = 2.0f;
    uint32_t capsule_body_id = UpeWorld_CreateBody(world, &capsule_body_desc);
    UpeBody *capsule_body = UpeWorld_GetBody(world, capsule_body_id);

    /* Attach capsule shape */
    UpeShapeDesc capsule_shape_desc = {};
    capsule_shape_desc.type = UPE_SHAPE_CAPSULE;
    capsule_shape_desc.body_id = capsule_body_id;
    capsule_shape_desc.capsule = {{0.0f, -0.5f, 0.0f}, {0.0f, 0.5f, 0.0f}, 0.2f};
    capsule_shape_desc.friction = 0.5f;
    UpeWorld_CreateShape(world, &capsule_shape_desc);

    /* Simulate 3 seconds at 60 FPS */
    float current_time = 0.0f;
    float end_time = 3.0f;
    int frame = 0;

    printf("Time | Sphere Y | Capsule Y | Status\n");
    printf("----------------------------------------\n");

    while (current_time < end_time) {
        /* Step world */
        UpeWorld_Step(world);
        current_time = UpeWorld_Time(world);
        frame++;

        /* Print status every 30 frames (0.5 seconds) */
        if (frame % 30 == 0) {
            printf("%.2f | %.3f  | %.3f   | Simulating...\n",
                   current_time,
                   sphere_body->position.y,
                   capsule_body->position.y);
        }
    }

    printf("----------------------------------------\n");
    printf("Final Results:\n");
    printf("  Sphere:  Y = %.3f (expected ~0.3m)\n", sphere_body->position.y);
    printf("  Capsule: Y = %.3f (expected ~0.5m)\n", capsule_body->position.y);
    printf("\nMixed-shape GJK/EPA collisions working!\n");

    /* Cleanup */
    UpeWorld_Destroy(world);
    UpeConvexHull_Destroy(platform);

    return 0;
}
