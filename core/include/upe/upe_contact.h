#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeContactPoint {
    UpeVec3 position;
    UpeVec3 normal;
    float penetration;
} UpeContactPoint;

typedef struct UpeContactManifold {
    UpeContactPoint points[4];
    uint32_t count;
} UpeContactManifold;

void UpeContactManifold_Clear(UpeContactManifold *manifold);

#ifdef __cplusplus
}
#endif
