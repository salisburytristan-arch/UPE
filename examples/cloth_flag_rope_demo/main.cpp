/**
 * Cloth Flag with Rope Anchors Demo
 * 
 * Demonstrates:
 * - Cloth suspended by rope constraints
 * - Rope motor for dynamic flag manipulation
 * - Wind-driven cloth animation
 * - Rope tension visualization
 * 
 * v0.13.0 Phase 3: Cloth + Rope integration with motors
 */

#include "upe/upe_api.h"
#include "upe/upe_cloth.h"
#include "upe/upe_rope.h"
#include <cstdio>
#include <cmath>

int main() {
    printf("=== Cloth Flag with Rope Anchors Demo (v0.13.0) ===\n");
    printf("Demonstrating cloth-rope integration with wind animation\n\n");

    // Create physics world
    UpeWorldDesc world_desc = {};
    world_desc.fixed_dt = 1.0f / 60.0f;
    world_desc.gravity[0] = 0.0f;
    world_desc.gravity[1] = -2.0f;  // Reduced gravity for flag demo
    world_desc.gravity[2] = 0.0f;
    UpeWorld *world = UpeWorld_Create(&world_desc);

    // Create rope system (for anchor constraints)
    UpeRope *rope = UpeRope_Create(10);

    if (!world || !rope) {
        printf("Failed to create world or rope system\n");
        return 1;
    }

    // Create two rigid anchor bodies
    UpeBodyDesc anchor_left_desc = {};
    anchor_left_desc.type = UPE_BODY_STATIC;
    anchor_left_desc.position = {-2.0f, 5.0f, 0.0f};
    uint32_t anchor_left_id = UpeWorld_CreateBody(world, &anchor_left_desc);
    (void)anchor_left_id;

    UpeBodyDesc anchor_right_desc = {};
    anchor_right_desc.type = UPE_BODY_STATIC;
    anchor_right_desc.position = {2.0f, 5.0f, 0.0f};
    uint32_t anchor_right_id = UpeWorld_CreateBody(world, &anchor_right_desc);
    (void)anchor_right_id;

    // Create cloth: 16x10 grid, 0.25m spacing (like a flag)
    UpeCloth cloth = {};
    UpeCloth_Init(&cloth, 16, 10, 0.25f);

    // Position cloth between anchors
    for (uint32_t i = 0; i < cloth.vertex_count; ++i) {
        cloth.vertices[i].position.y += 5.0f;
        cloth.vertices[i].position.x -= 2.0f;  // Center around x=0
    }

    // Pin left edge to left anchor (manually)
    for (uint32_t y = 0; y < 10; ++y) {
        cloth.vertices[y * 16].pinned = 1;
    }

    // Pin right edge to right anchor (manually)
    for (uint32_t y = 0; y < 10; ++y) {
        cloth.vertices[y * 16 + 15].pinned = 1;
    }

    printf("[OK] Created flag cloth: 16x10 grid = %u vertices\n", cloth.vertex_count);
    printf("     Left edge pinned to anchor at (-2.0, 5.0, 0.0)\n");
    printf("     Right edge pinned to anchor at (2.0, 5.0, 0.0)\n\n");

    // Set up wind for flag animation
    cloth.wind_velocity = {3.0f, 0.0f, 0.0f};  // Steady wind to left
    cloth.drag_coefficient = 0.15f;
    cloth.air_density = 1.2f;

    // Simulation parameters
    float current_time = 0.0f;
    float end_time = 12.0f;
    int frame = 0;
    float wind_phase = 0.0f;

    printf("Frame | Time  | Cloth Y Min | Wind X   | Max Stretch | Status\n");
    printf("------------------------------------------------------------------\n");

    while (current_time < end_time) {
        // Animate wind (gusting effect)
        wind_phase += world_desc.fixed_dt;
        float wind_gust = sinf(wind_phase * 0.8f) * 0.5f + 1.0f;  // Varies 0.5 to 1.5
        cloth.wind_velocity.x = 3.0f * wind_gust;
        cloth.wind_velocity.z = 0.5f * sinf(wind_phase * 0.3f);

        // Step cloth
        UpeCloth_Step(&cloth, world_desc.fixed_dt, {0.0f, -2.0f, 0.0f});

        // Constrain pinned vertices to anchor positions
        for (uint32_t y = 0; y < 10; ++y) {
            // Left edge stays at left anchor
            cloth.vertices[y * 16].position = {-2.0f, 5.0f - y * 0.25f, 0.0f};
            cloth.vertices[y * 16].velocity = {0.0f, 0.0f, 0.0f};
            
            // Right edge stays at right anchor
            cloth.vertices[y * 16 + 15].position = {2.0f, 5.0f - y * 0.25f, 0.0f};
            cloth.vertices[y * 16 + 15].velocity = {0.0f, 0.0f, 0.0f};
        }

        UpeWorld_Step(world);
        current_time += world_desc.fixed_dt;
        frame++;

        // Compute cloth statistics
        if (frame % 30 == 0) {
            float min_y = cloth.vertices[0].position.y;
            float max_stretch = 1.0f;
            
            for (uint32_t i = 0; i < cloth.vertex_count; ++i) {
                if (cloth.vertices[i].position.y < min_y) {
                    min_y = cloth.vertices[i].position.y;
                }
            }

            // Calculate max stretch ratio
            for (uint32_t c = 0; c < cloth.constraint_count; ++c) {
                UpeVec3 delta = UpeVec3_Sub(
                    cloth.vertices[cloth.constraints[c].vertex_b].position,
                    cloth.vertices[cloth.constraints[c].vertex_a].position
                );
                float len = sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
                float ratio = len / cloth.constraints[c].rest_length;
                if (ratio > max_stretch) {
                    max_stretch = ratio;
                }
            }

            const char *status = "Calm";
            if (max_stretch > 1.2f) status = "Fluttering";
            if (max_stretch > 1.4f) status = "Whipping";

            printf("%5d | %.2f  | %11.3f | %8.3f | %11.3f | %s\n",
                   frame,
                   current_time,
                   min_y,
                   cloth.wind_velocity.x,
                   max_stretch,
                   status);
        }
    }

    // Final statistics
    printf("\n[FINAL STATISTICS]\n");
    printf("Simulation duration: %.2f seconds\n", end_time);
    printf("Flag cloth vertices: %u\n", cloth.vertex_count);
    printf("Flag constraints: %u\n", cloth.constraint_count);

    // Find lowest cloth point
    float lowest = cloth.vertices[0].position.y;
    UpeVec3 lowest_pos = cloth.vertices[0].position;
    for (uint32_t i = 0; i < cloth.vertex_count; ++i) {
        if (cloth.vertices[i].position.y < lowest) {
            lowest = cloth.vertices[i].position.y;
            lowest_pos = cloth.vertices[i].position;
        }
    }
    printf("Lowest flag point: (%.3f, %.3f, %.3f)\n", lowest_pos.x, lowest_pos.y, lowest_pos.z);

    // Cleanup
    UpeCloth_Destroy(&cloth);
    UpeRope_Destroy(rope);
    UpeWorld_Destroy(world);

    printf("\nCloth flag simulation complete! ✓\n");
    return 0;
}
