#include "upe/upe_procedural_animation.h"
#include "upe/upe_character_controller.h"
#include "upe/upe_world_body_api.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Internal helpers for calculations */
static float upe_proc_lerp(float a, float b, float t) {
    return a + (b - a) * (t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t));
}

static float upe_proc_smoothstep(float t) {
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    return t * t * (3.0f - 2.0f * t);
}

static UpeGaitType upe_proc_determine_gait(float speed) {
    if (speed < 0.1f) return UPE_GAIT_IDLE;
    if (speed < 1.5f) return UPE_GAIT_WALKING;
    if (speed < 3.5f) return UPE_GAIT_JOGGING;
    if (speed < 6.0f) return UPE_GAIT_RUNNING;
    return UPE_GAIT_SPRINTING;
}

static float upe_proc_get_cadence(UpeProceduralAnimation* proc, UpeGaitType gait) {
    switch (gait) {
        case UPE_GAIT_IDLE: return 0.0f;
        case UPE_GAIT_WALKING: return proc->config.cadence_walk;
        case UPE_GAIT_JOGGING: return proc->config.cadence_walk * 1.3f;
        case UPE_GAIT_RUNNING: return proc->config.cadence_run;
        case UPE_GAIT_SPRINTING: return proc->config.cadence_run * 1.25f;
        default: return 0.0f;
    }
}

static float upe_proc_get_step_height(UpeProceduralAnimation* proc, UpeGaitType gait) {
    switch (gait) {
        case UPE_GAIT_IDLE: return 0.0f;
        case UPE_GAIT_WALKING: return proc->config.step_height_walk;
        case UPE_GAIT_JOGGING: return proc->config.step_height_walk * 1.2f;
        case UPE_GAIT_RUNNING: return proc->config.step_height_run;
        case UPE_GAIT_SPRINTING: return proc->config.step_height_run * 1.15f;
        default: return 0.0f;
    }
}

static float upe_proc_adapt_stride(float desired_speed, float stride_min, float stride_max) {
    /* Stride length increases with speed up to a maximum */
    if (desired_speed < 1.5f) {
        /* Walking: stride length 0.4-0.7m */
        return upe_proc_lerp(stride_min, stride_min + (stride_max - stride_min) * 0.5f, desired_speed / 1.5f);
    } else if (desired_speed < 6.0f) {
        /* Running: stride length 0.7-1.2m */
        float progress = (desired_speed - 1.5f) / 4.5f;
        return upe_proc_lerp(stride_min + (stride_max - stride_min) * 0.5f, stride_max, progress);
    }
    return stride_max;
}

/* Update single leg */
static void upe_proc_update_leg(
    UpeProceduralLegState* leg,
    UpeProceduralAnimation* proc,
    int is_left_leg,
    float dt
) {
    float half_cycle = proc->cycle_duration / 2.0f;
    float phase_in_cycle = fmodf(proc->cycle_time + (is_left_leg ? 0.0f : half_cycle), proc->cycle_duration);
    
    leg->phase_progress = phase_in_cycle / proc->cycle_duration;
    
    /* Determine phase */
    if (leg->phase_progress < 0.1f || leg->phase_progress > 0.9f) {
        leg->phase = UPE_PHASE_DOUBLE_SUPPORT;
    } else if (leg->phase_progress < 0.5f) {
        leg->phase = UPE_PHASE_STANCE;
        leg->on_ground = 1;
    } else {
        leg->phase = UPE_PHASE_SWING;
        leg->on_ground = 0;
    }
    
    /* Update muscle stiffness */
    if (leg->phase == UPE_PHASE_STANCE) {
        leg->muscle_stiffness = 1.0f; /* Maximum tension on ground */
    } else if (leg->phase == UPE_PHASE_SWING) {
        leg->muscle_stiffness = 0.5f; /* Relaxed during swing */
    } else {
        leg->muscle_stiffness = 0.7f; /* Preparing for contact */
    }
}

/* Update balance and center of mass */
static void upe_proc_update_balance(UpeProceduralAnimation* proc) {
    /* Weight shifts toward supporting leg */
    if (proc->left_leg.on_ground && !proc->right_leg.on_ground) {
        proc->balance.weight_distribution = upe_proc_lerp(proc->balance.weight_distribution, -0.5f, 0.1f);
    } else if (!proc->left_leg.on_ground && proc->right_leg.on_ground) {
        proc->balance.weight_distribution = upe_proc_lerp(proc->balance.weight_distribution, 0.5f, 0.1f);
    } else {
        proc->balance.weight_distribution = upe_proc_lerp(proc->balance.weight_distribution, 0.0f, 0.1f);
    }
    
    /* Forward lean based on speed */
    float speed = proc->desired_speed;
    if (speed > 1.5f) {
        proc->balance.forward_lean = upe_proc_lerp(0.0f, 15.0f, (speed - 1.5f) / 4.5f);
    } else {
        proc->balance.forward_lean = 0.0f;
    }
    
    /* Balance factor (1.0 = stable, 0.0 = falling) */
    proc->balance.balance_factor = 0.95f; /* Good default stability */
    proc->balance.is_falling = 0;
}

