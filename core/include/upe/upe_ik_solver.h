#ifndef UPE_IK_SOLVER_H
#define UPE_IK_SOLVER_H

#include "upe_math.h"
#include "upe_animation.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*================================================
  INVERSE KINEMATICS SOLVER
  
  Solves bone chain constraints to match end-effector
  target positions. Supports:
  - 2-bone IK (analytical solution - instant)
  - Multi-bone IK (iterative solver - FABRIK)
  - Per-bone rotation limits
  - Configurable precision and iterations
  
  Common uses:
  - Foot IK (ground contact)
  - Hand IK (object interaction)
  - Look-at constraints (head)
  - Partial body follow
================================================*/

/* IK solver instance - uses UpeIKConstraint from upe_animation.h */
typedef struct {
  UpeIKConstraint constraints[16];
    uint32_t constraint_count;
    
    /* Working data for solver */
    UpeVec3 chain_positions[16];    /* Temporary chain positions */
    UpeQuat chain_rotations[16];    /* Temporary chain rotations */
    
    bool enabled;
    uint32_t total_iterations;      /* Cumulative iterations this frame */
    float total_time;               /* Time spent solving (seconds) */
} UpeIKSolver;

/*================================================
  API FUNCTIONS
================================================*/

/* Solver creation */
UpeIKSolver* UpeIKSolver_Create(void);
void UpeIKSolver_Destroy(UpeIKSolver *solver);

/* Constraint management */
uint32_t UpeIKSolver_AddConstraint(UpeIKSolver *solver, const char *name,
                                   UpeIKSolverType solver_type);
void UpeIKSolver_RemoveConstraint(UpeIKSolver *solver, uint32_t constraint_id);
UpeIKConstraint* UpeIKSolver_GetConstraint(UpeIKSolver *solver, uint32_t constraint_id);

/* Chain building */
void UpeIKConstraint_AddBoneLink(UpeIKConstraint *constraint, uint32_t bone_id, float length);
void UpeIKConstraint_SetBoneLimits(UpeIKConstraint *constraint, uint32_t link_index,
                                   float min_x, float max_x,
                                   float min_y, float max_y,
                                   float min_z, float max_z);

/* Target and configuration */
void UpeIKConstraint_SetTarget(UpeIKConstraint *constraint, UpeVec3 position, float radius);
void UpeIKConstraint_SetPoleVector(UpeIKConstraint *constraint, UpeVec3 pole, float weight);
void UpeIKConstraint_SetWeight(UpeIKConstraint *constraint, float weight);
void UpeIKConstraint_SetDamping(UpeIKConstraint *constraint, float damping);
void UpeIKConstraint_SetIterations(UpeIKConstraint *constraint, uint32_t max_iterations);

/* Solving */
void UpeIKSolver_Solve(UpeIKSolver *solver, UpeSkeleton *skeleton, float dt);

/* Results */
bool UpeIKConstraint_GetSolution(UpeIKConstraint *constraint, UpeSkeleton *skeleton);
float UpeIKConstraint_GetLastError(UpeIKConstraint *constraint);

/* Statistics */
uint32_t UpeIKSolver_GetTotalIterations(UpeIKSolver *solver);
float UpeIKSolver_GetTotalTime(UpeIKSolver *solver);

/*================================================
  BUILT-IN PRESETS
================================================*/

/* Create standard foot IK constraint */
UpeIKConstraint* UpeIKSolver_CreateFootIK(UpeIKSolver *solver, 
                                           uint32_t hip_bone, uint32_t knee_bone,
                                           uint32_t foot_bone);

/* Create standard hand IK constraint */
UpeIKConstraint* UpeIKSolver_CreateHandIK(UpeIKSolver *solver,
                                           uint32_t shoulder_bone, uint32_t elbow_bone,
                                           uint32_t hand_bone);

/* Create look-at constraint for head */
UpeIKConstraint* UpeIKSolver_CreateLookAtIK(UpeIKSolver *solver, uint32_t neck_bone);

#ifdef __cplusplus
}
#endif

#endif /* UPE_IK_SOLVER_H */
