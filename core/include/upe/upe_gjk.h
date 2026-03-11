#pragma once

#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeGjkSupport {
    UpeVec3 point;
} UpeGjkSupport;

typedef UpeGjkSupport (*UpeGjkSupportFn)(const void *shape, UpeVec3 dir);

typedef struct UpeGjkResult {
    int intersecting;
    UpeVec3 witness_a;
    UpeVec3 witness_b;
    float distance;
} UpeGjkResult;

UpeGjkResult UpeGjk_Distance(const void *shape_a, UpeGjkSupportFn support_a,
                             const void *shape_b, UpeGjkSupportFn support_b,
                             UpeVec3 initial_dir);

#ifdef __cplusplus
}
#endif
