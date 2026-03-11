#include "test_framework.h"
#include "upe/upe_joint.h"
#include "upe/upe_solver.h"

TEST(TestJointInit) {
    UpeJointDesc desc = {};
    desc.type = UPE_JOINT_HINGE;
    desc.body_a = 0;
    desc.body_b = 1;
    desc.anchor_a = {1.0f, 0.0f, 0.0f};
    desc.anchor_b = {-1.0f, 0.0f, 0.0f};
    desc.axis = {0.0f, 1.0f, 0.0f};

    UpeJoint joint;
    UpeJoint_Init(&joint, &desc);
    REQUIRE(joint.type == UPE_JOINT_HINGE);
    REQUIRE(joint.body_a == 0);
    REQUIRE(joint.body_b == 1);
}
