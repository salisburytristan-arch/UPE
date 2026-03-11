#pragma once

#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeFrame {
    UpeVec3 origin;
    UpeVec3 axis_x;
    UpeVec3 axis_y;
    UpeVec3 axis_z;
} UpeFrame;

UpeFrame UpeFrame_Identity();
UpeVec3 UpeFrame_TransformPoint(const UpeFrame *frame, UpeVec3 point);
UpeVec3 UpeFrame_TransformVector(const UpeFrame *frame, UpeVec3 vector);

#ifdef __cplusplus
}
#endif
