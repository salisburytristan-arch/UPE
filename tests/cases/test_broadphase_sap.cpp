#include "test_framework.h"
#include "upe/upe_broadphase.h"

TEST(TestBroadphaseSapPairs) {
    UpeAabb3 bounds[3];
    bounds[0] = UpeAabb3_FromMinMax({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    bounds[1] = UpeAabb3_FromMinMax({0.5f, 0.0f, 0.0f}, {1.5f, 1.0f, 1.0f});
    bounds[2] = UpeAabb3_FromMinMax({2.0f, 0.0f, 0.0f}, {3.0f, 1.0f, 1.0f});

    UpeBroadphasePair pairs[4];
    uint32_t count = UpeBroadphase_SapPairs(bounds, 3, pairs, 4);
    REQUIRE(count == 1);
    REQUIRE((pairs[0].a == 0 && pairs[0].b == 1) || (pairs[0].a == 1 && pairs[0].b == 0));
}
