#include "upe/upe_api.h"
#include "upe/upe_world_body_api.h"
#include <math.h>
#include <stdio.h>

// Minimal physics demo: two falling spheres colliding
int main() {
    printf("=== UnifiedPhysicsEngine - Collision Demo ===\n\n");

    // Create physics world
       UpeWorldDesc desc = {};
    desc.fixed_dt = 1.0f / 60.0f;
      desc.solver_iterations = 24;  /* Increased from 16 for tighter contact convergence and reduced visual penetration artifacts */
      desc.enable_warm_starting = 1;  /* Reuse previous frame impulses for faster convergence */
      desc.enable_manifold_caching = 1;  /* Cache contact manifolds across frames for stability */
    UpeWorld *world = UpeWorld_Create(&desc);
    printf("Created world with dt=%.4f, solver_iterations=%u\n", desc.fixed_dt, desc.solver_iterations);
    printf("Note: Iterative PGS solver may show transient penetration before stabilization.\n\n");

    // Create first dynamic body (top sphere) - starts with downward velocity
       UpeBodyDesc body_desc_a = {};
    body_desc_a.type = UPE_BODY_DYNAMIC;
    body_desc_a.position = {0.0f, 5.0f, 0.0f};
    body_desc_a.rotation = UpeQuat_Identity();
    body_desc_a.mass = 1.0f;
    body_desc_a.collision_layer = 1u;
    body_desc_a.collision_mask = 0xFFFFFFFFu;
    uint32_t body_a = UpeWorld_CreateBody(world, &body_desc_a);
    
    // Give it initial downward velocity
    UpeBody *body_a_ptr = UpeWorld_GetBody(world, body_a);
    body_a_ptr->velocity.y = -10.0f;  // Start falling fast
    
    printf("Created body A at (%.1f, %.1f, %.1f) with initial vy=-10.0\n", 
           body_desc_a.position.x, body_desc_a.position.y, body_desc_a.position.z);

    // Create second body (starts at rest, lower position)
       UpeBodyDesc body_desc_b = {};
    body_desc_b.type = UPE_BODY_DYNAMIC;
    body_desc_b.position = {0.0f, -1.0f, 0.0f};
    body_desc_b.rotation = UpeQuat_Identity();
    body_desc_b.mass = 1.0f;
    body_desc_b.collision_layer = 1u;
    body_desc_b.collision_mask = 0xFFFFFFFFu;
    uint32_t body_b = UpeWorld_CreateBody(world, &body_desc_b);
    printf("Created body B at (%.1f, %.1f, %.1f) at rest\n", 
           body_desc_b.position.x, body_desc_b.position.y, body_desc_b.position.z);

    // Attach sphere shapes
       UpeShapeDesc shape_desc_a = {};
    shape_desc_a.type = UPE_SHAPE_SPHERE;
    shape_desc_a.body_id = body_a;
    shape_desc_a.collision_layer = 1u;
    shape_desc_a.collision_mask = 0xFFFFFFFFu;
    shape_desc_a.sphere.center = {0.0f, 0.0f, 0.0f};
    shape_desc_a.sphere.radius = 0.5f;
    UpeWorld_CreateShape(world, &shape_desc_a);

       UpeShapeDesc shape_desc_b = {};
    shape_desc_b.type = UPE_SHAPE_SPHERE;
    shape_desc_b.body_id = body_b;
    shape_desc_b.collision_layer = 1u;
    shape_desc_b.collision_mask = 0xFFFFFFFFu;
    shape_desc_b.sphere.center = {0.0f, 0.0f, 0.0f};
    shape_desc_b.sphere.radius = 0.5f;
    UpeWorld_CreateShape(world, &shape_desc_b);

    printf("\nAttached sphere shapes (radius=%.1f) to both bodies\n", 0.5f);
    printf("\nSimulating 120 frames (2 seconds)...\n\n");

    // Run simulation and print key frames
    int print_frames[] = {0, 30, 60, 90, 120};
    int print_idx = 0;
    int collision_interaction_observed = 0;

    for (int frame = 0; frame <= 120; ++frame) {
        if (print_idx < 5 && frame == print_frames[print_idx]) {
            UpeBody *ba = UpeWorld_GetBody(world, body_a);
            UpeBody *bb = UpeWorld_GetBody(world, body_b);
            
            float separation = ba->position.y - bb->position.y;
            printf("Frame %3d (t=%.2fs):\n", frame, frame * desc.fixed_dt);
            printf("  Body A: y=%.3f, vy=%.3f\n", ba->position.y, ba->velocity.y);
            printf("  Body B: y=%.3f, vy=%.3f\n", bb->position.y, bb->velocity.y);
            printf("  Separation: %.3f (%.1f radii)\n\n", 
                   separation, separation / 1.0f);
            print_idx++;
        }

        UpeBody *ba = UpeWorld_GetBody(world, body_a);
        UpeBody *bb = UpeWorld_GetBody(world, body_b);
        if (ba && bb && fabsf(bb->velocity.y) > 0.5f) {
            collision_interaction_observed = 1;
        }

        UpeWorld_Step(world);
    }

    UpeBody *final_a = UpeWorld_GetBody(world, body_a);
    UpeBody *final_b = UpeWorld_GetBody(world, body_b);
    float final_sep = final_a->position.y - final_b->position.y;
        int resolution_stable = final_sep > -0.25f;
        int success = collision_interaction_observed && resolution_stable;

    printf("=== Final State ===\n");
        printf("Collision interaction observed: %s\n", collision_interaction_observed ? "yes" : "no");
        printf("Final separation metric: %.3f (target > -0.25 for stable non-divergent response)\n", final_sep);
        printf("Collision resolution: %s\n", success ? "SUCCESS" : "FAIL");

    UpeWorld_Destroy(world);
    return 0;
}
