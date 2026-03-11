#include "test_framework.h"
#include "upe/upe_math.h"

TEST(TestVec3Cross) {
    UpeVec3 a{1.0f, 0.0f, 0.0f};
    UpeVec3 b{0.0f, 1.0f, 0.0f};
    UpeVec3 c = UpeVec3_Cross(a, b);
    REQUIRE(c.x == 0.0f);
    REQUIRE(c.y == 0.0f);
    REQUIRE(c.z == 1.0f);
}

TEST(TestQuatIdentity) {
    UpeQuat q = UpeQuat_Identity();
    UpeVec3 v{2.0f, 0.0f, 0.0f};
    UpeVec3 out = UpeQuat_RotateVec3(q, v);
    REQUIRE(out.x == v.x);
    REQUIRE(out.y == v.y);
    REQUIRE(out.z == v.z);
}
