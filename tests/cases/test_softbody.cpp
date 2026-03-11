#include "../test_framework.h"
#include "upe/upe_softbody.h"
#include "upe/upe_math.h"
#include <cmath>

// Test soft body creation and destruction
TEST(SoftBodyCreateDestroy) {
    // Create a simple cloth: 2x2 grid of vertices
    UpeSoftBodyVertex vertices[4] = {
        {{0.0f, 2.0f, 0.0f}, {0.0f, 2.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 1.0f, 1.0f, 0},
        {{1.0f, 2.0f, 0.0f}, {1.0f, 2.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 1.0f, 1.0f, 0},
        {{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 1.0f, 1.0f, 0},
        {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 1.0f, 1.0f, 0},
    };

    // Structural constraints (edges)
    UpeSoftBodyConstraint constraints[5] = {
        {0, 1, 1.0f, 0.0f, 0},  // Top edge
        {2, 3, 1.0f, 0.0f, 0},  // Bottom edge
        {0, 2, 1.0f, 0.0f, 0},  // Left edge
        {1, 3, 1.0f, 0.0f, 0},  // Right edge
        {0, 3, 1.414f, 0.0f, 0}, // Diagonal
    };

    UpeSoftBodyDesc desc = {
        vertices,
        4,
        constraints,
        5,
        {0.0f, -9.8f, 0.0f},
        0.99f,
        0.0f,
        {0.0f, 0.0f, 0.0f},
    };

    UpeSoftBody *body = UpeSoftBody_Create(&desc);
    REQUIRE(body != nullptr);
    REQUIRE(body->vertex_count == 4);
    REQUIRE(body->constraint_count == 5);

    UpeSoftBody_Destroy(body);
}

// Test vertex pinning
TEST(SoftBodyPinVertex) {
    UpeSoftBodyVertex vertices[2] = {
        {{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 1.0f, 1.0f, 0},
        {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 1.0f, 1.0f, 0},
    };

    UpeSoftBodyConstraint constraint = {0, 1, 1.0f, 0.0f, 0};

    UpeSoftBodyDesc desc = {
        vertices,
        2,
        &constraint,
        1,
        {0.0f, 0.0f, 0.0f},
        0.99f,
        0.0f,
        {0.0f, 0.0f, 0.0f},
    };

    UpeSoftBody *body = UpeSoftBody_Create(&desc);

    // Pin first vertex
    UpeVec3 pin_pos = {5.0f, 5.0f, 5.0f};
    UpeSoftBody_PinVertex(body, 0, pin_pos);

    REQUIRE(body->vertices[0].inv_mass == 0.0f);  // Pinned
    REQUIRE(pin_pos.x == body->vertices[0].position.x);
    REQUIRE(pin_pos.y == body->vertices[0].position.y);
    REQUIRE(pin_pos.z == body->vertices[0].position.z);

    // Unpin and check it's movable again
    UpeSoftBody_UnpinVertex(body, 0);
    REQUIRE(body->vertices[0].inv_mass > 0.0f);

    UpeSoftBody_Destroy(body);
}

// Test single step with gravity
TEST(SoftBodyGravityStep) {
    UpeSoftBodyVertex vertex = {
        {0.0f, 1.0f, 0.0f},     // position
        {0.0f, 1.0f, 0.0f},     // previous_position
        {0.0f, 0.0f, 0.0f},     // velocity
        1.0f,                    // mass
        1.0f,                    // inv_mass
        0,                       // pinned
    };

    UpeSoftBodyDesc desc = {
        &vertex,
        1,
        nullptr,
        0,
        {0.0f, -10.0f, 0.0f},
        1.0f,  // No damping for simple test
        0.0f,
        {0.0f, 0.0f, 0.0f},
    };

    UpeSoftBody *body = UpeSoftBody_Create(&desc);

    float dt = 0.1f;
    UpeSoftBody_Step(body, dt, 0);  // 0 constraint iterations

    // After 1 step with gravity, position should drop
    // p_new = p + 0 + a*dt² = 1.0 + (-10)*0.01 = 0.9
    float expected_y = 1.0f - 10.0f * dt * dt;
    float actual_y = body->vertices[0].position.y;
    float diff = expected_y > actual_y ? expected_y - actual_y : actual_y - expected_y;
    REQUIRE(diff < 0.001f);

    UpeSoftBody_Destroy(body);
}

// Test constraint solving
TEST(SoftBodyConstraintSolve) {
    UpeSoftBodyVertex vertices[2] = {
        {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 1.0f, 1.0f, 0},
        {{2.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 1.0f, 1.0f, 0},
    };

    // Constraint: distance = 1.0 (currently 2.0, will compress)
    UpeSoftBodyConstraint constraint = {0, 1, 1.0f, 0.0f, 0};

    UpeSoftBodyDesc desc = {
        vertices,
        2,
        &constraint,
        1,
        {0.0f, 0.0f, 0.0f},
        1.0f,
        0.0f,
        {0.0f, 0.0f, 0.0f},
    };

    UpeSoftBody *body = UpeSoftBody_Create(&desc);

    // Step and solve
    float dt = 0.01f;
    UpeSoftBody_Step(body, dt, 10);  // 10 constraint iterations

    // After solving, vertices should move closer to satisfy constraint
    float dist = UpeVec3_Length(UpeVec3_Sub(body->vertices[1].position, body->vertices[0].position));
    float diff = dist > 1.0f ? dist - 1.0f : 1.0f - dist;
    REQUIRE(diff < 0.1f);  // Should be close to rest length

    UpeSoftBody_Destroy(body);
}

// Test get vertex position
TEST(SoftBodyGetPosition) {
    UpeSoftBodyVertex vertex = {
        {3.5f, 4.2f, 1.1f},
        {3.5f, 4.2f, 1.1f},
        {0.0f, 0.0f, 0.0f},
        1.0f,
        1.0f,
        0,
    };

    UpeSoftBodyDesc desc = {
        &vertex,
        1,
        nullptr,
        0,
        {0.0f, 0.0f, 0.0f},
        1.0f,
        0.0f,
        {0.0f, 0.0f, 0.0f},
    };

    UpeSoftBody *body = UpeSoftBody_Create(&desc);

    UpeVec3 pos = UpeSoftBody_GetVertexPosition(body, 0);
    float dx = pos.x > 3.5f ? pos.x - 3.5f : 3.5f - pos.x;
    float dy = pos.y > 4.2f ? pos.y - 4.2f : 4.2f - pos.y;
    float dz = pos.z > 1.1f ? pos.z - 1.1f : 1.1f - pos.z;
    REQUIRE(dx < 0.001f);
    REQUIRE(dy < 0.001f);
    REQUIRE(dz < 0.001f);

    // Invalid index should return zero
    UpeVec3 invalid = UpeSoftBody_GetVertexPosition(body, 999);
    REQUIRE(invalid.x == 0.0f);
    REQUIRE(invalid.y == 0.0f);
    REQUIRE(invalid.z == 0.0f);

    UpeSoftBody_Destroy(body);
}

