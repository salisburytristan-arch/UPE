#include "test_framework.h"
#include "upe/upe_math.h"

TEST(TestMat4Identity) {
    UpeMat4 m = UpeMat4_Identity();
    UpeVec4 v{1.0f, 2.0f, 3.0f, 1.0f};
    UpeVec4 out = UpeMat4_MulVec4(m, v);
    REQUIRE(out.x == 1.0f);
    REQUIRE(out.y == 2.0f);
    REQUIRE(out.z == 3.0f);
    REQUIRE(out.w == 1.0f);
}

TEST(TestVec4Dot) {
    UpeVec4 a{1.0f, 0.0f, 0.0f, 2.0f};
    UpeVec4 b{0.0f, 3.0f, 0.0f, 4.0f};
    REQUIRE(UpeVec4_Dot(a, b) == 8.0f);
}
