#pragma once

#include <stdint.h>
#include "upe/upe_solver.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeWarmStart {
    float *lambda_cache;
    uint32_t constraint_count;
    uint32_t frame;
} UpeWarmStart;

void UpeWarmStart_Init(UpeWarmStart *warm, uint32_t max_constraints);
void UpeWarmStart_Store(UpeWarmStart *warm, const UpeSolverConstraint *constraints, uint32_t count);
void UpeWarmStart_Apply(UpeWarmStart *warm, UpeSolverConstraint *constraints, uint32_t count);
void UpeWarmStart_Destroy(UpeWarmStart *warm);

#ifdef __cplusplus
}
#endif
