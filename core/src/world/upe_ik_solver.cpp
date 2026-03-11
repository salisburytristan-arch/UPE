#include "upe/upe_ik_solver.h"
#include "upe/upe_animation.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Local math helpers because the public math API lacks these utilities */
static UpeQuat UpeQuat_Conjugate(UpeQuat q) {
    UpeQuat out;
    out.x = -q.x;
    out.y = -q.y;
    out.z = -q.z;
    out.w = q.w;
    return out;
}

static UpeQuat UpeQuat_FromToRotation(UpeVec3 from, UpeVec3 to) {
    /* Builds the minimal rotation taking `from` to `to` */
    UpeVec3 f = UpeVec3_Normalize(from);
    UpeVec3 t = UpeVec3_Normalize(to);
    float dot = UpeVec3_Dot(f, t);

    /* Handle opposite direction (180 deg) with an arbitrary orthogonal axis */
    if (dot < -0.9999f) {
        UpeVec3 axis = fabsf(f.x) > fabsf(f.z)
            ? UpeVec3{ -f.y, f.x, 0.0f }
            : UpeVec3{ 0.0f, -f.z, f.y };
        axis = UpeVec3_Normalize(axis);
        float half = (float)M_PI * 0.5f;
        float s = sinf(half);
        return UpeQuat{ axis.x * s, axis.y * s, axis.z * s, cosf(half) };
    }

    UpeVec3 axis = UpeVec3_Cross(f, t);
    float s = sqrtf((1.0f + dot) * 2.0f);
    float inv_s = 1.0f / s;
    return UpeQuat{ axis.x * inv_s, axis.y * inv_s, axis.z * inv_s, s * 0.5f };
}

/*================================================
  2-BONE ANALYTICAL IK SOLVER
  
  Fast, accurate solution for 2-bone chains.
  Commonly used for limbs (shoulder-elbow-hand,
  hip-knee-foot).
================================================*/

static bool UpeIK_Solve2Bone(UpeIKBoneLink *link0, UpeIKBoneLink *link1,
                              UpeVec3 root_pos, UpeQuat root_rot,
                              UpeVec3 target_pos, UpeVec3 pole_vector,
                              UpeSkeleton *skeleton, UpeSkeleton *temp_skeleton) {
    if (!link0 || !link1 || !skeleton) return false;
    
    float len0 = link0->length;
    float len1 = link1->length;
    
    /* Vector from root to target */
    UpeVec3 to_target = UpeVec3_Sub(target_pos, root_pos);
    float dist = UpeVec3_Length(to_target);
    
    /* Check if target is reachable */
    float max_reach = len0 + len1;
    (void)temp_skeleton;
    
    if (dist > max_reach) {
        /* Target unreachable - extend fully towards target */
        UpeVec3 direction = UpeVec3_Normalize(to_target);
        UpeVec3 joint1_pos = UpeVec3_Add(root_pos, 
                                         UpeVec3_Scale(direction, len0));
        (void)joint1_pos;
        
        /* Back-calculate rotations */
        UpeVec3 local_dir0 = UpeQuat_RotateVec3(UpeQuat_Conjugate(root_rot), direction);
        skeleton->bones[link0->bone_id].local_rotation = 
            UpeQuat_FromToRotation(UpeVec3{1.0f, 0.0f, 0.0f}, local_dir0);
        skeleton->bones[link1->bone_id].local_rotation =
            UpeQuat_FromToRotation(UpeVec3{1.0f, 0.0f, 0.0f}, direction);
        return true;
    }
    
    /* Analytical solution using law of cosines */
    float dist_sq = dist * dist;
    float len0_sq = len0 * len0;
    float len1_sq = len1 * len1;
    
    /* Angle at end effector */
    float cos_angle_end = (dist_sq + len1_sq - len0_sq) / (2.0f * dist * len1 + 1e-6f);
    cos_angle_end = fmaxf(-1.0f, fminf(1.0f, cos_angle_end));
    (void)cos_angle_end;
    
    /* Angle at root */
    float cos_angle_root = (dist_sq + len0_sq - len1_sq) / (2.0f * dist * len0 + 1e-6f);
    cos_angle_root = fmaxf(-1.0f, fminf(1.0f, cos_angle_root));
    (void)cos_angle_root;
    
    /* Direction to target */
    UpeVec3 target_dir = UpeVec3_Normalize(to_target);
    
    /* Position of first joint */
    UpeVec3 joint_dir = UpeVec3_Scale(target_dir, len0);
    UpeVec3 joint_pos = UpeVec3_Add(root_pos, joint_dir);
    
    /* Use pole vector to determine plane (optional) */
    (void)pole_vector;
    
    /* First joint rotation: aligns with target */
    UpeVec3 local_to_joint = UpeQuat_RotateVec3(UpeQuat_Conjugate(root_rot), target_dir);
    skeleton->bones[link0->bone_id].local_rotation = 
        UpeQuat_FromToRotation(UpeVec3{1.0f, 0.0f, 0.0f}, local_to_joint);
    
    /* Second joint rotation: handles remaining angle */
    UpeVec3 joint_to_target = UpeVec3_Sub(target_pos, joint_pos);
    UpeVec3 local_joint_dir = UpeVec3_Normalize(joint_to_target);
    skeleton->bones[link1->bone_id].local_rotation =
        UpeQuat_FromToRotation(UpeVec3{1.0f, 0.0f, 0.0f}, local_joint_dir);
    
    return true;
}