/* Calculate animation parameters */
static void upe_proc_compute_animation_params(UpeProceduralAnimation* proc) {
    float progress = proc->cycle_time / proc->cycle_duration;
    
    /* Stride blend: how extended vs compact the stance is */
    proc->stride_length = upe_proc_adapt_stride(
        proc->desired_speed,
        proc->config.stride_length_min,
        proc->config.stride_length_max
    );
    
    /* Step height adaptation */
    proc->step_height = upe_proc_get_step_height(proc, proc->current_gait);
    
    /* Upper body sway (side-to-side oscillation during running) */
    if (proc->current_gait >= UPE_GAIT_RUNNING) {
        proc->upper_body_sway = sinf(progress * 6.28318f) * 5.0f; /* ~5 degrees max */
    } else {
        proc->upper_body_sway = sinf(progress * 6.28318f) * 2.0f; /* ~2 degrees walking */
    }
    
    /* Hip bounce (vertical motion) */
    float bounce_curve = upe_proc_smoothstep(1.0f - fabsf(2.0f * (progress - 0.5f)));
    proc->hip_bounce = bounce_curve * proc->step_height * 0.5f;
    
    /* Arm swing (opposite legs) */
    proc->arm_swing_factor = sinf(progress * 6.28318f);
    if (proc->current_gait <= UPE_GAIT_WALKING) {
        proc->arm_swing_factor *= 20.0f; /* ~20 degree swing while walking */
    } else {
        proc->arm_swing_factor *= 35.0f; /* ~35 degree swing while running */
    }
}

/* Transition between gaits */
static void upe_proc_update_gait_transition(UpeProceduralAnimation* proc, float dt) {
    UpeGaitType desired = upe_proc_determine_gait(proc->desired_speed);
    
    if (desired != proc->current_gait) {
        proc->target_gait = desired;
        if (proc->gait_transition_progress < 1.0f) {
            proc->gait_transition_progress += dt / 0.5f; /* 0.5s transition */
        } else {
            proc->current_gait = desired;
            proc->gait_transition_progress = 0.0f;
        }
    } else {
        proc->gait_transition_progress = 0.0f;
    }
    
    /* Update cycle duration based on gait */
    float cadence = upe_proc_get_cadence(proc, proc->current_gait);
    if (cadence > 0.0f) {
        proc->cycle_duration = 60.0f / cadence; /* Convert steps/min to seconds */
    } else {
        proc->cycle_duration = 1.0f;
    }
}

/* Create procedural animation */
UpeProceduralAnimation* UpeProceduralAnimation_Create(UpeCharacterController* controller) {
    UpeProceduralAnimation* proc = (UpeProceduralAnimation*)malloc(sizeof(UpeProceduralAnimation));
    if (!proc) return NULL;
    
    memset(proc, 0, sizeof(UpeProceduralAnimation));
    
    proc->controller = controller;
    proc->current_gait = UPE_GAIT_IDLE;
    proc->target_gait = UPE_GAIT_IDLE;
    proc->cycle_time = 0.0f;
    proc->cycle_duration = 1.0f;
    proc->is_active = 1;
    
    /* Default configuration */
    proc->config.idle_pose_blend = 0.5f;
    proc->config.stride_length_min = 0.4f;
    proc->config.stride_length_max = 1.2f;
    proc->config.step_height_walk = 0.15f;
    proc->config.step_height_run = 0.25f;
    proc->config.cadence_walk = 120.0f;
    proc->config.cadence_run = 170.0f;
    proc->config.walking_speed_threshold = 1.5f;
    proc->config.running_speed_threshold = 3.5f;
    proc->config.sprinting_speed_threshold = 6.0f;
    proc->config.balance_weight = 0.7f;
    
    /* Initialize legs */
    proc->left_leg.phase = UPE_PHASE_STANCE;
    proc->right_leg.phase = UPE_PHASE_SWING;
    
    return proc;
}

/* Destroy procedural animation */
void UpeProceduralAnimation_Destroy(UpeProceduralAnimation* proc) {
    if (proc) {
        free(proc);
    }
}

