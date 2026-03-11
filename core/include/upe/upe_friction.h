#pragma once

#include <stdint.h>
#include "upe/upe_solver.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeFrictionConstraint {
    uint32_t body_a;
    uint32_t body_b;
    UpeVec3 tangent;
    float lambda;
    float friction_coeff;
} UpeFrictionConstraint;

void UpeFriction_Solve(UpeBodyState *bodies, uint32_t body_count,
                       UpeFrictionConstraint *constraints, uint32_t constraint_count);

#ifdef __cplusplus
}
#endif
