#include "test_framework.h"
#include "upe/upe_geometry.h"

TEST(TestAabbOverlap) {
    UpeAabb3 a = UpeAabb3_FromMinMax({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    UpeAabb3 b = UpeAabb3_FromMinMax({0.5f, 0.0f, 0.0f}, {1.5f, 1.0f, 1.0f});
    UpeAabb3 c = UpeAabb3_FromMinMax({2.0f, 0.0f, 0.0f}, {3.0f, 1.0f, 1.0f});
    REQUIRE(UpeAabb3_Overlap(a, b));
    REQUIRE(!UpeAabb3_Overlap(a, c));
}