/* Main update function */
void UpeProceduralAnimation_Update(UpeProceduralAnimation* proc, float dt, UpeWorld* world) {
    if (!proc || !proc->is_active) return;
    
    proc->frame_time = dt;
    
    /* Update gait transition */
    upe_proc_update_gait_transition(proc, dt);
    
    /* Update legs */
    upe_proc_update_leg(&proc->left_leg, proc, 1, dt);
    upe_proc_update_leg(&proc->right_leg, proc, 0, dt);
    
    /* Update balance */
    upe_proc_update_balance(proc);
    
    /* Compute animation parameters */
    upe_proc_compute_animation_params(proc);
    
    /* Advance cycle timer */
    proc->cycle_time += dt;
    if (proc->cycle_time >= proc->cycle_duration) {
        proc->cycle_time = fmodf(proc->cycle_time, proc->cycle_duration);
    }
}

/* Set desired velocity */
void UpeProceduralAnimation_SetDesiredVelocity(UpeProceduralAnimation* proc, float speed_ms) {
    if (proc) {
        proc->desired_speed = speed_ms < 0.0f ? 0.0f : speed_ms;
        proc->desired_velocity.x = proc->desired_speed;
        proc->desired_velocity.y = 0.0f;
        proc->desired_velocity.z = 0.0f;
    }
}

void UpeProceduralAnimation_SetDesiredVelocityVector(UpeProceduralAnimation* proc, UpeVec3 velocity) {
    if (proc) {
        proc->desired_velocity = velocity;
        float vx = velocity.x;
        float vz = velocity.z;
        proc->desired_speed = sqrtf(vx * vx + vz * vz);
    }
}

/* Gait queries */
UpeGaitType UpeProceduralAnimation_GetCurrentGait(UpeProceduralAnimation* proc) {
    return proc ? proc->current_gait : UPE_GAIT_IDLE;
}

UpeGaitType UpeProceduralAnimation_GetTargetGait(UpeProceduralAnimation* proc) {
    return proc ? proc->target_gait : UPE_GAIT_IDLE;
}

/* Stride and cadence queries */
float UpeProceduralAnimation_GetStrideLength(UpeProceduralAnimation* proc) {
    return proc ? proc->stride_length : 0.0f;
}

float UpeProceduralAnimation_GetStepHeight(UpeProceduralAnimation* proc) {
    return proc ? proc->step_height : 0.0f;
}

float UpeProceduralAnimation_GetCycleProgress(UpeProceduralAnimation* proc) {
    return proc ? (proc->cycle_time / proc->cycle_duration) : 0.0f;
}

float UpeProceduralAnimation_GetCycleDuration(UpeProceduralAnimation* proc) {
    return proc ? proc->cycle_duration : 1.0f;
}

/* Balance queries */
float UpeProceduralAnimation_GetBalanceFactor(UpeProceduralAnimation* proc) {
    return proc ? proc->balance.balance_factor : 0.0f;
}

float UpeProceduralAnimation_GetWeightDistribution(UpeProceduralAnimation* proc) {
    return proc ? proc->balance.weight_distribution : 0.0f;
}

int UpeProceduralAnimation_IsBalanced(UpeProceduralAnimation* proc) {
    return proc ? (proc->balance.balance_factor > 0.7f && !proc->balance.is_falling) : 0;
}

int UpeProceduralAnimation_IsFalling(UpeProceduralAnimation* proc) {
    return proc ? proc->balance.is_falling : 0;
}

/* Animation parameters */
void UpeProceduralAnimation_GetAnimationParameters(
    UpeProceduralAnimation* proc,
    float* out_stride_blend,
    float* out_cycle_progress,
    float* out_upper_body_sway,
    float* out_hip_bounce,
    float* out_arm_swing,
    float* out_lean_angle,
    float* out_balance_blend
) {
    if (!proc) {
        if (out_stride_blend) *out_stride_blend = 0.0f;
        if (out_cycle_progress) *out_cycle_progress = 0.0f;
        if (out_upper_body_sway) *out_upper_body_sway = 0.0f;
        if (out_hip_bounce) *out_hip_bounce = 0.0f;
        if (out_arm_swing) *out_arm_swing = 0.0f;
        if (out_lean_angle) *out_lean_angle = 0.0f;
        if (out_balance_blend) *out_balance_blend = 0.0f;
        return;
    }
    
    if (out_stride_blend) {
        *out_stride_blend = (proc->stride_length - proc->config.stride_length_min) /
                           (proc->config.stride_length_max - proc->config.stride_length_min);
    }
    if (out_cycle_progress) {
        *out_cycle_progress = proc->cycle_time / proc->cycle_duration;
    }
    if (out_upper_body_sway) {
        *out_upper_body_sway = proc->upper_body_sway;
    }
    if (out_hip_bounce) {
        *out_hip_bounce = proc->hip_bounce;
    }
    if (out_arm_swing) {
        *out_arm_swing = proc->arm_swing_factor;
    }
    if (out_lean_angle) {
        *out_lean_angle = proc->balance.forward_lean;
    }
    if (out_balance_blend) {
        *out_balance_blend = proc->balance.is_falling ? 1.0f : (1.0f - proc->balance.balance_factor);
    }
}

