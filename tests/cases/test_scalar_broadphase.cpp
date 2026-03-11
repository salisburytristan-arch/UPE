#include "test_framework.h"
#include "upe/upe_broadphase_scalar.h"

TEST(TestScalarBroadphaseSapPairsBasic) {
    UpeScalarAabb3 boxes[4];
    boxes[0] = UpeScalarAabb3_FromMinMax(
        {UpeScalar_FromI32(0), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(2), UpeScalar_FromI32(2), UpeScalar_FromI32(2)}
    );
    boxes[1] = UpeScalarAabb3_FromMinMax(
        {UpeScalar_FromI32(1), UpeScalar_FromI32(1), UpeScalar_FromI32(1)},
        {UpeScalar_FromI32(3), UpeScalar_FromI32(3), UpeScalar_FromI32(3)}
    );
    boxes[2] = UpeScalarAabb3_FromMinMax(
        {UpeScalar_FromI32(5), UpeScalar_FromI32(5), UpeScalar_FromI32(5)},
        {UpeScalar_FromI32(6), UpeScalar_FromI32(6), UpeScalar_FromI32(6)}
    );
    boxes[3] = UpeScalarAabb3_FromMinMax(
        {UpeScalar_FromI32(2), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(4), UpeScalar_FromI32(2), UpeScalar_FromI32(2)}
    );

    UpeScalarBroadphasePair pairs[8] = {};
    uint32_t count = UpeBroadphaseScalar_SapPairs(boxes, 4, pairs, 8);

    REQUIRE(count == 3);
    REQUIRE(pairs[0].a == 0 && pairs[0].b == 1);
    REQUIRE(pairs[1].a == 0 && pairs[1].b == 3);
    REQUIRE(pairs[2].a == 1 && pairs[2].b == 3);
}

TEST(TestScalarBroadphaseSapPairsDeterministicOrder) {
    UpeScalarAabb3 boxes[3];
    boxes[0] = UpeScalarAabb3_FromMinMax(
        {UpeScalar_FromI32(10), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(12), UpeScalar_FromI32(1), UpeScalar_FromI32(1)}
    );
    boxes[1] = UpeScalarAabb3_FromMinMax(
        {UpeScalar_FromI32(8), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(11), UpeScalar_FromI32(1), UpeScalar_FromI32(1)}
    );
    boxes[2] = UpeScalarAabb3_FromMinMax(
        {UpeScalar_FromI32(9), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(13), UpeScalar_FromI32(1), UpeScalar_FromI32(1)}
    );

    UpeScalarBroadphasePair pairs[8] = {};
    uint32_t count = UpeBroadphaseScalar_SapPairs(boxes, 3, pairs, 8);

    REQUIRE(count == 3);
    REQUIRE(pairs[0].a == 0 && pairs[0].b == 1);
    REQUIRE(pairs[1].a == 0 && pairs[1].b == 2);
    REQUIRE(pairs[2].a == 1 && pairs[2].b == 2);
}

TEST(TestScalarBroadphaseSapPairsFiltered) {
    UpeScalarAabb3 boxes[3];
    boxes[0] = UpeScalarAabb3_FromMinMax(
        {UpeScalar_FromI32(0), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(2), UpeScalar_FromI32(2), UpeScalar_FromI32(2)}
    );
    boxes[1] = UpeScalarAabb3_FromMinMax(
        {UpeScalar_FromI32(1), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(3), UpeScalar_FromI32(2), UpeScalar_FromI32(2)}
    );
    boxes[2] = UpeScalarAabb3_FromMinMax(
        {UpeScalar_FromI32(1), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(3), UpeScalar_FromI32(2), UpeScalar_FromI32(2)}
    );

    uint32_t layers[3] = {0, 1, 2};
    uint32_t masks[3] = {
        (1u << 1),
        (1u << 0),
        0u
    };

    UpeScalarBroadphasePair pairs[8] = {};
    uint32_t count = UpeBroadphaseScalar_SapPairsFiltered(boxes, layers, masks, 3, pairs, 8);

    REQUIRE(count == 1);
    REQUIRE(pairs[0].a == 0 && pairs[0].b == 1);
}
