#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Articulation & Character Physics
  
  - Ragdoll physics
  - Inverse Kinematics (IK)
  - Biomechanics (muscle forces, tendons)
  - Procedural locomotion
*/

#define UPE_ARTICULATION_MAX_BONES 128

typedef struct UpeArticulatedBone {
    UpeVec3 position;
    UpeQuat rotation;
    UpeVec3 local_offset;        // Offset from parent
    uint32_t parent_index;       // Index of parent bone (-1 for root)
    float length;
    float mass;
    UpeVec3 inertia;
} UpeArticulatedBone;

typedef struct UpeArticulatedJoint {
    uint32_t bone_a;
    uint32_t bone_b;
    
    // Joint limits (cone-twist)
    float swing_limit_y;         // Radians
    float swing_limit_z;
    float twist_limit;
    
    float stiffness;
    float damping;
    
    uint32_t joint_type;         // 0=ball, 1=hinge, 2=cone-twist
} UpeArticulatedJoint;

typedef struct UpeArticulation {
    UpeArticulatedBone *bones;
    uint32_t bone_count;
    
    UpeArticulatedJoint *joints;
    uint32_t joint_count;
    
    UpeVec3 gravity;
    
    // For IK
    uint32_t *ik_chain;          // Bone indices in IK chain
    uint32_t ik_chain_length;
    UpeVec3 ik_target;
    
} UpeArticulation;

void UpeArticulation_Init(UpeArticulation *artic, uint32_t max_bones);
void UpeArticulation_Destroy(UpeArticulation *artic);

// Bone management
uint32_t UpeArticulation_AddBone(UpeArticulation *artic, UpeVec3 position, float length, uint32_t parent);
void UpeArticulation_AddJoint(UpeArticulation *artic, const UpeArticulatedJoint *joint);

// Forward Kinematics
void UpeArticulation_UpdateTransforms(UpeArticulation *artic);
void UpeArticulation_GetWorldTransform(const UpeArticulation *artic, uint32_t bone_id,
                                      UpeVec3 *out_pos, UpeQuat *out_rot);

// Inverse Kinematics
void UpeArticulation_SetIKTarget(UpeArticulation *artic, const uint32_t *bone_chain, uint32_t chain_length, UpeVec3 target);
void UpeArticulation_SolveIK_CCD(UpeArticulation *artic, uint32_t iterations);  // Cyclic Coordinate Descent
void UpeArticulation_SolveIK_FABRIK(UpeArticulation *artic);                    // Forward-And-Backward Reaching IK

// Ragdoll simulation
void UpeArticulation_Step(UpeArticulation *artic, float dt);
void UpeArticulation_ApplyForces(UpeArticulation *artic);
void UpeArticulation_EnforceJointLimits(UpeArticulation *artic);

// ========================
// Biomechanics
// ========================

typedef struct UpeMuscle {
    uint32_t attachment_bone_a;
    uint32_t attachment_bone_b;
    UpeVec3 attachment_point_a;  // Local space
    UpeVec3 attachment_point_b;
    
    float rest_length;
    float max_force;
    float activation;            // 0..1 (neural input)
    float stiffness;
} UpeMuscle;

typedef struct UpeBiomechanicsSystem {
    UpeArticulation *skeleton;
    UpeMuscle *muscles;
    uint32_t muscle_count;
    uint32_t muscle_capacity;
    
    float fatigue_rate;          // How fast muscles tire
    
} UpeBiomechanicsSystem;

void UpeBiomech_Init(UpeBiomechanicsSystem *bio, UpeArticulation *skeleton, uint32_t max_muscles);
void UpeBiomech_Destroy(UpeBiomechanicsSystem *bio);

uint32_t UpeBiomech_AddMuscle(UpeBiomechanicsSystem *bio, const UpeMuscle *muscle);
void UpeBiomech_SetActivation(UpeBiomechanicsSystem *bio, uint32_t muscle_id, float activation);
void UpeBiomech_ApplyMuscleForces(UpeBiomechanicsSystem *bio);

// ========================
// Procedural Locomotion
// ========================

typedef struct UpeProceduralWalker {
    UpeArticulation *skeleton;
    
    // Leg IK targets
    UpeVec3 *foot_targets;
    uint32_t *leg_chains;        // Bone indices for each leg
    uint32_t leg_count;
    uint32_t bones_per_leg;
    
    // Gait parameters
    float stride_length;
    float step_height;
    float cycle_time;
    float phase;                 // Current phase in walk cycle
    
} UpeProceduralWalker;

void UpeWalker_Init(UpeProceduralWalker *walker, UpeArticulation *skeleton, uint32_t leg_count);
void UpeWalker_Destroy(UpeProceduralWalker *walker);

void UpeWalker_SetGait(UpeProceduralWalker *walker, float stride, float height, float cycle_time);
void UpeWalker_UpdateGait(UpeProceduralWalker *walker, float dt, UpeVec3 direction);
void UpeWalker_PlantFeet(UpeProceduralWalker *walker);  // Ground detection + IK

#ifdef __cplusplus
}
#endif