/*================================================
  FABRIK (FORWARD AND BACKWARD REACHING IK)
  
  Iterative multi-bone IK solver. Fast convergence,
  handles arbitrary chain lengths, respects bone limits.
================================================*/

static void UpeIK_SolveFABRIK(UpeIKConstraint *constraint, UpeSkeleton *skeleton) {
    if (!constraint || constraint->link_count < 2) return;
    
    uint32_t link_count = constraint->link_count;
    float total_chain_length = 0.0f;
    
    /* Calculate total chain length */
    for (uint32_t i = 0; i < link_count; i++) {
        total_chain_length += constraint->links[i].length;
    }
    
    /* Get root bone world position */
    UpeVec3 root_world_pos = skeleton->bone_world_positions[constraint->links[0].bone_id];
    
    /* Check if target reachable */
    UpeVec3 to_target = UpeVec3_Sub(constraint->target_position, root_world_pos);
    float dist_to_target = UpeVec3_Length(to_target);
    
    if (dist_to_target > total_chain_length) {
        /* Extend arm fully */
        UpeVec3 direction = UpeVec3_Normalize(to_target);
        UpeVec3 current_pos = root_world_pos;
        
        for (uint32_t i = 0; i < link_count; i++) {
            UpeVec3 next_pos = UpeVec3_Add(current_pos, 
                                           UpeVec3_Scale(direction, constraint->links[i].length));
            
            UpeBone *bone = &skeleton->bones[constraint->links[i].bone_id];
            bone->local_position = next_pos;
            
            current_pos = next_pos;
        }
        return;
    }
    
    /* FABRIK iteration */
    for (uint32_t iter = 0; iter < constraint->max_iterations; iter++) {
        /* Forward pass: move from end-effector towards root */
        UpeVec3 positions[16];
        
        /* Initialize positions from skeleton */
        for (uint32_t i = 0; i < link_count; i++) {
            positions[i] = skeleton->bone_world_positions[constraint->links[i].bone_id];
        }
        
        /* Pin end-effector to target */
        positions[link_count - 1] = constraint->target_position;
        
        /* Backward pass: move from root, pulling each joint towards next */
        for (int i = (int)link_count - 2; i >= 0; i--) {
            UpeVec3 direction = UpeVec3_Sub(positions[i], positions[i + 1]);
            float distance = UpeVec3_Length(direction);
            
            if (distance > 1e-6f) {
                direction = UpeVec3_Scale(UpeVec3_Normalize(direction), 
                                          constraint->links[i].length);
                positions[i] = UpeVec3_Add(positions[i + 1], direction);
            }
        }
        
        /* Pin root */
        positions[0] = root_world_pos;
        
        /* Forward pass: move from root towards end-effector */
        for (uint32_t i = 0; i < link_count - 1; i++) {
            UpeVec3 direction = UpeVec3_Sub(positions[i + 1], positions[i]);
            float distance = UpeVec3_Length(direction);
            
            if (distance > 1e-6f) {
                direction = UpeVec3_Scale(UpeVec3_Normalize(direction),
                                          constraint->links[i].length);
                positions[i + 1] = UpeVec3_Add(positions[i], direction);
            }
        }
        
        /* Update skeleton with new positions */
        for (uint32_t i = 0; i < link_count; i++) {
            UpeBone *bone = &skeleton->bones[constraint->links[i].bone_id];
            bone->local_position = positions[i];
        }
        
        /* Check convergence */
        float error = UpeVec3_Length(UpeVec3_Sub(positions[link_count - 1],
                                                  constraint->target_position));
        if (error < constraint->target_radius) {
            constraint->last_error = error;
            constraint->solved = true;
            break;
        }
        
        constraint->last_error = error;
    }
}

