#include "test_framework.h"
#include "upe/upe_geometry_scalar.h"

TEST(TestScalarAabb3ContainsAndExpand) {
    UpeScalarVec3 min = {UpeScalar_FromI32(0), UpeScalar_FromI32(0), UpeScalar_FromI32(0)};
    UpeScalarVec3 max = {UpeScalar_FromI32(2), UpeScalar_FromI32(2), UpeScalar_FromI32(2)};
    UpeScalarAabb3 box = UpeScalarAabb3_FromMinMax(min, max);

    UpeScalarVec3 inside = {UpeScalar_FromI32(1), UpeScalar_FromI32(2), UpeScalar_FromI32(0)};
    UpeScalarVec3 outside = {UpeScalar_FromI32(3), UpeScalar_FromI32(1), UpeScalar_FromI32(1)};

    REQUIRE(UpeScalarAabb3_Contains(box, inside) == 1);
    REQUIRE(UpeScalarAabb3_Contains(box, outside) == 0);

    UpeScalarVec3 point = {UpeScalar_FromI32(-1), UpeScalar_FromI32(1), UpeScalar_FromI32(1)};
    box = UpeScalarAabb3_Expand(box, point);
    REQUIRE(UpeScalar_ToI32(box.min.x) == -1);
}

TEST(TestScalarAabb3OverlapUnionCenter) {
    UpeScalarAabb3 a = UpeScalarAabb3_FromMinMax(
        {UpeScalar_FromI32(0), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(2), UpeScalar_FromI32(2), UpeScalar_FromI32(2)}
    );

    UpeScalarAabb3 b = UpeScalarAabb3_FromMinMax(
        {UpeScalar_FromI32(1), UpeScalar_FromI32(1), UpeScalar_FromI32(1)},
        {UpeScalar_FromI32(4), UpeScalar_FromI32(4), UpeScalar_FromI32(4)}
    );

    REQUIRE(UpeScalarAabb3_IsValid(a) == 1);
    REQUIRE(UpeScalarAabb3_IsValid(b) == 1);
    REQUIRE(UpeScalarAabb3_Overlap(a, b) == 1);

    UpeScalarAabb3 u = UpeScalarAabb3_Union(a, b);
    REQUIRE(UpeScalar_ToI32(u.min.x) == 0);
    REQUIRE(UpeScalar_ToI32(u.max.z) == 4);

    UpeScalarVec3 c = UpeScalarAabb3_Center(a);
    REQUIRE(UpeScalar_ToI32(c.x) == 1);
    REQUIRE(UpeScalar_ToI32(c.y) == 1);
    REQUIRE(UpeScalar_ToI32(c.z) == 1);
}
