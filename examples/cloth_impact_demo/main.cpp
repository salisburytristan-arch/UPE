/**
 * Cloth Impact and Tearing Demo
 * 
 * Demonstrates:
 * - Cloth collision with moving rigid body (sphere)
 * - Progressive tearing under stress
 * - Impact dynamics and energy transfer
 * - Stretched cloth behavior
 * 
 * v0.13.0 Phase 2: Cloth-rigid body interaction and tearing
 */

#include "upe/upe_api.h"
#include "upe/upe_cloth.h"
#include "upe/upe_body.h"
#include <cstdio>
#include <cmath>

int main() {
    printf("=== Cloth Impact and Tearing Demo (v0.13.0) ===\n");
    printf("Demonstrating cloth-rigid collision and tearing mechanics\n\n");

    // Create physics world
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

    // Create cloth: 20x15 grid, 0.15m spacing
    UpeCloth cloth = {};
    UpeCloth_Init(&cloth, 20, 15, 0.15f);
    
    // Pin top corners and center for hanging configuration
    for (uint32_t x = 0; x < 20; ++x) {
        cloth.vertices[x].pinned = 1;  // Top row
        cloth.vertices[x].position.y += 4.0f;
    }

    // Enable tearing with low threshold for demo
    cloth.tearing_enabled = 1;
    cloth.tear_distance = 0.35f;  // Tears at 2.3x rest length

    printf("[OK] Created cloth: 20x15 grid = %u vertices, %u constraints\n", 
           cloth.vertex_count, cloth.constraint_count);
    printf("     Tear threshold: %.2f m (%.1f%% stretch)\n\n", 
           cloth.tear_distance, (cloth.tear_distance - 0.15f) / 0.15f * 100.0f);

    // Create a falling sphere to impact cloth
    UpeBodyDesc sphere_desc = {};
    sphere_desc.type = UPE_BODY_DYNAMIC;
    sphere_desc.position = {2.0f, 3.5f, 0.0f};
    sphere_desc.mass = 5.0f;
    uint32_t sphere_id = UpeWorld_CreateBody(world, &sphere_desc);
    UpeBody *sphere = UpeWorld_GetBody(world, sphere_id);

    // Attach sphere shape
    UpeShapeDesc sphere_shape_desc = {};
    sphere_shape_desc.type = UPE_SHAPE_SPHERE;
    sphere_shape_desc.body_id = sphere_id;
    sphere_shape_desc.sphere.center = {0.0f, 0.0f, 0.0f};
    sphere_shape_desc.sphere.radius = 0.3f;
    sphere_shape_desc.friction = 0.4f;
    sphere_shape_desc.restitution = 0.6f;
    UpeWorld_CreateShape(world, &sphere_shape_desc);

    // Give sphere initial velocity toward cloth
    sphere->velocity.y = -3.0f;

    printf("[OK] Created sphere (5 kg, r=0.3m) with downward velocity\n");
    printf("     Sphere starts at (2.0, 3.5, 0.0) falling toward cloth\n\n");

    // Simulation state
    float current_time = 0.0f;
    float end_time = 8.0f;
    int frame = 0;
    int initial_constraints = cloth.constraint_count;
    int max_tear_rate = 0;

    printf("Frame | Time  | Constraints | Torn%% | Sphere Y | Velocity | Status\n");
    printf("---------------------------------------------------------------------\n");

    while (current_time < end_time) {
        // Step cloth
        UpeCloth_Step(&cloth, world_desc.fixed_dt, {0.0f, -9.81f, 0.0f});

        // Simple cloth-sphere collision (separate penetration handling)
        for (uint32_t i = 20; i < cloth.vertex_count; ++i) {  // Non-pinned vertices
            UpeVec3 delta = UpeVec3_Sub(cloth.vertices[i].position, sphere->position);
            float dist_sq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
            float collision_dist = 0.3f;  // Sphere radius
            
            if (dist_sq < collision_dist * collision_dist && dist_sq > 1.0e-6f) {
                float dist = sqrtf(dist_sq);
                float overlap = collision_dist - dist;
                
                // Push cloth vertex out
                UpeVec3 normal = UpeVec3_Scale(delta, 1.0f / dist);
                cloth.vertices[i].position = UpeVec3_Add(
                    cloth.vertices[i].position, 
                    UpeVec3_Scale(normal, overlap)
                );
                
                // Transfer some momentum
                cloth.vertices[i].velocity = UpeVec3_Add(
                    cloth.vertices[i].velocity,
                    UpeVec3_Scale(sphere->velocity, 0.3f)
                );
            }
        }

        // Ground collision for sphere
        if (sphere->position.y < 0.3f) {
            sphere->position.y = 0.3f;
            sphere->velocity.y *= -0.7f;  // Bounce with damping
        }

        UpeWorld_Step(world);
        current_time += world_desc.fixed_dt;
        frame++;

        // Track tearing rate
        int current_constraints = cloth.constraint_count;
        int torn_count = initial_constraints - current_constraints;
        int tears_this_frame = (frame > 0) ? torn_count : 0;
        if (tears_this_frame > max_tear_rate) {
            max_tear_rate = tears_this_frame;
        }

        // Print state every 30 frames (~0.5 seconds)
        if (frame % 30 == 0) {
            float torn_percent = (torn_count / (float)initial_constraints) * 100.0f;
            float sphere_speed = sqrtf(
                sphere->velocity.x * sphere->velocity.x +
                sphere->velocity.y * sphere->velocity.y +
                sphere->velocity.z * sphere->velocity.z
            );

            const char *status = "Normal";
            if (torn_percent > 20.0f) status = "Tearing";
            if (torn_percent > 50.0f) status = "Heavy Tear";
            if (sphere->position.y < 0.5f) status = "Bouncing";

            printf("%5d | %.2f  | %11d | %5.1f | %8.3f | %8.3f | %s\n",
                   frame,
                   current_time,
                   current_constraints,
                   torn_percent,
                   sphere->position.y,
                   sphere_speed,
                   status);
        }
    }

    // Final statistics
    printf("\n[FINAL STATISTICS]\n");
    printf("Simulation duration: %.2f seconds\n", end_time);
    printf("Initial constraints: %d\n", initial_constraints);
    printf("Final constraints: %u\n", cloth.constraint_count);
    printf("Constraints torn: %d (%.1f%%)\n", 
           initial_constraints - cloth.constraint_count,
           ((initial_constraints - cloth.constraint_count) / (float)initial_constraints) * 100.0f);
    printf("Max tear rate: %d constraints/frame\n", max_tear_rate);

    // Cloth damage assessment
    float min_rest_dist = 0.15f;
    for (uint32_t i = 0; i < cloth.constraint_count; ++i) {
        UpeVec3 delta = UpeVec3_Sub(
            cloth.vertices[cloth.constraints[i].vertex_b].position,
            cloth.vertices[cloth.constraints[i].vertex_a].position
        );
        float current_len = sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
        float stretch = current_len / cloth.constraints[i].rest_length;
        if (stretch > 1.5f) {
            min_rest_dist = 1.0f;  // Flag critical stretch
        }
    }
    printf("Cloth structural integrity: %s\n", min_rest_dist > 0.5f ? "CRITICAL" : "Damaged");

    // Cleanup
    UpeCloth_Destroy(&cloth);
    UpeWorld_Destroy(world);

    printf("\nCloth impact simulation complete! ✓\n");
    return 0;
}
