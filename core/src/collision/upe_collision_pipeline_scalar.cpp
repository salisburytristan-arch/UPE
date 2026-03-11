#include "upe/upe_collision_pipeline_scalar.h"

uint32_t UpeCollisionScalar_BuildAabbPairs(const UpeScalarAabb3 *bounds, uint32_t count,
                                           UpeScalarBroadphasePair *out_pairs, uint32_t max_pairs) {
    return UpeBroadphaseScalar_SapPairs(bounds, count, out_pairs, max_pairs);
}

uint32_t UpeCollisionScalar_BuildAabbPairsFiltered(const UpeScalarAabb3 *bounds,
                                                   const uint32_t *collision_layers,
                                                   const uint32_t *collision_masks,
                                                   uint32_t count,
                                                   UpeScalarBroadphasePair *out_pairs,
                                                   uint32_t max_pairs) {
    return UpeBroadphaseScalar_SapPairsFiltered(bounds, collision_layers, collision_masks, count, out_pairs, max_pairs);
}

uint32_t UpeCollisionScalar_SolveAabbPairs(const UpeScalarAabb3 *bounds,
                                           const UpeScalarBroadphasePair *pairs, uint32_t pair_count,
                                           UpeScalarCollisionResult *out_results, uint32_t max_results) {
    if (!bounds || !pairs || pair_count == 0 || !out_results || max_results == 0) {
        return 0;
    }

    uint32_t result_count = 0;
    for (uint32_t i = 0; i < pair_count && result_count < max_results; ++i) {
        const UpeScalarBroadphasePair pair = pairs[i];
        UpeScalarContactManifold manifold = {};

        int hit = UpeNarrowphaseScalar_AabbAabb(bounds[pair.a], bounds[pair.b], &manifold);
        if (hit) {
            out_results[result_count].pair = pair;
            out_results[result_count].manifold = manifold;
            ++result_count;
        }
    }

    return result_count;
}

uint32_t UpeCollisionScalar_DetectAabb(const UpeScalarAabb3 *bounds, uint32_t count,
                                       UpeScalarBroadphasePair *scratch_pairs, uint32_t scratch_pair_capacity,
                                       UpeScalarCollisionResult *out_results, uint32_t max_results) {
    if (!bounds || count == 0 || !scratch_pairs || scratch_pair_capacity == 0 || !out_results || max_results == 0) {
        return 0;
    }

    uint32_t pair_count = UpeCollisionScalar_BuildAabbPairs(bounds, count, scratch_pairs, scratch_pair_capacity);
    return UpeCollisionScalar_SolveAabbPairs(bounds, scratch_pairs, pair_count, out_results, max_results);
}

uint32_t UpeCollisionScalar_DetectAabbFiltered(const UpeScalarAabb3 *bounds,
                                               const uint32_t *collision_layers,
                                               const uint32_t *collision_masks,
                                               uint32_t count,
                                               UpeScalarBroadphasePair *scratch_pairs,
                                               uint32_t scratch_pair_capacity,
                                               UpeScalarCollisionResult *out_results,
                                               uint32_t max_results) {
    if (!bounds || count == 0 || !scratch_pairs || scratch_pair_capacity == 0 || !out_results || max_results == 0) {
        return 0;
    }

    uint32_t pair_count = UpeCollisionScalar_BuildAabbPairsFiltered(
        bounds,
        collision_layers,
        collision_masks,
        count,
        scratch_pairs,
        scratch_pair_capacity
    );
    return UpeCollisionScalar_SolveAabbPairs(bounds, scratch_pairs, pair_count, out_results, max_results);
}
