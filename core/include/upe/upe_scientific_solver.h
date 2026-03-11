#ifndef UPE_SCIENTIFIC_SOLVER_H
#define UPE_SCIENTIFIC_SOLVER_H

#include "upe_math.h"
#include <stdint.h>

// ============================================================================
// SCIENTIFIC SOLVER HEADER
//
// - High-accuracy implicit time integration
// - Energy/momentum/angular momentum conservation tracking
// - Adaptive time stepping with error estimation
// - Physics constraint satisfaction (A-stability)
// ============================================================================

// Function pointer for acceleration computation
typedef UpeVec3 (*AccelerationFunc)(UpeVec3 pos, UpeVec3 vel, float mass, void *user_data);

// Conservation metrics tracking structure
typedef struct UpeScientificConservationMetrics {
    float energy_error;              // Relative energy error ΔE/E0
    float momentum_error;            // Relative momentum error Δp/p0
    float angular_momentum_error;    // Relative angular momentum error ΔL/L0
    float max_error;                 // Maximum of three errors
} UpeScientificConservationMetrics;

// Conservation tracking state (internal)
typedef struct ConservationState ConservationState;

// Implicit Euler solver (first-order, A-stable)
void UpeScientific_StepImplicitEuler(
    UpeVec3 *pos, UpeVec3 *vel, float mass, float dt,
    AccelerationFunc accel_func, void *user_data,
    float damping);

// Conservation tracking
void UpeScientific_InitConservationTracking(
    ConservationState *state,
    const UpeVec3 *positions, const UpeVec3 *velocities, const float *masses,
    uint32_t num_particles);

void UpeScientific_UpdateConservationTracking(
    ConservationState *state,
    const UpeVec3 *positions, const UpeVec3 *velocities, const float *masses,
    uint32_t num_particles,
    UpeScientificConservationMetrics *out_metrics);

// Adaptive time stepping
float UpeScientific_AdaptiveTimestep(
    const UpeVec3 *pos_euler, const UpeVec3 *pos_implicit,
    float current_dt, float target_error);

// Physics constraint satisfaction
void UpeScientific_SatisfyContraints(
    UpeVec3 *velocity, float velocity_max);

UpeVec3 UpeScientific_ComputeAcceleration(
    const UpeVec3 *vel_new, const UpeVec3 *vel_old, float dt);

void UpeScientific_ProjectConstraints(
    UpeVec3 *positions, uint32_t num_particles,
    const float *masses, const ConservationState *conservation,
    int max_iterations);

#endif // UPE_SCIENTIFIC_SOLVER_H
