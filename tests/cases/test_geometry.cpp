#include "test_framework.h"
#include "upe/upe_geometry.h"

TEST(TestAabbContains) {
    UpeAabb3 box = UpeAabb3_FromMinMax({-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f});
    REQUIRE(UpeAabb3_Contains(box, {0.0f, 0.0f, 0.0f}));
    REQUIRE(!UpeAabb3_Contains(box, {2.0f, 0.0f, 0.0f}));
}
