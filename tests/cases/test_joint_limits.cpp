#include "test_framework.h"
#include "upe/upe_joint_limits.h"

TEST(TestJointLimitApply) {
    UpeJointLimit limit;
    UpeJointLimit_Init(&limit, -1.0f, 1.0f);

    float correction = 0.0f;
    UpeJointLimit_Apply(&limit, -2.0f, &correction);
    REQUIRE(correction > 0.0f);

    UpeJointLimit_Apply(&limit, 0.0f, &correction);
    REQUIRE(correction == 0.0f);
}

TEST(TestJointMotorApply) {
    UpeJointMotor motor;
    UpeJointMotor_Init(&motor, 10.0f, 100.0f);

    float force = 0.0f;
    UpeJointMotor_Apply(&motor, 0.0f, &force);
    REQUIRE(force > 0.0f);
}