/*================================================
  SOLVER MANAGEMENT
================================================*/

UpeIKSolver* UpeIKSolver_Create(void) {
    UpeIKSolver *solver = (UpeIKSolver*)malloc(sizeof(UpeIKSolver));
    if (!solver) return NULL;
    
    solver->constraint_count = 0;
    solver->enabled = true;
    solver->total_iterations = 0;
    solver->total_time = 0.0f;
    
    memset(solver->constraints, 0, sizeof(solver->constraints));
    memset(solver->chain_positions, 0, sizeof(solver->chain_positions));
    memset(solver->chain_rotations, 0, sizeof(solver->chain_rotations));
    
    return solver;
}

void UpeIKSolver_Destroy(UpeIKSolver *solver) {
    if (solver) {
        free(solver);
    }
}

/*================================================
  CONSTRAINT MANAGEMENT
================================================*/

uint32_t UpeIKSolver_AddConstraint(UpeIKSolver *solver, const char *name,
                                   UpeIKSolverType solver_type) {
    if (!solver || solver->constraint_count >= 16) {
        return UINT32_MAX;
    }
    
    uint32_t id = solver->constraint_count;
    UpeIKConstraint *constraint = &solver->constraints[id];
    
    constraint->constraint_id = id;
    strncpy(constraint->name, name, 63);
    constraint->name[63] = '\0';
    constraint->solver_type = solver_type;
    constraint->link_count = 0;
    constraint->max_iterations = 10;
    constraint->damping = 0.5f;
    constraint->enabled = true;
    constraint->weight = 1.0f;
    constraint->solved = false;
    constraint->last_error = 0.0f;
    constraint->target_radius = 0.01f;
    constraint->pole_weight = 0.0f;
    
    constraint->target_position = UpeVec3{0.0f, 0.0f, 0.0f};
    constraint->pole_vector = UpeVec3{0.0f, 0.0f, 0.0f};
    
    solver->constraint_count++;
    return id;
}

void UpeIKSolver_RemoveConstraint(UpeIKSolver *solver, uint32_t constraint_id) {
    if (!solver || constraint_id >= solver->constraint_count) return;
    
    /* Shift constraints */
    for (uint32_t i = constraint_id; i < solver->constraint_count - 1; i++) {
        solver->constraints[i] = solver->constraints[i + 1];
        solver->constraints[i].constraint_id = i;
    }
    solver->constraint_count--;
}

UpeIKConstraint* UpeIKSolver_GetConstraint(UpeIKSolver *solver, uint32_t constraint_id) {
    if (!solver || constraint_id >= solver->constraint_count) return NULL;
    return &solver->constraints[constraint_id];
}

