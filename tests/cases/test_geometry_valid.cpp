#include "test_framework.h"
#include "upe/upe_geometry.h"

TEST(TestGeometryValid) {
    UpeAabb2 aabb2 = UpeAabb2_FromMinMax({0.0f, 0.0f}, {1.0f, 1.0f});
    UpeAabb3 aabb3 = UpeAabb3_FromMinMax({-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f});
    UpeSphere sphere = {{0.0f, 0.0f, 0.0f}, 2.0f};
    UpeCapsule capsule = {{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 0.5f};
    UpeObb obb = {{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, UpeMat3_Identity()};

    REQUIRE(UpeAabb2_IsValid(aabb2));
    REQUIRE(UpeAabb3_IsValid(aabb3));
    REQUIRE(UpeSphere_IsValid(sphere));
    REQUIRE(UpeCapsule_IsValid(capsule));
    REQUIRE(UpeObb_IsValid(obb));
}
