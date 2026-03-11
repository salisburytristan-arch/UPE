#include "test_framework.h"
#include "upe/upe_joint.h"
#include "upe/upe_solver.h"
#include <math.h>

TEST(TestGearJointReducesConstraint) {
    UpeJointDesc desc = {};
    desc.type = UPE_JOINT_GEAR;
    desc.body_a = 0;
    desc.body_b = 1;
    desc.axis = {0.0f, 1.0f, 0.0f};
    desc.gear_ratio = 2.0f;

    UpeJoint joint = {};
    UpeJoint_Init(&joint, &desc);

    UpeBodyState bodies[2] = {};
    bodies[0].rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    bodies[1].rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    bodies[0].angular_velocity = {0.0f, 1.0f, 0.0f};
    bodies[1].angular_velocity = {0.0f, 0.0f, 0.0f};
    bodies[0].inv_mass = 1.0f;
    bodies[1].inv_mass = 1.0f;
    bodies[0].inv_inertia = {1.0f, 1.0f, 1.0f};
    bodies[1].inv_inertia = {1.0f, 1.0f, 1.0f};

    float before = bodies[0].angular_velocity.y + desc.gear_ratio * bodies[1].angular_velocity.y;
    UpeJoint_Solve(&joint, bodies, 2);
    float after = bodies[0].angular_velocity.y + desc.gear_ratio * bodies[1].angular_velocity.y;

    REQUIRE(fabsf(after) < fabsf(before));
}
