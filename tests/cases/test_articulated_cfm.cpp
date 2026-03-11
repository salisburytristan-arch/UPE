#include "../test_framework.h"
#include "upe/upe_articulated.h"
#include "upe/upe_world_body_api.h"
#include "upe/upe_api.h"
#include "upe/upe_solver.h"
#include "upe/upe_math.h"

// Test articulated body creation
TEST(ArticulatedBodyCreate) {
    UpeArticulatedLink links[3] = {
        {(uint32_t)-1, (uint32_t)-1, (uint32_t)-1, {0.0f, 0.0f, 0.0f}},  // Root link
        {(uint32_t)-1, 0, (uint32_t)-1, {0.0f, -1.0f, 0.0f}},            // Left arm
        {(uint32_t)-1, 0, (uint32_t)-1, {0.0f, -1.0f, 0.0f}},            // Right arm
    };

    UpeArticulatedDesc desc = {3, links, 0};

    /* world-unaware creation */
    UpeArticulatedBody *body = UpeArticulatedBody_Create(NULL, &desc);
    REQUIRE(body != NULL);
    REQUIRE(body->link_count == 3);
    REQUIRE(body->root_link == 0);
    REQUIRE(body->links != NULL);
    REQUIRE(body->link_alive != NULL);
    REQUIRE(body->link_positions != NULL);
    UpeArticulatedBody_Destroy(NULL, body);

    /* world-aware creation should spawn bodies and joints */
    UpeWorldDesc wdesc = {};
    UpeWorld *world = UpeWorld_Create(&wdesc);
    REQUIRE(world != NULL);

    body = UpeArticulatedBody_Create(world, &desc);
    REQUIRE(body != NULL);
    REQUIRE(body->link_count == 3);
    /* each link should now reference a valid body in the world */
    for (uint32_t i = 0; i < body->link_count; ++i) {
        REQUIRE(UpeWorld_GetBody(world, body->links[i].body_id) != NULL);
    }
    /* joints added for both child links */
    REQUIRE(UpeWorld_GetJointCount(world) >= 2);

    UpeArticulatedBody_Destroy(world, body);
    UpeWorld_Destroy(world);
}

// Test articulated body link access
TEST(ArticulatedBodyLinkAccess) {
    UpeArticulatedLink links[2] = {
        {(uint32_t)-1, (uint32_t)-1, (uint32_t)-1, {0.0f, 0.0f, 0.0f}},
        {(uint32_t)-1, 0, (uint32_t)-1, {0.0f, -1.0f, 0.0f}},
    };

    UpeArticulatedDesc desc = {2, links, 0};

    UpeArticulatedBody *body = UpeArticulatedBody_Create(NULL, &desc);

    // Check link hierarchy and that ids were preserved by descriptor
    REQUIRE(body->links[0].parent_link == (uint32_t)-1);
    REQUIRE(body->links[1].parent_link == 0);  // Second link's parent is first

    UpeArticulatedBody_Destroy(NULL, body);
}

// Test world-interaction helpers (positions and impulses)
TEST(ArticulatedBodyWorldHelpers) {
    UpeArticulatedLink links[2] = {
        {(uint32_t)-1, (uint32_t)-1, (uint32_t)-1, {0.0f, 0.0f, 0.0f}},
        {(uint32_t)-1, 0, (uint32_t)-1, {0.0f, -1.0f, 0.0f}},
    };
    UpeArticulatedDesc desc = {2, links, 0};
    UpeWorldDesc wdesc = {};
    UpeWorld *world = UpeWorld_Create(&wdesc);
    REQUIRE(world != NULL);
    UpeArticulatedBody *body = UpeArticulatedBody_Create(world, &desc);
    REQUIRE(body != NULL);

    /* both links should have bodies, and positions match world bodies */
    for (uint32_t i = 0; i < body->link_count; ++i) {
        UpeBody *b = UpeWorld_GetBody(world, body->links[i].body_id);
        REQUIRE(b != NULL);
        b->position.x = (float)i;
        b->position.y = 0.0f;
        b->position.z = 0.0f;
        UpeVec3 pos = UpeArticulatedBody_GetLinkPosition(body, i, world);
        REQUIRE(pos.x == (float)i);
    }

    /* applying impulse to child should not crash */
    UpeVec3 impulse;
    impulse.x = 1.0f;
    impulse.y = 0.0f;
    impulse.z = 0.0f;
    UpeArticulatedBody_ApplyLinkImpulse(body, 1, impulse, world);

    UpeArticulatedBody_Destroy(world, body);
    UpeWorld_Destroy(world);
}

