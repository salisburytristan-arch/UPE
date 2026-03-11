#pragma once

#include <stdint.h>
#include "upe/upe_geometry_scalar.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeScalarBroadphasePair {
    uint32_t a;
    uint32_t b;
} UpeScalarBroadphasePair;

uint32_t UpeBroadphaseScalar_SapPairs(const UpeScalarAabb3 *bounds, uint32_t count,
                                      UpeScalarBroadphasePair *out_pairs, uint32_t max_pairs);

uint32_t UpeBroadphaseScalar_SapPairsFiltered(const UpeScalarAabb3 *bounds,
                                              const uint32_t *collision_layers,
                                              const uint32_t *collision_masks,
                                              uint32_t count,
                                              UpeScalarBroadphasePair *out_pairs,
                                              uint32_t max_pairs);

#ifdef __cplusplus
}
#endif
