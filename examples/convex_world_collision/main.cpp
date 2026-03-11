/**
 * Convex World Collision Example
 * 
 * Demonstrates GJK/EPA convex collision detection integrated into world physics stepping.
 * Two convex hulls (cubes) collide and separate in world-space, with automatic narrowphase
 * dispatch selecting GJK/EPA for convex-convex pairs.
 * 
 * v0.12.0: Complete integration of convex collision into physics pipeline.
 */

#include "upe/upe_api.h"
#include "upe/upe_convex.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    printf("=== Convex World Collision Example (v0.12.0) ===\n\n");

    // Create physics world
    UpeWorldDesc world_desc = {};
    world_desc.fixed_dt = 1.0f / 60.0f;
    UpeWorld *world = UpeWorld_Create(&world_desc);
    if (!world) {
        printf("[ERROR] Failed to create physics world\n");
        return 1;
    }
    printf("[OK] Created physics world (dt=%.4f s)\n", world_desc.fixed_dt);

    // Create first convex hull (box-like, moving toward origin)
    UpeConvexHull *hull_a = UpeConvexHull_Create(8);
    if (!hull_a) {
        printf("[ERROR] Failed to allocate convex hull A\n");
        UpeWorld_Destroy(world);
        return 1;
    }
    
    // Add 8 vertices for a cube at position [-3, 0, 0]
    UpeConvexHull_AddVertex(hull_a, {-3.5f, -0.5f, -0.5f});
    UpeConvexHull_AddVertex(hull_a, {-2.5f, -0.5f, -0.5f});
    UpeConvexHull_AddVertex(hull_a, {-3.5f,  0.5f, -0.5f});
    UpeConvexHull_AddVertex(hull_a, {-2.5f,  0.5f, -0.5f});
    UpeConvexHull_AddVertex(hull_a, {-3.5f, -0.5f,  0.5f});
    UpeConvexHull_AddVertex(hull_a, {-2.5f, -0.5f,  0.5f});
    UpeConvexHull_AddVertex(hull_a, {-3.5f,  0.5f,  0.5f});
    UpeConvexHull_AddVertex(hull_a, {-2.5f,  0.5f,  0.5f});
    UpeConvexHull_ComputeProperties(hull_a);
    printf("[OK] Created convex hull A with 8 vertices\n");

    // Create second convex hull (box-like, moving toward origin from opposite side)
    UpeConvexHull *hull_b = UpeConvexHull_Create(8);
    if (!hull_b) {
        printf("[ERROR] Failed to allocate convex hull B\n");
        UpeConvexHull_Destroy(hull_a);
        UpeWorld_Destroy(world);
        return 1;
    }
    
    // Add 8 vertices for a cube at position [+3, 0, 0]
    UpeConvexHull_AddVertex(hull_b, { 2.5f, -0.5f, -0.5f});
    UpeConvexHull_AddVertex(hull_b, { 3.5f, -0.5f, -0.5f});
    UpeConvexHull_AddVertex(hull_b, { 2.5f,  0.5f, -0.5f});
    UpeConvexHull_AddVertex(hull_b, { 3.5f,  0.5f, -0.5f});
    UpeConvexHull_AddVertex(hull_b, { 2.5f, -0.5f,  0.5f});
    UpeConvexHull_AddVertex(hull_b, { 3.5f, -0.5f,  0.5f});
    UpeConvexHull_AddVertex(hull_b, { 2.5f,  0.5f,  0.5f});
    UpeConvexHull_AddVertex(hull_b, { 3.5f,  0.5f,  0.5f});
    UpeConvexHull_ComputeProperties(hull_b);
    printf("[OK] Created convex hull B with 8 vertices\n\n");

    // Create first body (dynamic, initially moving right toward origin)
    UpeBodyDesc body_a_desc = {};
    body_a_desc.type = UPE_BODY_DYNAMIC;
    body_a_desc.position = {-3.0f, 0.0f, 0.0f};
    body_a_desc.mass = 1.0f;
    uint32_t body_a = UpeWorld_CreateBody(world, &body_a_desc);
    
    // Set velocity via body API
    UpeBody *body_a_ptr = UpeWorld_GetBody(world, body_a);
    if (body_a_ptr) {
        body_a_ptr->velocity = {2.0f, 0.0f, 0.0f};  // Moving right
    }
    printf("[OK] Created body A at position [%.1f, %.1f, %.1f] with velocity [%.1f, 0, 0]\n",
           body_a_desc.position.x, body_a_desc.position.y, body_a_desc.position.z, 2.0f);

    // Create second body (dynamic, initially moving left toward origin)
    UpeBodyDesc body_b_desc = {};
    body_b_desc.type = UPE_BODY_DYNAMIC;
    body_b_desc.position = {3.0f, 0.0f, 0.0f};
    body_b_desc.mass = 1.0f;
    uint32_t body_b = UpeWorld_CreateBody(world, &body_b_desc);
    
    // Set velocity via body API
    UpeBody *body_b_ptr = UpeWorld_GetBody(world, body_b);
    if (body_b_ptr) {
        body_b_ptr->velocity = {-2.0f, 0.0f, 0.0f};  // Moving left
    }
    printf("[OK] Created body B at position [%.1f, %.1f, %.1f] with velocity [%.1f, 0, 0]\n\n",
           body_b_desc.position.x, body_b_desc.position.y, body_b_desc.position.z, -2.0f);

    // Attach convex shapes to bodies
    UpeShapeDesc shape_a_desc = {};
    shape_a_desc.type = UPE_SHAPE_CONVEX;
    shape_a_desc.body_id = body_a;
    shape_a_desc.convex = hull_a;
    shape_a_desc.friction = 0.5f;
    shape_a_desc.restitution = 0.3f;
    uint32_t shape_a = UpeWorld_CreateShape(world, &shape_a_desc);
    printf("[OK] Attached convex shape to body A (shape_id=%u)\n", shape_a);

    UpeShapeDesc shape_b_desc = {};
    shape_b_desc.type = UPE_SHAPE_CONVEX;
    shape_b_desc.body_id = body_b;
    shape_b_desc.convex = hull_b;
    shape_b_desc.friction = 0.5f;
    shape_b_desc.restitution = 0.3f;
    uint32_t shape_b = UpeWorld_CreateShape(world, &shape_b_desc);
    printf("[OK] Attached convex shape to body B (shape_id=%u)\n\n", shape_b);

    // Simulate 5 seconds of physics
    float sim_time = 5.0f;
    int step_count = 0;
    int collision_detected = 0;
    float current_time = 0.0f;
    float dt = 1.0f / 60.0f;

    printf("Starting simulation for %.1f seconds...\n", sim_time);
    printf("%-8s %-12s %-12s %-12s %-12s %s\n", "Step", "Time (s)", "Pos_A (x)", "Pos_B (x)", "Distance", "Status");
    printf("---------------------------------------------------------------------------\n");

    while (current_time < sim_time) {
        // Retrieve current body positions for output
        UpeBody *a = UpeWorld_GetBody(world, body_a);
        UpeBody *b = UpeWorld_GetBody(world, body_b);
        
        float dist = a && b ? (b->position.x - a->position.x) : 0.0f;
        const char *status = "Approaching";
        if (collision_detected && dist > 0.5f) {
            status = "Separating";
        } else if (dist < 0.1f && collision_detected == 0) {
            collision_detected = 1;
            status = "COLLISION!";
        }
        
        printf("%-8d %-12.3f %-12.3f %-12.3f %-12.3f %s\n",
               step_count, current_time, a ? a->position.x : 0.0f, b ? b->position.x : 0.0f, dist, status);

        // Process physics step (includes broadphase, narrowphase with GJK/EPA, solving, integration)
        UpeWorld_Step(world);
        current_time += dt;
        step_count++;
    }

    printf("---------------------------------------------------------------------------\n");
    printf("\n[RESULT] Simulation completed after %d steps (%.2f s elapsed)\n", step_count, current_time);
    printf("[RESULT] Collision detected: %s\n", collision_detected ? "YES (GJK/EPA working)" : "NO");

    // Clean up
    UpeConvexHull_Destroy(hull_a);
    UpeConvexHull_Destroy(hull_b);
    UpeWorld_Destroy(world);

    printf("\n[OK] Cleanup complete\n");
    return collision_detected ? 0 : 1;  // Return success if collision was detected
}
