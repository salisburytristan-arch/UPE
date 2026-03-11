/**
 * UPE Procedural Animation System
 * 
 * Procedurally generates realistic locomotion animations based on physics parameters,
 * movement state, and terrain conditions. Eliminates need for keyframed animations
 * while maintaining physical realism.
 * 
 * Features:
 * - Automatic gait generation for walking/running
 * - Stride timing and length adaptation
 * - Balance and weight distribution computation
 * - Muscle-based motion synthesis
 * - Terrain-aware pose adaptation
 * - Smooth animation blending
 * 
 * Usage:
 *   UpeProceduralAnimation* proc = UpeProceduralAnimation_Create(character);
 *   UpeProceduralAnimation_SetDesiredVelocity(proc, desired_speed);
 *   UpeProceduralAnimation_Update(proc, dt, world);
 *   float blend = UpeProceduralAnimation_GetAnimationBlend(proc);
 */

#ifndef UPE_PROCEDURAL_ANIMATION_H
#define UPE_PROCEDURAL_ANIMATION_H

#include "upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct UpeCharacterController UpeCharacterController;
typedef struct UpeWorld UpeWorld;

/* Gait types */
typedef enum {
    UPE_GAIT_IDLE = 0,
    UPE_GAIT_WALKING = 1,
    UPE_GAIT_JOGGING = 2,
    UPE_GAIT_RUNNING = 3,
    UPE_GAIT_SPRINTING = 4
} UpeGaitType;

/* Locomotion phase within gait cycle */
typedef enum {
    UPE_PHASE_STANCE = 0,      /* Foot on ground */
    UPE_PHASE_SWING = 1,       /* Foot in air */
    UPE_PHASE_DOUBLE_SUPPORT = 2  /* Both feet on ground (brief moment) */
} UpeLocomotionPhase;

/* Muscle stiffness levels */
typedef enum {
    UPE_MUSCLE_RELAXED = 0,
    UPE_MUSCLE_NORMAL = 1,
    UPE_MUSCLE_TENSE = 2
} UpeMuscleStiffness;

/* Gait configuration */
typedef struct {
    float idle_pose_blend;          /* 0-1: How relaxed the idle pose is */
    float stride_length_min;         /* Minimum stride length (meters) */
    float stride_length_max;         /* Maximum stride length (meters) */
    float step_height_walk;          /* Foot lift height while walking */
    float step_height_run;           /* Foot lift height while running */
    float cadence_walk;              /* Steps per minute (walking) */
    float cadence_run;               /* Steps per minute (running) */
    float walking_speed_threshold;   /* m/s threshold to switch from walk to jog */
    float running_speed_threshold;   /* m/s threshold to switch from jog to run */
    float sprinting_speed_threshold; /* m/s threshold to switch to sprint */
    float balance_weight;            /* 0-1: How much weight dist affects balance */
} UpeGaitConfiguration;

/* Leg state for one leg */
typedef struct {
    UpeVec3 position;               /* Current foot position */
    UpeVec3 target_position;        /* Target position for swing phase */
    float phase_progress;           /* Cycle progress for this leg (0-1) */
    UpeLocomotionPhase phase;       /* Current phase */
    float muscle_stiffness;         /* 0-1: Muscle tension level */
    int on_ground;                  /* Is foot currently on ground */
    UpeVec3 normal;                 /* Floor normal at foot */
} UpeProceduralLegState;

/* Balance and weight distribution */
typedef struct {
    float center_of_mass_offset;     /* Horizontal offset from center (meters) */
    float weight_distribution;       /* 0=left, 0.5=center, 1=right */
    float forward_lean;              /* Degrees forward lean */
    float balance_factor;            /* 0-1: Balance stability */
    int is_falling;                  /* Detected loss of balance */
} UpeProceduralBalance;

/* Main procedural animation state */
typedef struct {
    UpeCharacterController* controller;
    
    /* Gait and movement */
    UpeGaitType current_gait;
    UpeGaitType target_gait;
    float gait_transition_time;
    float gait_transition_progress;
    
    /* Leg states (2 legs for biped) */
    UpeProceduralLegState left_leg;
    UpeProceduralLegState right_leg;
    
    /* Timing */
    float cycle_time;               /* Time into current gait cycle */
    float cycle_duration;           /* Total cycle duration (seconds) */
    
    /* Desired movement */
    UpeVec3 desired_velocity;
    float desired_speed;
    
    /* Balance and stability */
    UpeProceduralBalance balance;
    
    /* Configuration */
    UpeGaitConfiguration config;
    
    /* Animation parameters */
    float stride_length;            /* Current stride length */
    float step_height;              /* Current step height */
    float upper_body_sway;          /* Side-to-side sway amount */
    float hip_bounce;               /* Vertical hip motion */
    float arm_swing_factor;         /* Amplitude of arm swing */
    
    /* Internal state */
    float frame_time;
    int is_active;
} UpeProceduralAnimation;