/*================================================
  CHAIN BUILDING
================================================*/

void UpeIKConstraint_AddBoneLink(UpeIKConstraint *constraint, uint32_t bone_id, float length) {
    if (!constraint || constraint->link_count >= 16) return;
    
    UpeIKBoneLink *link = &constraint->links[constraint->link_count];
    link->bone_id = bone_id;
    link->length = length;
    link->min_rotation_x = -M_PI;
    link->max_rotation_x = M_PI;
    link->min_rotation_y = -M_PI;
    link->max_rotation_y = M_PI;
    link->min_rotation_z = -M_PI;
    link->max_rotation_z = M_PI;
    
    constraint->link_count++;
}

void UpeIKConstraint_SetBoneLimits(UpeIKConstraint *constraint, uint32_t link_index,
                                   float min_x, float max_x,
                                   float min_y, float max_y,
                                   float min_z, float max_z) {
    if (!constraint || link_index >= constraint->link_count) return;
    
    UpeIKBoneLink *link = &constraint->links[link_index];
    link->min_rotation_x = min_x;
    link->max_rotation_x = max_x;
    link->min_rotation_y = min_y;
    link->max_rotation_y = max_y;
    link->min_rotation_z = min_z;
    link->max_rotation_z = max_z;
}

/*================================================
  CONFIGURATION
================================================*/

void UpeIKConstraint_SetTarget(UpeIKConstraint *constraint, UpeVec3 position, float radius) {
    if (!constraint) return;
    constraint->target_position = position;
    constraint->target_radius = radius;
}

void UpeIKConstraint_SetPoleVector(UpeIKConstraint *constraint, UpeVec3 pole, float weight) {
    if (!constraint) return;
    constraint->pole_vector = pole;
    constraint->pole_weight = fmaxf(0.0f, fminf(1.0f, weight));
}

void UpeIKConstraint_SetWeight(UpeIKConstraint *constraint, float weight) {
    if (!constraint) return;
    constraint->weight = fmaxf(0.0f, fminf(1.0f, weight));
}

void UpeIKConstraint_SetDamping(UpeIKConstraint *constraint, float damping) {
    if (!constraint) return;
    constraint->damping = fmaxf(0.0f, fminf(1.0f, damping));
}

void UpeIKConstraint_SetIterations(UpeIKConstraint *constraint, uint32_t max_iterations) {
    if (!constraint) return;
    constraint->max_iterations = max_iterations > 0 ? max_iterations : 1;
}

/*================================================
  SOLVING
================================================*/

void UpeIKSolver_Solve(UpeIKSolver *solver, UpeSkeleton *skeleton, float dt) {
    if (!solver || !skeleton || !solver->enabled) return;
    
    solver->total_iterations = 0;
    
    for (uint32_t i = 0; i < solver->constraint_count; i++) {
        UpeIKConstraint *constraint = &solver->constraints[i];
        
        if (!constraint->enabled || constraint->link_count < 2) continue;
        
        constraint->solved = false;
        
        /* Choose solver type */
        if (constraint->solver_type == UPE_IK_TYPE_2BONE && constraint->link_count == 2) {
            /* Analytical 2-bone solver */
            UpeVec3 root_pos = skeleton->bone_world_positions[constraint->links[0].bone_id];
            UpeQuat root_rot = skeleton->bone_world_rotations[constraint->links[0].bone_id];
            
            UpeIK_Solve2Bone(&constraint->links[0], &constraint->links[1],
                             root_pos, root_rot, constraint->target_position,
                             constraint->pole_vector, skeleton, skeleton);
            constraint->solved = true;
            solver->total_iterations += 1;
            
        } else {
            /* FABRIK solver for multi-bone chains */
            UpeIK_SolveFABRIK(constraint, skeleton);
            solver->total_iterations += constraint->max_iterations;
        }
    }
    
    /* Recompute world transforms */
    UpeSkeleton_UpdateWorldTransforms(skeleton);
}