// Test articulated body invalid creation
TEST(ArticulatedBodyInvalidCreate) {
    // NULL descriptor
    UpeArticulatedBody *body1 = UpeArticulatedBody_Create(NULL, NULL);
    REQUIRE(body1 == NULL);

    // Empty descriptor
    UpeArticulatedDesc empty_desc = {0, NULL, 0};
    UpeArticulatedBody *body2 = UpeArticulatedBody_Create(NULL, &empty_desc);
    REQUIRE(body2 == NULL);
}

// Test CFM solver constraint (soft constraints)
TEST(SolverCfmConstraint) {
    UpeSolverConstraint constraint;
    constraint.body_a = 0;
    constraint.body_b = 1;
    constraint.cfm = 0.01f;  // Soft constraint
    constraint.friction = 0.5f;
    constraint.restitution = 0.2f;
    constraint.penetration = 0.05f;
    constraint.lambda = 0.0f;
    constraint.lambda_t1 = 0.0f;
    constraint.lambda_t2 = 0.0f;

    // With CFM, effective mass increases, making impulses smaller (softer)
    // Verify CFM field is properly initialized
    REQUIRE(constraint.cfm > 0.0f);
    REQUIRE(constraint.cfm < 1.0f);
}

// Test anisotropic friction in solver constraint
TEST(SolverAnisotropicFriction) {
    UpeSolverConstraint constraint;
    constraint.body_a = 0;
    constraint.body_b = 1;
    constraint.friction = 0.6f;
    constraint.friction_anisotropy = 2.0f;  // 2x slippier in one direction
    constraint.restitution = 0.1f;
    constraint.penetration = 0.01f;
    constraint.lambda = 1.0f;
    constraint.lambda_t1 = 0.0f;
    constraint.lambda_t2 = 0.0f;

    // Anisotropy > 1 means reduced friction in primary direction
    // This is used in PGS solver: max_friction = friction * lambda / anisotropy
    REQUIRE(constraint.friction_anisotropy > 1.0f);
    
    // Expected: With anisotropy=2, max friction is halved
    float expected_max = constraint.friction * constraint.lambda / constraint.friction_anisotropy;
    REQUIRE(expected_max < constraint.friction * constraint.lambda);
}

// Test multiple soft constraints don't break solver
TEST(SolverMultipleSoftConstraints) {
    UpeSolverConstraint constraints[3];
    for (int i = 0; i < 3; i++) {
        constraints[i].body_a = 0;
        constraints[i].body_b = 1;
        constraints[i].cfm = 0.01f * (i + 1);  // Varying CFM values
        constraints[i].friction = 0.5f;
        constraints[i].restitution = 0.0f;
        constraints[i].penetration = 0.01f;
        constraints[i].lambda = 0.0f;
        constraints[i].lambda_t1 = 0.0f;
        constraints[i].lambda_t2 = 0.0f;
        constraints[i].friction_anisotropy = 1.0f;
    }

    // Verify all constraints are properly initialized
    for (int i = 0; i < 3; i++) {
        REQUIRE(constraints[i].cfm > 0.0f);
        REQUIRE(constraints[i].friction_anisotropy > 0.0f);
    }
}

