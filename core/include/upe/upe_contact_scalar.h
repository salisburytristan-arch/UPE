#pragma once

#include <stdint.h>
#include "upe/upe_math_scalar.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeScalarContactPoint {
    UpeScalarVec3 position;
    UpeScalarVec3 normal;
    UpeScalar penetration;
} UpeScalarContactPoint;

typedef struct UpeScalarContactManifold {
    UpeScalarContactPoint points[4];
    uint32_t count;
} UpeScalarContactManifold;

void UpeScalarContactManifold_Clear(UpeScalarContactManifold *manifold);

#ifdef __cplusplus
}
#endif
