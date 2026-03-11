#pragma once

#include <stdint.h>
#include "upe/upe_broadphase_scalar.h"
#include "upe/upe_narrowphase_scalar.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeScalarCollisionResult {
    UpeScalarBroadphasePair pair;
    UpeScalarContactManifold manifold;
} UpeScalarCollisionResult;

uint32_t UpeCollisionScalar_BuildAabbPairs(const UpeScalarAabb3 *bounds, uint32_t count,
                                           UpeScalarBroadphasePair *out_pairs, uint32_t max_pairs);

uint32_t UpeCollisionScalar_BuildAabbPairsFiltered(const UpeScalarAabb3 *bounds,
                                                   const uint32_t *collision_layers,
                                                   const uint32_t *collision_masks,
                                                   uint32_t count,
                                                   UpeScalarBroadphasePair *out_pairs,
                                                   uint32_t max_pairs);

uint32_t UpeCollisionScalar_SolveAabbPairs(const UpeScalarAabb3 *bounds,
                                           const UpeScalarBroadphasePair *pairs, uint32_t pair_count,
                                           UpeScalarCollisionResult *out_results, uint32_t max_results);

uint32_t UpeCollisionScalar_DetectAabb(const UpeScalarAabb3 *bounds, uint32_t count,
                                       UpeScalarBroadphasePair *scratch_pairs, uint32_t scratch_pair_capacity,
                                       UpeScalarCollisionResult *out_results, uint32_t max_results);

uint32_t UpeCollisionScalar_DetectAabbFiltered(const UpeScalarAabb3 *bounds,
                                               const uint32_t *collision_layers,
                                               const uint32_t *collision_masks,
                                               uint32_t count,
                                               UpeScalarBroadphasePair *scratch_pairs,
                                               uint32_t scratch_pair_capacity,
                                               UpeScalarCollisionResult *out_results,
                                               uint32_t max_results);

#ifdef __cplusplus
}
#endif
