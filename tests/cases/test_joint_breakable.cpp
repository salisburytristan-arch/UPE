#include "test_framework.h"
#include "upe/upe_joint.h"
#include "upe/upe_solver.h"

TEST(TestBreakableJoint) {
    UpeJointDesc desc = {};
    desc.type = UPE_JOINT_HINGE;
    desc.body_a = 0;
    desc.body_b = 1;
    desc.anchor_a = {0.0f, 0.0f, 0.0f};
    desc.anchor_b = {0.0f, 0.0f, 0.0f};
    desc.axis = {0.0f, 1.0f, 0.0f};
    desc.max_impulse = 0.01f;

    UpeJoint joint = {};
    UpeJoint_Init(&joint, &desc);

    UpeBodyState bodies[2] = {};
    bodies[0].position = {0.0f, 0.0f, 0.0f};
    bodies[1].position = {10.0f, 0.0f, 0.0f};
    bodies[0].rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    bodies[1].rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    bodies[0].inv_mass = 1.0f;
    bodies[1].inv_mass = 1.0f;
    bodies[0].inv_inertia = {1.0f, 1.0f, 1.0f};
    bodies[1].inv_inertia = {1.0f, 1.0f, 1.0f};

    UpeJoint_Solve(&joint, bodies, 2);
    REQUIRE(joint.broken == 1);
}