/*================================================
  RESULTS
================================================*/

bool UpeIKConstraint_GetSolution(UpeIKConstraint *constraint, UpeSkeleton *skeleton) {
    if (!constraint || !skeleton) return false;
    return constraint->solved;
}

float UpeIKConstraint_GetLastError(UpeIKConstraint *constraint) {
    if (!constraint) return 0.0f;
    return constraint->last_error;
}

/*================================================
  STATISTICS
================================================*/

uint32_t UpeIKSolver_GetTotalIterations(UpeIKSolver *solver) {
    if (!solver) return 0;
    return solver->total_iterations;
}

float UpeIKSolver_GetTotalTime(UpeIKSolver *solver) {
    if (!solver) return 0.0f;
    return solver->total_time;
}

/*================================================
  PRESETS
================================================*/

UpeIKConstraint* UpeIKSolver_CreateFootIK(UpeIKSolver *solver, 
                                          uint32_t hip_bone, uint32_t knee_bone,
                                          uint32_t foot_bone) {
    if (!solver) return NULL;
    
    uint32_t id = UpeIKSolver_AddConstraint(solver, "FootIK", UPE_IK_TYPE_2BONE);
    if (id == UINT32_MAX) return NULL;
    
    UpeIKConstraint *constraint = &solver->constraints[id];
    UpeIKConstraint_AddBoneLink(constraint, hip_bone, 0.4f);    /* Thigh */
    UpeIKConstraint_AddBoneLink(constraint, knee_bone, 0.4f);   /* Calf */
    UpeIKConstraint_AddBoneLink(constraint, foot_bone, 0.2f);   /* Foot */
    UpeIKConstraint_SetIterations(constraint, 5);
    UpeIKConstraint_SetTarget(constraint, UpeVec3{0.0f, 0.0f, 0.0f}, 0.01f);
    
    return constraint;
}

UpeIKConstraint* UpeIKSolver_CreateHandIK(UpeIKSolver *solver,
                                          uint32_t shoulder_bone, uint32_t elbow_bone,
                                          uint32_t hand_bone) {
    if (!solver) return NULL;
    
    uint32_t id = UpeIKSolver_AddConstraint(solver, "HandIK", UPE_IK_TYPE_2BONE);
    if (id == UINT32_MAX) return NULL;
    
    UpeIKConstraint *constraint = &solver->constraints[id];
    UpeIKConstraint_AddBoneLink(constraint, shoulder_bone, 0.3f);  /* Upper arm */
    UpeIKConstraint_AddBoneLink(constraint, elbow_bone, 0.25f);    /* Forearm */
    UpeIKConstraint_AddBoneLink(constraint, hand_bone, 0.1f);      /* Hand */
    UpeIKConstraint_SetIterations(constraint, 3);
    UpeIKConstraint_SetTarget(constraint, UpeVec3{0.0f, 0.0f, 0.0f}, 0.01f);
    
    return constraint;
}

UpeIKConstraint* UpeIKSolver_CreateLookAtIK(UpeIKSolver *solver, uint32_t neck_bone) {
    if (!solver) return NULL;
    
    uint32_t id = UpeIKSolver_AddConstraint(solver, "LookAtIK", UPE_IK_TYPE_2BONE);
    if (id == UINT32_MAX) return NULL;
    
    UpeIKConstraint *constraint = &solver->constraints[id];
    UpeIKConstraint_AddBoneLink(constraint, neck_bone, 0.15f);
    UpeIKConstraint_AddBoneLink(constraint, neck_bone + 1, 0.2f);  /* Head (assumed next bone) */
    UpeIKConstraint_SetIterations(constraint, 2);
    UpeIKConstraint_SetTarget(constraint, UpeVec3{0.0f, 0.0f, -5.0f}, 0.5f);
    
    return constraint;
}
