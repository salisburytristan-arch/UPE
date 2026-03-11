#ifndef UPE_ANIMATION_H
#define UPE_ANIMATION_H

#include "upe_math.h"
#include "upe_body.h"
#include "upe_world_body_api.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration for physics world */
typedef struct UpeWorld UpeWorld;

/*================================================
  SKELETAL ANIMATION BINDING
  
  Enables skeletal animation with physics ragdoll
  blending. Each bone maps to a physics body.
  
  Features:
  - Bone hierarchy (parent-child relationships)
  - Animation state machine
  - Physics/animation blending
  - IK constraints (feet, hands)
  - Ragdoll damping control
================================================*/

/* Maximum bones per skeleton */
#define UPE_MAX_BONES 64

/* Maximum animations per controller */
#define UPE_MAX_ANIMATIONS 16

/* Bone flags */
#define UPE_BONE_FLAG_PHYSICAL    0x01  /* Has physics body */
#define UPE_BONE_FLAG_IK_TARGET   0x02  /* IK constraint target */
#define UPE_BONE_FLAG_ROOT        0x04  /* Root bone (world origin) */

/* Animation blend modes */
typedef enum {
    UPE_BLEND_REPLACE = 0,        /* Animation replaces physics completely */
    UPE_BLEND_ADDITIVE = 1,       /* Add animation delta to physics pose */
    UPE_BLEND_PHYSICS = 2,        /* Physics has priority (ragdoll) */
    UPE_BLEND_INTERPOLATED = 3    /* Smooth blend between physics/animation */
} UpeBlendMode;

/* Animation play states */
typedef enum {
    UPE_ANIM_STATE_STOPPED = 0,
    UPE_ANIM_STATE_PLAYING = 1,
    UPE_ANIM_STATE_PAUSED = 2,
    UPE_ANIM_STATE_LOOPING = 3
} UpeAnimationState;

/* IK solver types (shared with IK solver) */
typedef enum {
  UPE_IK_TYPE_2BONE = 0,      /* Analytical 2-bone solver (fast) */
  UPE_IK_TYPE_FABRIK = 1,     /* Iterative multi-bone FABRIK */
  UPE_IK_TYPE_CCDIK = 2       /* Cyclic Coordinate Descent IK */
} UpeIKSolverType;

/*================================================
  KEYFRAME ANIMATION DATA
================================================*/

/* Single keyframe (rotation + translation) */
typedef struct {
    float time;                   /* Time in animation (seconds) */
    UpeQuat rotation;             /* Bone rotation */
    UpeVec3 translation;          /* Bone translation relative to parent */
} UpeKeyframe;

/* Animation track for single bone */
typedef struct {
    uint32_t bone_id;
    UpeKeyframe *keyframes;
    uint32_t keyframe_count;
    float duration;               /* Total animation length (seconds) */
} UpeAnimationTrack;

/* Complete animation clip */
typedef struct {
    char name[64];
    UpeAnimationTrack *tracks;
    uint32_t track_count;
    float duration;               /* Total animation duration (seconds) */
    uint32_t flags;               /* Animation flags (looping, etc.) */
} UpeAnimationClip;

/*================================================
  BONE HIERARCHY
================================================*/

/* Single bone in skeleton */
typedef struct {
    uint32_t bone_id;
    char name[64];
    uint32_t parent_id;           /* Parent bone ID (UINT32_MAX = root) */
    uint32_t body_id;             /* Physics body ID (UINT32_MAX = none) */
    
    /* Local transform (relative to parent) */
    UpeQuat local_rotation;
    UpeVec3 local_position;
    
    /* Bone metadata */
    float length;                 /* Bone length (for IK) */
    uint32_t flags;               /* UPE_BONE_FLAG_* */
    
    /* Animation data */
    float blend_weight;           /* 0.0 = physics only, 1.0 = animation only */
    UpeBlendMode blend_mode;
    
} UpeBone;

/* Skeleton hierarchy */
typedef struct {
    uint32_t skeleton_id;
    UpeBone bones[UPE_MAX_BONES];
    uint32_t bone_count;
    uint32_t root_bone_id;
    
    /* Cached world transforms */
    UpeVec3 bone_world_positions[UPE_MAX_BONES];
    UpeQuat bone_world_rotations[UPE_MAX_BONES];
    
} UpeSkeleton;

/*================================================
  ANIMATION CONTROLLER
================================================*/

/* Animation playback state */
typedef struct {
    uint32_t current_animation_id;
    float playback_time;          /* Current position in animation (seconds) */
    UpeAnimationState state;
    float playback_speed;         /* Speed multiplier (1.0 = normal) */
    
    /* Blending */
    uint32_t blend_target_animation;
    float blend_duration;         /* Remaining blend time (seconds) */
    float blend_start_time;
    
    /* Per-bone override */
    float bone_blend_weights[UPE_MAX_BONES];
    
} UpeAnimationPlayback;

