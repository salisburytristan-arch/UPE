#include "test_framework.h"
#include "upe/upe_friction.h"

TEST(TestFrictionSolve) {
    UpeBodyState bodies[2];
    bodies[0].position = {0.0f, 0.0f, 0.0f};
    bodies[0].velocity = {1.0f, 0.0f, 0.0f};
    bodies[0].inv_mass = 1.0f;
    bodies[1].position = {0.0f, 0.0f, 0.0f};
    bodies[1].velocity = {0.0f, 0.0f, 0.0f};
    bodies[1].inv_mass = 0.0f;

    UpeFrictionConstraint constraint;
    constraint.body_a = 0;
    constraint.body_b = 1;
    constraint.tangent = {1.0f, 0.0f, 0.0f};
    constraint.lambda = 0.0f;
    constraint.friction_coeff = 0.5f;

    UpeFriction_Solve(bodies, 2, &constraint, 1);
    REQUIRE(bodies[0].velocity.x < 1.0f);
}
