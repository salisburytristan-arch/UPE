#pragma once

#include <stdint.h>
#include "upe/upe_geometry.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeBroadphasePair {
    uint32_t a;
    uint32_t b;
} UpeBroadphasePair;

uint32_t UpeBroadphase_SapPairs(const UpeAabb3 *bounds, uint32_t count,
                                 UpeBroadphasePair *out_pairs, uint32_t max_pairs);

/* BVH-based broadphase (O(N log N) vs SAP's O(N²)) */
uint32_t UpeBroadphase_BvhPairs(const UpeAabb3 *bounds, uint32_t count,
                                 UpeBroadphasePair *out_pairs, uint32_t max_pairs,
                                 void *bvh_scratch, uint32_t scratch_size);

#ifdef __cplusplus
}
#endif