/* Animation controller manages playback */
typedef struct {
    uint32_t controller_id;
    UpeSkeleton *skeleton;
    UpeAnimationClip *animations[UPE_MAX_ANIMATIONS];
    uint32_t animation_count;
    
    UpeAnimationPlayback playback;
    
    /* Global parameters */
    float ragdoll_blend;          /* 0.0 = animation, 1.0 = physics ragdoll */
    bool enable_physics_influence;
    
} UpeAnimationController;

/*================================================
  IK CONSTRAINTS
================================================*/

typedef struct {
  uint32_t bone_id;
  float length;
  float min_rotation_x, max_rotation_x;
  float min_rotation_y, max_rotation_y;
  float min_rotation_z, max_rotation_z;
} UpeIKBoneLink;

/* Inverse Kinematics constraint */
typedef struct {
  uint32_t constraint_id;
  char name[64];
  UpeIKSolverType solver_type;
    
  UpeIKBoneLink links[16];
  uint32_t link_count;
    
  UpeVec3 target_position;      /* World space target */
  UpeVec3 pole_vector;          /* Preferred bend direction */
  float target_radius;          /* Allowed error radius */
  float pole_weight;            /* Influence of pole vector */
    
  uint32_t max_iterations;      /* IK solver iterations */
  float damping;                /* Damping for CCD/FABRIK */
  float weight;                 /* 0.0 = off, 1.0 = full */
  bool enabled;
  bool solved;
  float last_error;             /* Error from last solve */
} UpeIKConstraint;

/*================================================
  RAGDOLL SYSTEM
================================================*/

/* Ragdoll pose (all body transforms at once) */
typedef struct {
    UpeVec3 body_positions[UPE_MAX_BONES];
    UpeQuat body_rotations[UPE_MAX_BONES];
    uint32_t body_count;
} UpeRagdollPose;

/*================================================
  API FUNCTIONS
================================================*/

/* Skeleton creation */
UpeSkeleton* UpeSkeleton_Create(uint32_t max_bones);
void UpeSkeleton_Destroy(UpeSkeleton *skeleton);
uint32_t UpeSkeleton_AddBone(UpeSkeleton *skeleton, const char *name, 
                              uint32_t parent_id, float length);

/* Animation clip management */
UpeAnimationClip* UpeAnimationClip_Create(const char *name, float duration);
void UpeAnimationClip_Destroy(UpeAnimationClip *clip);
void UpeAnimationClip_AddTrack(UpeAnimationClip *clip, uint32_t bone_id,
                                const UpeKeyframe *keyframes, uint32_t count);

/* Animation controller */
UpeAnimationController* UpeAnimationController_Create(UpeSkeleton *skeleton);
void UpeAnimationController_Destroy(UpeAnimationController *controller);

/* Playback control */
void UpeAnimationController_PlayAnimation(UpeAnimationController *controller,
                                           UpeAnimationClip *clip, bool loop);
void UpeAnimationController_BlendAnimation(UpeAnimationController *controller,
                                            UpeAnimationClip *target_clip, float duration);
void UpeAnimationController_Stop(UpeAnimationController *controller);
void UpeAnimationController_SetPlaybackSpeed(UpeAnimationController *controller, float speed);

/* Update skeleton from animation */
void UpeAnimationController_Update(UpeAnimationController *controller, float dt,
                                    UpeWorld *physics_world);

/* Blending */
void UpeAnimationController_SetRagdollBlend(UpeAnimationController *controller, float blend);
void UpeAnimationController_SetBoneBoneBlendWeight(UpeAnimationController *controller,
                                                    uint32_t bone_id, float weight);

/* IK constraints */
void UpeAnimationController_AddIKConstraint(UpeAnimationController *controller,
                                             UpeIKConstraint constraint);
void UpeAnimationController_SolveIK(UpeAnimationController *controller);

/* Pose queries */
void UpeAnimationController_GetBoneWorldTransform(UpeAnimationController *controller,
                                                   uint32_t bone_id, UpeVec3 *pos, UpeQuat *rot);
void UpeAnimationController_GetBoneWorldPosition(UpeAnimationController *controller,
                                                  uint32_t bone_id, UpeVec3 *pos);

/* Update world transforms (used by IK solver) */
void UpeSkeleton_UpdateWorldTransforms(UpeSkeleton *skeleton);

/* Ragdoll capture/apply */
void UpeAnimationController_CaptureRagdollPose(UpeAnimationController *controller,
                                                UpeWorld *physics_world, UpeRagdollPose *pose);
void UpeAnimationController_ApplyRagdollPose(UpeAnimationController *controller,
                                              UpeWorld *physics_world, const UpeRagdollPose *pose);

/* Bone binding to physics */
void UpeBone_SetPhysicsBody(UpeBone *bone, uint32_t body_id);
uint32_t UpeBone_GetPhysicsBody(UpeBone *bone);

#ifdef __cplusplus
}
#endif

#endif /* UPE_ANIMATION_H */
