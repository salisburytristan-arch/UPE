#include "test_framework.h"
#include "upe/upe_collision_pipeline_scalar.h"

TEST(TestScalarCollisionPipelineBuildAndSolve) {
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
        {UpeScalar_FromI32(6), UpeScalar_FromI32(6), UpeScalar_FromI32(6)},
        {UpeScalar_FromI32(7), UpeScalar_FromI32(7), UpeScalar_FromI32(7)}
    );
    boxes[3] = UpeScalarAabb3_FromMinMax(
        {UpeScalar_FromI32(2), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(4), UpeScalar_FromI32(2), UpeScalar_FromI32(2)}
    );

    UpeScalarBroadphasePair pairs[16] = {};
    uint32_t pair_count = UpeCollisionScalar_BuildAabbPairs(boxes, 4, pairs, 16);

    REQUIRE(pair_count == 3);
    REQUIRE(pairs[0].a == 0 && pairs[0].b == 1);
    REQUIRE(pairs[1].a == 0 && pairs[1].b == 3);
    REQUIRE(pairs[2].a == 1 && pairs[2].b == 3);

    UpeScalarCollisionResult results[16] = {};
    uint32_t result_count = UpeCollisionScalar_SolveAabbPairs(boxes, pairs, pair_count, results, 16);

    REQUIRE(result_count == 3);
    REQUIRE(results[0].pair.a == 0 && results[0].pair.b == 1);
    REQUIRE(results[1].pair.a == 0 && results[1].pair.b == 3);
    REQUIRE(results[2].pair.a == 1 && results[2].pair.b == 3);
    REQUIRE(results[0].manifold.count == 1);
    REQUIRE(results[1].manifold.count == 1);
    REQUIRE(results[2].manifold.count == 1);
}

TEST(TestScalarCollisionPipelineDetectAabb) {
    UpeScalarAabb3 boxes[3];
    boxes[0] = UpeScalarAabb3_FromMinMax(
        {UpeScalar_FromI32(0), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(1), UpeScalar_FromI32(1), UpeScalar_FromI32(1)}
    );
    boxes[1] = UpeScalarAabb3_FromMinMax(
        {UpeScalar_FromI32(1), UpeScalar_FromI32(0), UpeScalar_FromI32(0)},
        {UpeScalar_FromI32(2), UpeScalar_FromI32(1), UpeScalar_FromI32(1)}
    );
    boxes[2] = UpeScalarAabb3_FromMinMax(
        {UpeScalar_FromI32(5), UpeScalar_FromI32(5), UpeScalar_FromI32(5)},
        {UpeScalar_FromI32(6), UpeScalar_FromI32(6), UpeScalar_FromI32(6)}
    );

    UpeScalarBroadphasePair scratch[8] = {};
    UpeScalarCollisionResult results[8] = {};

    uint32_t count = UpeCollisionScalar_DetectAabb(boxes, 3, scratch, 8, results, 8);
    REQUIRE(count == 1);
    REQUIRE(results[0].pair.a == 0 && results[0].pair.b == 1);
    REQUIRE(results[0].manifold.count == 1);
}

TEST(TestScalarCollisionPipelineDetectAabbFiltered) {
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

    UpeScalarBroadphasePair scratch[8] = {};
    UpeScalarCollisionResult results[8] = {};

    uint32_t count = UpeCollisionScalar_DetectAabbFiltered(
        boxes,
        layers,
        masks,
        3,
        scratch,
        8,
        results,
        8
    );

    REQUIRE(count == 1);
    REQUIRE(results[0].pair.a == 0 && results[0].pair.b == 1);
}