/* Leg state queries */
UpeProceduralLegState UpeProceduralAnimation_GetLeftLegState(UpeProceduralAnimation* proc) {
    UpeProceduralLegState empty = {};
    return proc ? proc->left_leg : empty;
}

UpeProceduralLegState UpeProceduralAnimation_GetRightLegState(UpeProceduralAnimation* proc) {
    UpeProceduralLegState empty = {};
    return proc ? proc->right_leg : empty;
}

int UpeProceduralAnimation_IsLeftLegSwinging(UpeProceduralAnimation* proc) {
    return proc ? (proc->left_leg.phase == UPE_PHASE_SWING) : 0;
}

int UpeProceduralAnimation_IsRightLegSwinging(UpeProceduralAnimation* proc) {
    return proc ? (proc->right_leg.phase == UPE_PHASE_SWING) : 0;
}

/* Configuration */
void UpeProceduralAnimation_SetGaitConfiguration(UpeProceduralAnimation* proc, UpeGaitConfiguration config) {
    if (proc) {
        proc->config = config;
    }
}

UpeGaitConfiguration UpeProceduralAnimation_GetGaitConfiguration(UpeProceduralAnimation* proc) {
    UpeGaitConfiguration empty = {};
    return proc ? proc->config : empty;
}

void UpeProceduralAnimation_SetIdlePoseBlend(UpeProceduralAnimation* proc, float blend) {
    if (proc) {
        proc->config.idle_pose_blend = blend < 0.0f ? 0.0f : (blend > 1.0f ? 1.0f : blend);
    }
}

void UpeProceduralAnimation_SetStrideLengthRange(UpeProceduralAnimation* proc, float min, float max) {
    if (proc && min > 0.0f && max > min) {
        proc->config.stride_length_min = min;
        proc->config.stride_length_max = max;
    }
}

void UpeProceduralAnimation_SetCadenceWalking(UpeProceduralAnimation* proc, float steps_per_minute) {
    if (proc && steps_per_minute > 0.0f) {
        proc->config.cadence_walk = steps_per_minute;
    }
}

void UpeProceduralAnimation_SetCadenceRunning(UpeProceduralAnimation* proc, float steps_per_minute) {
    if (proc && steps_per_minute > 0.0f) {
        proc->config.cadence_run = steps_per_minute;
    }
}

/* Debug and visualization */
const char* UpeProceduralAnimation_GetGaitName(UpeGaitType gait) {
    switch (gait) {
        case UPE_GAIT_IDLE: return "IDLE";
        case UPE_GAIT_WALKING: return "WALKING";
        case UPE_GAIT_JOGGING: return "JOGGING";
        case UPE_GAIT_RUNNING: return "RUNNING";
        case UPE_GAIT_SPRINTING: return "SPRINTING";
        default: return "UNKNOWN";
    }
}

static char upe_proc_debug_buff[1024];

const char* UpeProceduralAnimation_GetDebugString(UpeProceduralAnimation* proc) {
    if (!proc) {
        sprintf(upe_proc_debug_buff, "ProceduralAnimation: NULL");
        return upe_proc_debug_buff;
    }
    
    sprintf(upe_proc_debug_buff,
        "Gait: %s | Speed: %.2f m/s | Stride: %.2f m | Step: %.2f m | "
        "Cycle: %.2f/%.2f s | Balance: %.2f | Forward Lean: %.1f°",
        UpeProceduralAnimation_GetGaitName(proc->current_gait),
        proc->desired_speed,
        proc->stride_length,
        proc->step_height,
        proc->cycle_time,
        proc->cycle_duration,
        proc->balance.balance_factor,
        proc->balance.forward_lean
    );
    
    return upe_proc_debug_buff;
}

/* Reset */
void UpeProceduralAnimation_Reset(UpeProceduralAnimation* proc) {
    if (proc) {
        proc->cycle_time = 0.0f;
        proc->current_gait = UPE_GAIT_IDLE;
        proc->target_gait = UPE_GAIT_IDLE;
        proc->desired_speed = 0.0f;
        proc->gait_transition_progress = 0.0f;
        memset(&proc->balance, 0, sizeof(UpeProceduralBalance));
    }
}

void UpeProceduralAnimation_SetEnabled(UpeProceduralAnimation* proc, int enabled) {
    if (proc) {
        proc->is_active = enabled ? 1 : 0;
    }
}

int UpeProceduralAnimation_IsEnabled(UpeProceduralAnimation* proc) {
    return proc ? proc->is_active : 0;
}
