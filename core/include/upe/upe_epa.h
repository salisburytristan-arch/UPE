#pragma once

#include "upe/upe_gjk.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeEpaResult {
    UpeVec3 normal;
    float depth;
} UpeEpaResult;

UpeEpaResult UpeEpa_Penetration(const void *shape_a, UpeGjkSupportFn support_a,
                                const void *shape_b, UpeGjkSupportFn support_b);

#ifdef __cplusplus
}
#endif
