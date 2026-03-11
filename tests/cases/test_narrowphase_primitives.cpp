#include "test_framework.h"
#include "upe/upe_narrowphase.h"

TEST(TestSphereSphereContact) {
    UpeSphere a = {{0.0f, 0.0f, 0.0f}, 1.0f};
    UpeSphere b = {{1.5f, 0.0f, 0.0f}, 1.0f};
    UpeContactManifold manifold;

    REQUIRE(UpeNarrowphase_SphereSphere(a, b, &manifold));
    REQUIRE(manifold.count == 1);
    REQUIRE(manifold.points[0].penetration > 0.0f);
}

TEST(TestCapsuleCapsuleContact) {
    UpeCapsule a = {{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 0.5f};
    UpeCapsule b = {{0.5f, 0.0f, 0.0f}, {0.5f, 1.0f, 0.0f}, 0.5f};
    UpeContactManifold manifold;

    REQUIRE(UpeNarrowphase_CapsuleCapsule(a, b, &manifold));
    REQUIRE(manifold.count == 1);
}

TEST(TestAabbAabbContact) {
    UpeAabb3 a = UpeAabb3_FromMinMax({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    UpeAabb3 b = UpeAabb3_FromMinMax({0.5f, 0.0f, 0.0f}, {1.5f, 1.0f, 1.0f});
    UpeContactManifold manifold;

    REQUIRE(UpeNarrowphase_AabbAabb(a, b, &manifold));
    REQUIRE(manifold.count == 1);
}
