#pragma once

#include <stdint.h>
#include "upe/upe_solver.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeSolverStabilization {
    float baumgarte_factor;
    float penetration_slop;
    float max_penetration_correction;
    int position_correction_enabled;
} UpeSolverStabilization;

void UpeSolverStabilization_Init(UpeSolverStabilization *stab);
void UpeSolverStabilization_Apply(const UpeSolverStabilization *stab, 
                                   UpeSolverConstraint *constraint, float dt);

#ifdef __cplusplus
}
#endif
