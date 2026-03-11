#pragma once

#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeTransform2 {
    UpeVec2 position;
    float rotation;
} UpeTransform2;

typedef struct UpeTransform3 {
    UpeVec3 position;
    UpeQuat rotation;
} UpeTransform3;

UpeTransform2 UpeTransform2_Identity();
UpeTransform2 UpeTransform2_Compose(UpeTransform2 a, UpeTransform2 b);
UpeTransform2 UpeTransform2_Inverse(UpeTransform2 t);
UpeVec2 UpeTransform2_TransformPoint(UpeTransform2 t, UpeVec2 p);

UpeTransform3 UpeTransform3_Identity();
UpeTransform3 UpeTransform3_Compose(UpeTransform3 a, UpeTransform3 b);
UpeTransform3 UpeTransform3_Inverse(UpeTransform3 t);
UpeVec3 UpeTransform3_TransformPoint(UpeTransform3 t, UpeVec3 p);

#ifdef __cplusplus
}
#endif
