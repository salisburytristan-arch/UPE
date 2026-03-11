#pragma once

#include "upe/upe_math_scalar.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeScalarAabb3 {
    UpeScalarVec3 min;
    UpeScalarVec3 max;
} UpeScalarAabb3;

typedef struct UpeScalarSphere {
    UpeScalarVec3 center;
    UpeScalar radius;
} UpeScalarSphere;

typedef struct UpeScalarCapsule {
    UpeScalarVec3 a;
    UpeScalarVec3 b;
    UpeScalar radius;
} UpeScalarCapsule;

UpeScalarAabb3 UpeScalarAabb3_FromMinMax(UpeScalarVec3 min, UpeScalarVec3 max);
UpeScalarAabb3 UpeScalarAabb3_Expand(UpeScalarAabb3 box, UpeScalarVec3 point);
int UpeScalarAabb3_Contains(UpeScalarAabb3 box, UpeScalarVec3 point);
int UpeScalarAabb3_IsValid(UpeScalarAabb3 box);
int UpeScalarAabb3_Overlap(UpeScalarAabb3 a, UpeScalarAabb3 b);
UpeScalarAabb3 UpeScalarAabb3_Union(UpeScalarAabb3 a, UpeScalarAabb3 b);
UpeScalarVec3 UpeScalarAabb3_Center(UpeScalarAabb3 box);

#ifdef __cplusplus
}
#endif
