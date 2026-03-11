#include "upe/upe_solver_stabilization.h"

void UpeSolverStabilization_Init(UpeSolverStabilization *stab) {
    if (!stab) {
        return;
    }
    stab->baumgarte_factor = 0.2f;
    stab->penetration_slop = 0.01f;
    stab->max_penetration_correction = 0.2f;
    stab->position_correction_enabled = 1;
}

void UpeSolverStabilization_Apply(const UpeSolverStabilization *stab, 
                                   UpeSolverConstraint *constraint, float dt) {
    if (!stab || !constraint || !stab->position_correction_enabled) {
        return;
    }
    
    float corrected_penetration = constraint->penetration - stab->penetration_slop;
    if (corrected_penetration < 0.0f) {
        corrected_penetration = 0.0f;
    }
    
    if (corrected_penetration > stab->max_penetration_correction) {
        corrected_penetration = stab->max_penetration_correction;
    }
    
    float bias = (stab->baumgarte_factor / dt) * corrected_penetration;
    constraint->lambda += bias;
}