/* Creation and lifecycle */
UpeProceduralAnimation* UpeProceduralAnimation_Create(UpeCharacterController* controller);
void UpeProceduralAnimation_Destroy(UpeProceduralAnimation* proc);

/* Main update */
void UpeProceduralAnimation_Update(UpeProceduralAnimation* proc, float dt, UpeWorld* world);

/* Gait control */
void UpeProceduralAnimation_SetDesiredVelocity(UpeProceduralAnimation* proc, float speed_ms);
void UpeProceduralAnimation_SetDesiredVelocityVector(UpeProceduralAnimation* proc, UpeVec3 velocity);
UpeGaitType UpeProceduralAnimation_GetCurrentGait(UpeProceduralAnimation* proc);
UpeGaitType UpeProceduralAnimation_GetTargetGait(UpeProceduralAnimation* proc);

/* Stride and cadence */
float UpeProceduralAnimation_GetStrideLength(UpeProceduralAnimation* proc);
float UpeProceduralAnimation_GetStepHeight(UpeProceduralAnimation* proc);
float UpeProceduralAnimation_GetCycleProgress(UpeProceduralAnimation* proc);
float UpeProceduralAnimation_GetCycleDuration(UpeProceduralAnimation* proc);

/* Balance and stability */
float UpeProceduralAnimation_GetBalanceFactor(UpeProceduralAnimation* proc);
float UpeProceduralAnimation_GetWeightDistribution(UpeProceduralAnimation* proc);
int UpeProceduralAnimation_IsBalanced(UpeProceduralAnimation* proc);
int UpeProceduralAnimation_IsFalling(UpeProceduralAnimation* proc);

/* Animation parameters for blending */
void UpeProceduralAnimation_GetAnimationParameters(
    UpeProceduralAnimation* proc,
    float* out_stride_blend,        /* 0=shortest, 1=longest */
    float* out_cycle_progress,      /* 0-1: Where in animation cycle */
    float* out_upper_body_sway,     /* Rotation/sway amount */
    float* out_hip_bounce,          /* Vertical motion */
    float* out_arm_swing,           /* Arm swing amplitude */
    float* out_lean_angle,          /* Forward/back lean */
    float* out_balance_blend        /* 0=stable, 1=falling */
);

/* Leg state queries */
UpeProceduralLegState UpeProceduralAnimation_GetLeftLegState(UpeProceduralAnimation* proc);
UpeProceduralLegState UpeProceduralAnimation_GetRightLegState(UpeProceduralAnimation* proc);
int UpeProceduralAnimation_IsLeftLegSwinging(UpeProceduralAnimation* proc);
int UpeProceduralAnimation_IsRightLegSwinging(UpeProceduralAnimation* proc);

/* Configuration */
void UpeProceduralAnimation_SetGaitConfiguration(UpeProceduralAnimation* proc, UpeGaitConfiguration config);
UpeGaitConfiguration UpeProceduralAnimation_GetGaitConfiguration(UpeProceduralAnimation* proc);
void UpeProceduralAnimation_SetIdlePoseBlend(UpeProceduralAnimation* proc, float blend);
void UpeProceduralAnimation_SetStrideLengthRange(UpeProceduralAnimation* proc, float min, float max);
void UpeProceduralAnimation_SetCadenceWalking(UpeProceduralAnimation* proc, float steps_per_minute);
void UpeProceduralAnimation_SetCadenceRunning(UpeProceduralAnimation* proc, float steps_per_minute);

/* Debug and visualization */
const char* UpeProceduralAnimation_GetDebugString(UpeProceduralAnimation* proc);
const char* UpeProceduralAnimation_GetGaitName(UpeGaitType gait);

/* Reset and reinitialize */
void UpeProceduralAnimation_Reset(UpeProceduralAnimation* proc);
void UpeProceduralAnimation_SetEnabled(UpeProceduralAnimation* proc, int enabled);
int UpeProceduralAnimation_IsEnabled(UpeProceduralAnimation* proc);

#ifdef __cplusplus
}
#endif

#endif
