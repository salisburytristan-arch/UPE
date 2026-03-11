#include "test_framework.h"
#include "upe/upe_narrowphase_scalar.h"

TEST(TestScalarNarrowphaseAabbAabbOverlap) {
    UpeScalarAabb3 a = UpeScalarAabb3_FromMinMax(
        {UpeScalar_FromI32(0), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(2), UpeScalar_FromI32(2), UpeScalar_FromI32(2)}
    );
    UpeScalarAabb3 b = UpeScalarAabb3_FromMinMax(
        {UpeScalar_FromI32(1), UpeScalar_FromI32(1), UpeScalar_FromI32(1)},
        {UpeScalar_FromI32(3), UpeScalar_FromI32(3), UpeScalar_FromI32(3)}
    );

    UpeScalarContactManifold manifold = {};
    int hit = UpeNarrowphaseScalar_AabbAabb(a, b, &manifold);

    REQUIRE(hit == 1);
    REQUIRE(manifold.count == 1);
    REQUIRE(UpeScalar_ToI32(manifold.points[0].penetration) == 1);
    REQUIRE(UpeScalar_ToI32(manifold.points[0].normal.x) == 1);
    REQUIRE(UpeScalar_ToI32(manifold.points[0].normal.y) == 0);
    REQUIRE(UpeScalar_ToI32(manifold.points[0].normal.z) == 0);
    REQUIRE(UpeScalar_ToI32(manifold.points[0].position.x) == 1);
}

TEST(TestScalarNarrowphaseAabbAabbNoOverlap) {
    UpeScalarAabb3 a = UpeScalarAabb3_FromMinMax(
        {UpeScalar_FromI32(0), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(1), UpeScalar_FromI32(1), UpeScalar_FromI32(1)}
    );
    UpeScalarAabb3 b = UpeScalarAabb3_FromMinMax(
        {UpeScalar_FromI32(5), UpeScalar_FromI32(5), UpeScalar_FromI32(5)},
        {UpeScalar_FromI32(6), UpeScalar_FromI32(6), UpeScalar_FromI32(6)}
    );

    UpeScalarContactManifold manifold = {};
    int hit = UpeNarrowphaseScalar_AabbAabb(a, b, &manifold);

    REQUIRE(hit == 0);
    REQUIRE(manifold.count == 0);
}

