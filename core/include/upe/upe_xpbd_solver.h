#pragma once

#include <stdint.h>
#include "upe_math.h"
#include "upe_body.h"

#ifdef __cplusplus
extern "C" {
#endif

/* eXtended Position-Based Dynamics (XPBD) Solver
 * Superior constraint quality vs standard PBD through:
 * - Impulse-based velocity corrections before position corrections
 * - Constraint force mixing (compliance) tuning for better stability
 * - Better energy conservation and contact behavior
 */

typedef struct {
    float compliance;           /* Constraint compliance (inverse of stiffness): higher = softer */
    float damping;              /* Damping ratio [0, 1]: critical damping = 1.0 */
    float bias_coefficient;     /* Position error bias correction [0, 1] */
} UpeXpbdConstraintConfig;

typedef struct {
    uint32_t max_iterations;    /* Solver iterations per frame */
    uint32_t max_velocity_iterations; /* Additional velocity correction passes */
    float gravity_scale;        /* Scale gravity in constraint solving */
    int use_warm_start;         /* Use previous frame impulses as initial guess */
    int use_velocity_phase;     /* Separate velocity correction phase */
} UpeXpbdSolverConfig;

typedef struct UpeXpbdSolver UpeXpbdSolver;

/* Create XPBD solver with default config */
UpeXpbdSolver *UpeXpbdSolver_Create(uint32_t max_bodies, uint32_t max_constraints);
void UpeXpbdSolver_Destroy(UpeXpbdSolver *solver);

/* Configure solver properties */
void UpeXpbdSolver_SetConfig(UpeXpbdSolver *solver, const UpeXpbdSolverConfig *config);

/* Add/clear bodies and constraints for frame */
void UpeXpbdSolver_PrepareFrame(UpeXpbdSolver *solver);
void UpeXpbdSolver_AddBody(UpeXpbdSolver *solver, UpeBody *body);
void UpeXpbdSolver_AddContactConstraint(UpeXpbdSolver *solver, 
    uint32_t body_a_idx, uint32_t body_b_idx,
    UpeVec3 contact_point_a, UpeVec3 contact_point_b, UpeVec3 normal, float penetration,
    float friction_static, float friction_dynamic);
void UpeXpbdSolver_AddDistanceConstraint(UpeXpbdSolver *solver,
    uint32_t body_a_idx, uint32_t body_b_idx,
    UpeVec3 anchor_a, UpeVec3 anchor_b, float rest_length,
    const UpeXpbdConstraintConfig *config);
void UpeXpbdSolver_AddBendingConstraint(UpeXpbdSolver *solver,
    uint32_t body_a_idx, uint32_t body_b_idx, uint32_t body_c_idx,
    float target_angle, const UpeXpbdConstraintConfig *config);

/* Solve all constraints for frame */
void UpeXpbdSolver_Solve(UpeXpbdSolver *solver, float dt);

/* Query solved velocities */
UpeVec3 UpeXpbdSolver_GetVelocity(UpeXpbdSolver *solver, uint32_t body_idx);
UpeVec3 UpeXpbdSolver_GetAngularVelocity(UpeXpbdSolver *solver, uint32_t body_idx);

/* Constraint compliance presets for common configurations */
typedef enum {
    UPE_COMPLIANCE_RIGID = 0,           /* Very stiff (concrete contacts) */
    UPE_COMPLIANCE_STIFF = 1,           /* Stiff (metal/stone) */
    UPE_COMPLIANCE_NORMAL = 2,          /* Medium compliance (rubber/wood) */
    UPE_COMPLIANCE_SOFT = 3,            /* Soft (cloth/foam) */
    UPE_COMPLIANCE_VERY_SOFT = 4,       /* Very soft (stretchy material) */
} UpeCompliancePreset;

UpeXpbdConstraintConfig UpeXpbdConstraintConfig_GetPreset(UpeCompliancePreset preset);

/* Statistics for debugging and optimization */
typedef struct {
    uint32_t total_constraints;
    uint32_t contact_constraints;
    uint32_t distance_constraints;
    uint32_t bending_constraints;
    float avg_iterations_to_convergence;
    float residual_energy_change; /* Change in system energy this frame */
} UpeXpbdSolverStats;

void UpeXpbdSolver_GetStats(UpeXpbdSolver *solver, UpeXpbdSolverStats *out_stats);

#ifdef __cplusplus
}
#endif
