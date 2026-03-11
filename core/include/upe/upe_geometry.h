#pragma once

#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeAabb2 {
    UpeVec2 min;
    UpeVec2 max;
} UpeAabb2;

typedef struct UpeAabb3 {
    UpeVec3 min;
    UpeVec3 max;
} UpeAabb3;

typedef struct UpeSphere {
    UpeVec3 center;
    float radius;
} UpeSphere;

typedef struct UpeCapsule {
    UpeVec3 a;
    UpeVec3 b;
    float radius;
} UpeCapsule;

typedef struct UpeObb {
    UpeVec3 center;
    UpeVec3 half_extents;
    UpeMat3 axes;
} UpeObb;

UpeAabb2 UpeAabb2_FromMinMax(UpeVec2 min, UpeVec2 max);
UpeAabb3 UpeAabb3_FromMinMax(UpeVec3 min, UpeVec3 max);
UpeAabb2 UpeAabb2_Expand(UpeAabb2 box, UpeVec2 point);
UpeAabb3 UpeAabb3_Expand(UpeAabb3 box, UpeVec3 point);
int UpeAabb2_Contains(UpeAabb2 box, UpeVec2 point);
int UpeAabb3_Contains(UpeAabb3 box, UpeVec3 point);
int UpeAabb2_IsValid(UpeAabb2 box);
int UpeAabb3_IsValid(UpeAabb3 box);
int UpeSphere_IsValid(UpeSphere sphere);
int UpeCapsule_IsValid(UpeCapsule capsule);
int UpeObb_IsValid(UpeObb obb);
int UpeAabb2_Overlap(UpeAabb2 a, UpeAabb2 b);
int UpeAabb3_Overlap(UpeAabb3 a, UpeAabb3 b);
UpeAabb3 UpeAabb3_Union(UpeAabb3 a, UpeAabb3 b);
UpeVec3 UpeAabb3_Center(UpeAabb3 box);

#ifdef __cplusplus
}
#endif
