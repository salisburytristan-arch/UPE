#include "test_framework.h"
#include "upe/upe_math.h"

TEST(TestVec2Add) {
    UpeVec2 a{1.0f, 2.0f};
    UpeVec2 b{3.0f, 4.0f};
    UpeVec2 c = UpeVec2_Add(a, b);
    REQUIRE(c.x == 4.0f);
    REQUIRE(c.y == 6.0f);
}

TEST(TestVec3Dot) {
    UpeVec3 a{1.0f, 0.0f, 0.0f};
    UpeVec3 b{0.0f, 2.0f, 0.0f};
    float d = UpeVec3_Dot(a, b);
    REQUIRE(d == 0.0f);
}
