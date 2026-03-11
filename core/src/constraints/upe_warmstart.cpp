#include "upe/upe_warmstart.h"
#include <stdlib.h>
#include <string.h>

void UpeWarmStart_Init(UpeWarmStart *warm, uint32_t max_constraints) {
    if (!warm) {
        return;
    }
    warm->constraint_count = max_constraints;
    warm->frame = 0;
    warm->lambda_cache = (float *)malloc(max_constraints * sizeof(float));
    memset(warm->lambda_cache, 0, max_constraints * sizeof(float));
}

void UpeWarmStart_Store(UpeWarmStart *warm, const UpeSolverConstraint *constraints, uint32_t count) {
    if (!warm || !constraints) {
        return;
    }
    uint32_t store_count = count < warm->constraint_count ? count : warm->constraint_count;
    for (uint32_t i = 0; i < store_count; ++i) {
        warm->lambda_cache[i] = constraints[i].lambda;
    }
    warm->frame++;
}

void UpeWarmStart_Apply(UpeWarmStart *warm, UpeSolverConstraint *constraints, uint32_t count) {
    if (!warm || !constraints) {
        return;
    }
    uint32_t apply_count = count < warm->constraint_count ? count : warm->constraint_count;
    for (uint32_t i = 0; i < apply_count; ++i) {
        constraints[i].lambda = warm->lambda_cache[i] * 0.8f;
    }
}

void UpeWarmStart_Destroy(UpeWarmStart *warm) {
    if (!warm) {
        return;
    }
    free(warm->lambda_cache);
}
