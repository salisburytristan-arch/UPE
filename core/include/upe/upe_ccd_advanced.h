#pragma once

#include <stdint.h>
#include "upe/upe_geometry.h"
#include "upe/upe_ccd.h"
#include "upe/upe_gjk.h"

#ifdef __cplusplus
extern "C" {
#endif

// Advanced CCD (note: UpeCcd_BoxBox is already defined in upe_ccd.h)
UpeCcdResult UpeCcd_ConvexConvex(const void *a, UpeGjkSupportFn support_a, UpeVec3 vel_a,
                                 const void *b, UpeGjkSupportFn support_b, UpeVec3 vel_b, float dt);

#ifdef __cplusplus
}
#endif
