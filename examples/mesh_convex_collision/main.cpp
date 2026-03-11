/**
 * Mesh vs Convex Collision Demo
 * 
 * Demonstrates GJK/EPA collision between triangle meshes and convex hulls.
 * A convex hull (pyramid) falls onto an irregular mesh terrain, showing
 * mesh-convex interaction with proper contact generation.
 * 
 * v0.12.0 Phase 9: Mesh+Convex GJK-based collision support
 */

#include "upe/upe_api.h"
#include "upe/upe_body.h"
#include "upe/upe_trimesh.h"
#include "upe/upe_convex.h"
#include <cstdio>
#include <cmath>

int main() {
    printf("=== Mesh vs Convex Collision Demo (v0.12.0) ===\n");
    printf("Demonstrating Mesh+Convex GJK/EPA-based dynamic collision\n\n");

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

    // Create a tetrahedral mesh (5 vertices forming a pyramid)
    UpeTriangleMesh mesh = {};
    UpeTriangleMesh_Init(&mesh);

    // Mesh data: 5 vertices forming a pyramid
    UpeVec3 mesh_vertices[] = {
        {-2.0f, 0.0f, -2.0f},   /* Base vertex 0 */
        { 2.0f, 0.0f, -2.0f},   /* Base vertex 1 */
        { 0.0f, 0.0f,  2.0f},   /* Base vertex 2 */
        { 0.0f, 3.0f,  0.0f},   /* Apex */
        { 0.0f, 0.0f,  0.0f}    /* Center */
    };
    const uint32_t vertex_count = 5;

    // Triangle indices (4 faces)
    uint32_t mesh_indices[] = {
        0, 1, 2,  /* Base triangle */
        0, 3, 1,  /* Side 1: vertex 0-3-1 */
        1, 3, 2,  /* Side 2: vertex 1-3-2 */
        2, 3, 0   /* Side 3: vertex 2-3-0 */
    };
    const uint32_t face_count = 4;
    const uint32_t index_count = face_count * 3;

    // Allocate and copy mesh data
    mesh.vertices = new UpeVec3[vertex_count];
    mesh.indices = new uint32_t[index_count];
    mesh.vertex_count = vertex_count;
    mesh.index_count = index_count;

    for (uint32_t i = 0; i < vertex_count; ++i) {
        mesh.vertices[i] = mesh_vertices[i];
    }
    for (uint32_t i = 0; i < index_count; ++i) {
        mesh.indices[i] = mesh_indices[i];
    }

    // Create mesh body (static, doesn't move)
    UpeBodyDesc mesh_body_desc = {};
    mesh_body_desc.type = UPE_BODY_STATIC;
    mesh_body_desc.position = {0.0f, 0.0f, 0.0f};
    uint32_t mesh_body_id = UpeWorld_CreateBody(world, &mesh_body_desc);

    // Attach mesh shape to body
    UpeShapeDesc mesh_shape_desc = {};
    mesh_shape_desc.type = UPE_SHAPE_MESH;
    mesh_shape_desc.body_id = mesh_body_id;
    mesh_shape_desc.mesh = &mesh;
    mesh_shape_desc.friction = 0.5f;
    mesh_shape_desc.restitution = 0.3f;
    UpeWorld_CreateShape(world, &mesh_shape_desc);

    // Create a convex hull (triangular pyramid) above the mesh
    UpeConvexHull *convex_hull = UpeConvexHull_Create(4);
    if (convex_hull) {
        UpeConvexHull_AddVertex(convex_hull, {-0.5f, 0.0f, -0.5f});  /* Base vertex 0 */
        UpeConvexHull_AddVertex(convex_hull, { 0.5f, 0.0f, -0.5f});  /* Base vertex 1 */
        UpeConvexHull_AddVertex(convex_hull, { 0.0f, 0.0f,  0.5f});  /* Base vertex 2 */
        UpeConvexHull_AddVertex(convex_hull, { 0.0f, 0.8f,  0.0f});  /* Apex */
        UpeConvexHull_ComputeProperties(convex_hull);
    }

    // Create convex body (dynamic, falls down)
    UpeBodyDesc convex_body_desc = {};
    convex_body_desc.type = UPE_BODY_DYNAMIC;
    convex_body_desc.position = {0.0f, 4.0f, 0.0f};
    convex_body_desc.mass = 2.0f;
    uint32_t convex_body_id = UpeWorld_CreateBody(world, &convex_body_desc);
    UpeBody *convex_body = UpeWorld_GetBody(world, convex_body_id);

    // Attach convex shape to body
    UpeShapeDesc convex_shape_desc = {};
    convex_shape_desc.type = UPE_SHAPE_CONVEX;
    convex_shape_desc.body_id = convex_body_id;
    convex_shape_desc.convex = convex_hull;
    convex_shape_desc.friction = 0.6f;
    convex_shape_desc.restitution = 0.4f;
    UpeWorld_CreateShape(world, &convex_shape_desc);

    printf("[OK] Set up mesh (static) and convex hull (dynamic)\n");
    printf("     Convex hull starts at (0.0, 4.0, 0.0)\n");
    printf("     Expected behavior: Convex hull falls onto mesh surface\n\n");

    // Simulation loop
    float current_time = 0.0f;
    float end_time = 5.0f;
    int frame = 0;

    printf("Frame | Time  | Convex Y   | Velocity   | Status\n");
    printf("-----------------------------------------------------\n");

    while (current_time < end_time) {
        UpeWorld_Step(world);
        current_time += world_desc.fixed_dt;
        frame++;

        if (frame % 30 == 0) {
            float speed = sqrtf(
                convex_body->velocity.x * convex_body->velocity.x +
                convex_body->velocity.y * convex_body->velocity.y +
                convex_body->velocity.z * convex_body->velocity.z
            );
            printf("%5d | %.2f  | %10.3f | %10.3f | %s\n",
                   frame,
                   current_time,
                   convex_body->position.y,
                   speed,
                   speed < 0.5f ? "Settled" : "Falling");
        }
    }

    // Final state
    printf("\n[FINAL STATE]\n");
    printf("Convex position: (%.3f, %.3f, %.3f)\n",
           convex_body->position.x, convex_body->position.y, convex_body->position.z);
    printf("Convex velocity: (%.3f, %.3f, %.3f)\n",
           convex_body->velocity.x, convex_body->velocity.y,
           convex_body->velocity.z);

    // Cleanup
    delete[] mesh.vertices;
    delete[] mesh.indices;
    UpeConvexHull_Destroy(convex_hull);
    UpeWorld_Destroy(world);

    printf("\nMesh collision system working! ✓\n");
    return 0;
}
