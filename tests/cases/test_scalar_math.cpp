#include "test_framework.h"
#include "upe/upe_math_scalar.h"

TEST(TestScalarVec2AddAndDot) {
    UpeScalarVec2 a = {UpeScalar_FromI32(2), UpeScalar_FromI32(3)};
    UpeScalarVec2 b = {UpeScalar_FromI32(4), UpeScalar_FromI32(-1)};

    UpeScalarVec2 sum = UpeScalarVec2_Add(a, b);
    UpeScalar dot = UpeScalarVec2_Dot(a, b);

    REQUIRE(UpeScalar_ToI32(sum.x) == 6);
    REQUIRE(UpeScalar_ToI32(sum.y) == 2);
    REQUIRE(UpeScalar_ToI32(dot) == 5);
}

TEST(TestScalarVec3ScaleMinMax) {
    UpeScalarVec3 v = {UpeScalar_FromI32(1), UpeScalar_FromI32(-2), UpeScalar_FromI32(3)};
    UpeScalar two = UpeScalar_FromI32(2);
    UpeScalarVec3 scaled = UpeScalarVec3_Scale(v, two);

    REQUIRE(UpeScalar_ToI32(scaled.x) == 2);
    REQUIRE(UpeScalar_ToI32(scaled.y) == -4);
    REQUIRE(UpeScalar_ToI32(scaled.z) == 6);

    UpeScalarVec3 a = {UpeScalar_FromI32(10), UpeScalar_FromI32(-5), UpeScalar_FromI32(4)};
    UpeScalarVec3 b = {UpeScalar_FromI32(6), UpeScalar_FromI32(7), UpeScalar_FromI32(9)};
    UpeScalarVec3 mn = UpeScalarVec3_Min(a, b);
    UpeScalarVec3 mx = UpeScalarVec3_Max(a, b);

    REQUIRE(UpeScalar_ToI32(mn.x) == 6);
    REQUIRE(UpeScalar_ToI32(mn.y) == -5);
    REQUIRE(UpeScalar_ToI32(mn.z) == 4);

    REQUIRE(UpeScalar_ToI32(mx.x) == 10);
    REQUIRE(UpeScalar_ToI32(mx.y) == 7);
    REQUIRE(UpeScalar_ToI32(mx.z) == 9);
}
