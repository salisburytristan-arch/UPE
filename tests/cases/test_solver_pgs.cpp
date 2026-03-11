#include "test_framework.h"
#include "upe/upe_solver.h"

TEST(TestPgsSolverIteration) {
    UpeBodyState bodies[2];
    bodies[0].position = {0.0f, 0.0f, 0.0f};
    bodies[0].velocity = {0.0f, -1.0f, 0.0f};
    bodies[0].rotation = UpeQuat_Identity();
    bodies[0].angular_velocity = {0.0f, 0.0f, 0.0f};
    bodies[0].inv_mass = 1.0f;
    bodies[0].inv_inertia = {1.0f, 1.0f, 1.0f};
    
    bodies[1].position = {0.0f, 1.0f, 0.0f};
    bodies[1].velocity = {0.0f, 0.0f, 0.0f};
    bodies[1].rotation = UpeQuat_Identity();
    bodies[1].angular_velocity = {0.0f, 0.0f, 0.0f};
    bodies[1].inv_mass = 0.0f;
    bodies[1].inv_inertia = {0.0f, 0.0f, 0.0f};

    UpeSolverConstraint constraint = {};
    constraint.body_a = 0;
    constraint.body_b = 1;
    constraint.normal = {0.0f, 1.0f, 0.0f};
    constraint.contact_point = {0.0f, 0.5f, 0.0f};
    constraint.penetration = 0.1f;
    constraint.lambda = 0.0f;
    constraint.lambda_t1 = 0.0f;
    constraint.lambda_t2 = 0.0f;
    constraint.friction = 0.5f;
    constraint.restitution = 0.0f;
    constraint.r_a = {0.0f, 0.5f, 0.0f};
    constraint.r_b = {0.0f, -0.5f, 0.0f};
    constraint.tangent1 = {1.0f, 0.0f, 0.0f};
    constraint.tangent2 = {0.0f, 0.0f, 1.0f};

    UpeSolver_PgsIteration(bodies, 2, &constraint, 1);
    // Body A moving down at -1.0 m/s should be slowed by collision with static body B
    // printf("After PGS: bodies[0].vel.y=%.3f (was -1.0)\\n", bodies[0].velocity.y);
    REQUIRE(bodies[0].velocity.y > -1.0f);
}
