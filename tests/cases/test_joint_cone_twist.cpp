#include "test_framework.h"
#include "upe/upe_joint.h"
#include "upe/upe_solver.h"

TEST(TestJointConeTwistLimits) {
    UpeJointDesc desc = {};
    desc.type = UPE_JOINT_CONE_TWIST;
    desc.body_a = 0;
    desc.body_b = 1;
    desc.anchor_a = {0.0f, 0.0f, 0.0f};
    desc.anchor_b = {0.0f, 0.0f, 0.0f};
    desc.axis = {0.0f, 1.0f, 0.0f};
    desc.cone_angle = 0.1f;
    desc.twist_min = -0.1f;
    desc.twist_max = 0.1f;

    UpeJoint joint = {};
    UpeJoint_Init(&joint, &desc);

    UpeBodyState bodies[2] = {};
    bodies[0].rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    bodies[1].rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    bodies[0].inv_mass = 1.0f;
    bodies[1].inv_mass = 1.0f;
    bodies[0].inv_inertia = {1.0f, 1.0f, 1.0f};
    bodies[1].inv_inertia = {1.0f, 1.0f, 1.0f};
    bodies[0].angular_velocity = {3.0f, 0.0f, 0.0f};
    bodies[1].angular_velocity = {0.0f, 0.0f, 0.0f};

    UpeJoint_Solve(&joint, bodies, 2);

    float swing_speed_after = UpeVec3_Length(UpeVec3_Sub(bodies[0].angular_velocity, bodies[1].angular_velocity));
    REQUIRE(swing_speed_after <= 3.0f);
}