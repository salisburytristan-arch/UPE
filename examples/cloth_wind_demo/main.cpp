/**
 * Cloth Simulation with Wind Demo
 * 
 * Demonstrates cloth particle simulation with:
 * - Distance, shear, and bending constraints
 * - Self-collision detection
 * - Wind aerodynamic forces
 * - Tearing mechanics under stress
 * - Integration with world physics
 * 
 * v0.13.0 Phase 1: Cloth simulation and wind interaction
 */

#include "upe/upe_api.h"
#include "upe/upe_cloth.h"
#include "upe/upe_convex.h"
#include <cstdio>
#include <cmath>

int main() {
    printf("=== Cloth Simulation with Wind Demo (v0.13.0) ===\n");
    printf("Demonstrating cloth particle dynamics with aerodynamics\n\n");

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

    // Create cloth: 15x10 grid, 0.2m spacing
    UpeCloth cloth = {};
    UpeCloth_Init(&cloth, 15, 10, 0.2f);
    
    printf("[OK] Created cloth: 15x10 grid = %u vertices, %u constraints\n", 
           cloth.vertex_count, cloth.constraint_count);

    // Set up pinned vertices (top two rows are pinned)
    for (uint32_t i = 0; i < 30; ++i) {
        cloth.vertices[i].pinned = 1;
    }

    // Position cloth above origin
    for (uint32_t i = 0; i < cloth.vertex_count; ++i) {
        cloth.vertices[i].position.y += 5.0f;
    }

    // Create a rigid platform (convex) for cloth to interact with
    UpeConvexHull *platform = UpeConvexHull_Create(4);
    if (platform) {
        UpeConvexHull_AddVertex(platform, {-3.0f, 0.0f, -3.0f});
        UpeConvexHull_AddVertex(platform, { 3.0f, 0.0f, -3.0f});
        UpeConvexHull_AddVertex(platform, { 3.0f, 0.0f,  3.0f});
        UpeConvexHull_AddVertex(platform, {-3.0f, 0.0f,  3.0f});
        UpeConvexHull_ComputeProperties(platform);
    }

    // Create platform body (static)
    UpeBodyDesc platform_desc = {};
    platform_desc.type = UPE_BODY_STATIC;
    platform_desc.position = {0.0f, 0.0f, 0.0f};
    uint32_t platform_id = UpeWorld_CreateBody(world, &platform_desc);

    // Attach platform shape
    UpeShapeDesc platform_shape_desc = {};
    platform_shape_desc.type = UPE_SHAPE_CONVEX;
    platform_shape_desc.body_id = platform_id;
    platform_shape_desc.convex = platform;
    UpeWorld_CreateShape(world, &platform_shape_desc);

    printf("[OK] Created platform and pinned cloth vertices\n");
    printf("     Cloth top edge pinned, falling under gravity\n\n");

    // Simulation state
    float current_time = 0.0f;
    float end_time = 10.0f;
    int frame = 0;
    
    // Wind parameters
    float wind_time = 0.0f;
    float wind_magnitude = 0.0f;

    printf("Frame | Time  | Cloth Y Min | Wind Mag | Constraints | Status\n");
    printf("---------------------------------------------------------------\n");

    while (current_time < end_time) {
        // Apply time-varying wind (increases then stabilizes)
        wind_time += world_desc.fixed_dt;
        if (wind_time < 3.0f) {
            wind_magnitude = (wind_time / 3.0f) * 5.0f;  // Ramp up to 5 m/s
        } else {
            wind_magnitude = 5.0f;
        }
        
        // Wind direction: swaying left-right with time
        cloth.wind_velocity.x = wind_magnitude * sinf(wind_time * 0.5f);
        cloth.wind_velocity.z = wind_magnitude * 0.3f * cosf(wind_time * 0.5f);

        // Step cloth simulation
        UpeCloth_Step(&cloth, world_desc.fixed_dt, {0.0f, -9.81f, 0.0f});

        // Basic cloth-platform collision (simple AABB-based for demo)
        for (uint32_t i = 30; i < cloth.vertex_count; ++i) {  // Non-pinned vertices
            if (cloth.vertices[i].position.y < 0.5f) {
                cloth.vertices[i].position.y = 0.5f;
                cloth.vertices[i].velocity.y = 0.0f;  // Damping
            }
        }

        UpeWorld_Step(world);
        current_time += world_desc.fixed_dt;
        frame++;

        // Print state every 60 frames (~1 second)
        if (frame % 60 == 0) {
            float min_y = cloth.vertices[cloth.vertex_count - 1].position.y;
            for (uint32_t i = 30; i < cloth.vertex_count; ++i) {
                if (cloth.vertices[i].position.y < min_y) {
                    min_y = cloth.vertices[i].position.y;
                }
            }

            int tearing = cloth.constraint_count < 600;
            printf("%5d | %.2f  | %11.3f | %8.3f | %11u | %s\n",
                   frame,
                   current_time,
                   min_y,
                   wind_magnitude,
                   cloth.constraint_count,
                   tearing ? "TEARING" : "Stable");
        }
    }

    // Final statistics
    printf("\n[FINAL STATISTICS]\n");
    printf("Total simulation time: %.2f seconds\n", current_time);
    printf("Final constraint count: %u (out of ~800 original)\n", cloth.constraint_count);
    printf("Cloth vertices: %u\n", cloth.vertex_count);

    // Calculate cloth extent
    float min_y = cloth.vertices[0].position.y;
    float max_y = cloth.vertices[0].position.y;
    for (uint32_t i = 0; i < cloth.vertex_count; ++i) {
        if (cloth.vertices[i].position.y < min_y) min_y = cloth.vertices[i].position.y;
        if (cloth.vertices[i].position.y > max_y) max_y = cloth.vertices[i].position.y;
    }
    printf("Cloth Y extent: %.3f to %.3f (range: %.3f m)\n", min_y, max_y, max_y - min_y);

    // Cleanup
    UpeCloth_Destroy(&cloth);
    UpeConvexHull_Destroy(platform);
    UpeWorld_Destroy(world);

    printf("\nCloth simulation complete! ✓\n");
    return 0;
}