TEST(TestScalarNarrowphaseSphereSphereOverlap) {
    UpeScalarSphere a = {
        {UpeScalar_FromI32(0), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        UpeScalar_FromI32(2)
    };
    UpeScalarSphere b = {
        {UpeScalar_FromI32(3), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        UpeScalar_FromI32(2)
    };
    UpeScalarSphere c = {
        {UpeScalar_FromI32(5), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        UpeScalar_FromI32(1)
    };

    REQUIRE(UpeNarrowphaseScalar_SphereSphereOverlap(a, b) == 1);
    REQUIRE(UpeNarrowphaseScalar_SphereSphereOverlap(a, c) == 0);
}

TEST(TestScalarNarrowphaseSphereAabbOverlap) {
    UpeScalarSphere sphere = {
        {UpeScalar_FromI32(3), UpeScalar_FromI32(1), UpeScalar_FromI32(1)},
        UpeScalar_FromI32(1)
    };
    UpeScalarAabb3 box = UpeScalarAabb3_FromMinMax(
        {UpeScalar_FromI32(0), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(2), UpeScalar_FromI32(2), UpeScalar_FromI32(2)}
    );

    UpeScalarSphere far_sphere = {
        {UpeScalar_FromI32(10), UpeScalar_FromI32(10), UpeScalar_FromI32(10)},
        UpeScalar_FromI32(1)
    };

    REQUIRE(UpeNarrowphaseScalar_SphereAabbOverlap(sphere, box) == 1);
    REQUIRE(UpeNarrowphaseScalar_SphereAabbOverlap(far_sphere, box) == 0);
}

TEST(TestScalarNarrowphaseSphereAabbManifold) {
    UpeScalarSphere sphere = {
        {UpeScalar_FromI32(3), UpeScalar_FromI32(1), UpeScalar_FromI32(1)},
        UpeScalar_FromI32(1)
    };
    UpeScalarAabb3 box = UpeScalarAabb3_FromMinMax(
        {UpeScalar_FromI32(0), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(2), UpeScalar_FromI32(2), UpeScalar_FromI32(2)}
    );

    UpeScalarContactManifold manifold = {};
    int hit = UpeNarrowphaseScalar_SphereAabb(sphere, box, &manifold);

    REQUIRE(hit == 1);
    REQUIRE(manifold.count == 1);
    REQUIRE(UpeScalar_ToI32(manifold.points[0].normal.x) == 1);
    REQUIRE(UpeScalar_ToI32(manifold.points[0].position.x) == 2);
}

TEST(TestScalarNarrowphaseCapsuleAabbOverlap) {
    UpeScalarCapsule capsule = {
        {UpeScalar_FromI32(3), UpeScalar_FromI32(1), UpeScalar_FromI32(1)},
        {UpeScalar_FromI32(5), UpeScalar_FromI32(1), UpeScalar_FromI32(1)},
        UpeScalar_FromI32(1)
    };
    UpeScalarAabb3 box = UpeScalarAabb3_FromMinMax(
        {UpeScalar_FromI32(0), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(2), UpeScalar_FromI32(2), UpeScalar_FromI32(2)}
    );

    UpeScalarCapsule far_capsule = {
        {UpeScalar_FromI32(20), UpeScalar_FromI32(20), UpeScalar_FromI32(20)},
        {UpeScalar_FromI32(22), UpeScalar_FromI32(20), UpeScalar_FromI32(20)},
        UpeScalar_FromI32(1)
    };

    REQUIRE(UpeNarrowphaseScalar_CapsuleAabbOverlap(capsule, box) == 1);
    REQUIRE(UpeNarrowphaseScalar_CapsuleAabbOverlap(far_capsule, box) == 0);
}

TEST(TestScalarNarrowphaseSphereSphereManifold) {
    UpeScalarSphere a = {
        {UpeScalar_FromI32(0), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        UpeScalar_FromI32(2)
    };
    UpeScalarSphere b = {
        {UpeScalar_FromI32(3), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        UpeScalar_FromI32(2)
    };

    UpeScalarContactManifold manifold = {};
    int hit = UpeNarrowphaseScalar_SphereSphere(a, b, &manifold);

    REQUIRE(hit == 1);
    REQUIRE(manifold.count == 1);
    REQUIRE(UpeScalar_ToI32(manifold.points[0].normal.x) == 1);
    REQUIRE(UpeScalar_ToI32(manifold.points[0].penetration) == 1);
}

TEST(TestScalarNarrowphaseCapsuleCapsuleOverlap) {
    UpeScalarCapsule a = {
        {UpeScalar_FromI32(0), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(2), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        UpeScalar_FromI32(1)
    };
    UpeScalarCapsule b = {
        {UpeScalar_FromI32(1), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(3), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        UpeScalar_FromI32(1)
    };
    UpeScalarCapsule far_b = {
        {UpeScalar_FromI32(10), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(12), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        UpeScalar_FromI32(1)
    };

    REQUIRE(UpeNarrowphaseScalar_CapsuleCapsuleOverlap(a, b) == 1);
    REQUIRE(UpeNarrowphaseScalar_CapsuleCapsuleOverlap(a, far_b) == 0);
}

TEST(TestScalarNarrowphaseSphereCapsuleOverlapAndManifold) {
    UpeScalarSphere sphere = {
        {UpeScalar_FromI32(2), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        UpeScalar_FromI32(1)
    };
    UpeScalarCapsule capsule = {
        {UpeScalar_FromI32(0), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(1), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        UpeScalar_FromI32(1)
    };
    UpeScalarCapsule far_capsule = {
        {UpeScalar_FromI32(10), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(11), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        UpeScalar_FromI32(1)
    };

    REQUIRE(UpeNarrowphaseScalar_SphereCapsuleOverlap(sphere, capsule) == 1);
    REQUIRE(UpeNarrowphaseScalar_SphereCapsuleOverlap(sphere, far_capsule) == 0);

    UpeScalarContactManifold manifold = {};
    int hit = UpeNarrowphaseScalar_SphereCapsule(sphere, capsule, &manifold);
    REQUIRE(hit == 1);
    REQUIRE(manifold.count == 1);
    REQUIRE(UpeScalar_ToI32(manifold.points[0].normal.x) == 1);
}

TEST(TestScalarNarrowphaseCapsuleCapsuleManifold) {
    UpeScalarCapsule a = {
        {UpeScalar_FromI32(0), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(2), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        UpeScalar_FromI32(1)
    };
    UpeScalarCapsule b = {
        {UpeScalar_FromI32(1), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(3), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        UpeScalar_FromI32(1)
    };

    UpeScalarContactManifold manifold = {};
    int hit = UpeNarrowphaseScalar_CapsuleCapsule(a, b, &manifold);
    REQUIRE(hit == 1);
    REQUIRE(manifold.count == 1);
    REQUIRE(UpeScalar_ToI32(manifold.points[0].normal.x) == 1);
}
