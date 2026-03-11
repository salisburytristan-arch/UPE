/*
  UPE Advanced Movement Implementation
*/

#include "upe/upe_advanced_movement.h"
#include "upe/upe_math.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define UPE_GRAVITY 9.81f
#define UPE_DEG_TO_RAD (3.14159265f / 180.0f)
#define UPE_RAD_TO_DEG (180.0f / 3.14159265f)

/*============================================================================
  Lifecycle
============================================================================*/

UpeAdvancedMovement* UpeAdvancedMovement_Create(
    UpeCharacterController *controller)
{
    if (!controller) return NULL;
    
    UpeAdvancedMovement *adv = (UpeAdvancedMovement *)malloc(sizeof(UpeAdvancedMovement));
    if (!adv) return NULL;
    
    adv->controller = controller;
    adv->state = UPE_ADVMOVE_STATE_NORMAL;
    
    /* Slope config */
    adv->max_climbable_slope = 45.0f;
    adv->max_slope_speed_hold = 2.0f;
    adv->slide_friction = 0.3f;
    memset(&adv->slope, 0, sizeof(UpeAdvancedSlopeState));
    
    /* Stair config */
    adv->step_height = 0.3f;
    adv->step_reach = 0.5f;
    adv->step_speed = 2.0f;
    memset(&adv->stairs, 0, sizeof(UpeAdvancedStairState));
    
    /* Ledge config */
    adv->ledge_grab_distance = 0.5f;
    adv->ledge_grab_strength = 0.8f;
    memset(&adv->ledge, 0, sizeof(UpeAdvancedLedgeGrabState));
    
    /* Wallrun config */
    adv->wall_run_min_speed = 3.0f;
    adv->wall_run_max_time = 2.0f;
    adv->wall_gravity = 0.1f;
    memset(&adv->wallrun, 0, sizeof(UpeAdvancedWallRunState));
    
    return adv;
}

void UpeAdvancedMovement_Destroy(UpeAdvancedMovement *adv_move) {
    if (adv_move) {
        free(adv_move);
    }
}

/*============================================================================
  Slope System
============================================================================*/

void UpeAdvancedMovement_UpdateSlopes(
    UpeAdvancedMovement *adv_move,
    UpeWorld *world,
    float dt)
{
    if (!adv_move || !adv_move->controller) return;
    (void)world;
    (void)dt;
    
    /* Raycast down to detect slope */
    
    /* Simplified slope detection */
    UpeVec3 ground_normal = {0, 1, 0};  /* Default: flat */
    float slope_angle = 0.0f;
    
    /* Compute slope angle from ground normal */
    float dot = ground_normal.y;
    slope_angle = acos(fmax(-1.0f, fmin(1.0f, dot))) * UPE_RAD_TO_DEG;
    
    adv_move->slope.is_on_slope = slope_angle > 5.0f;  /* >5 degree is slope */
    adv_move->slope.slope_angle = slope_angle;
    adv_move->slope.slope_normal = ground_normal;
    adv_move->slope.can_climb = slope_angle <= adv_move->max_climbable_slope;
    
    /* Speed adjustment on slopes */
    if (adv_move->slope.is_on_slope && adv_move->slope.can_climb) {
        /* Reduce speed on uphill, increase on downhill */
        UpeVec3 vel = UpeCharacterController_GetVelocity(adv_move->controller);
        float vel_len = sqrtf(vel.x * vel.x + vel.z * vel.z);
        
        if (vel_len > 0.1f) {
            /* Uphill penalty */
            float uphill_factor = 1.0f - (slope_angle / 90.0f) * 0.5f;
            adv_move->slope.slope_speed_factor = uphill_factor;
        } else {
            adv_move->slope.slope_speed_factor = 1.0f;
        }
        
        adv_move->state = UPE_ADVMOVE_STATE_SLOPE;
    } else if (!UpeCharacterController_IsGrounded(adv_move->controller)) {
        /* In air */
        adv_move->slope.is_on_slope = false;
    }
}

float UpeAdvancedMovement_GetSlopeAngle(const UpeAdvancedMovement *adv_move) {
    return adv_move ? adv_move->slope.slope_angle : 0.0f;
}

bool UpeAdvancedMovement_CanClimbSlope(const UpeAdvancedMovement *adv_move) {
    return adv_move ? adv_move->slope.can_climb : false;
}

void UpeAdvancedMovement_SetSlopeSliding(
    UpeAdvancedMovement *adv_move,
    bool enable) {
    if (adv_move && !enable && adv_move->slope.is_on_slope) {
        /* Lock character to slope */
        adv_move->slope.slope_speed_factor = 0.0f;
    }
}

/*============================================================================
  Stair System
============================================================================*/

void UpeAdvancedMovement_UpdateStairs(
    UpeAdvancedMovement *adv_move,
    UpeWorld *world,
    float dt)
{
    if (!adv_move || !adv_move->controller) return;
    (void)world;
    
    /* Raycast ahead for stair detection */
    UpeVec3 pos = UpeCharacterController_GetPosition(adv_move->controller);
    UpeVec3 vel = UpeCharacterController_GetVelocity(adv_move->controller);
    
    /* Check for stair edge ahead */
    float speed = sqrtf(vel.x * vel.x + vel.z * vel.z);
    
    if (speed > 0.5f && UpeCharacterController_IsGrounded(adv_move->controller)) {
        /* Raycast ahead and down to detect step */
        /* Simplified: assume stair detected if we're moving */
        adv_move->stairs.stair_detected = true;
        adv_move->stairs.stair_height = 0.2f;  /* Standard step height */
        adv_move->stairs.stair_position = pos;
    } else {
        adv_move->stairs.stair_detected = false;
    }
    
    /* Auto-stepping */
    if (adv_move->stairs.auto_stepping) {
        adv_move->stairs.step_progress += dt * adv_move->step_speed;
        
        if (adv_move->stairs.step_progress >= 1.0f) {
            adv_move->stairs.auto_stepping = false;
            adv_move->stairs.step_progress = 0.0f;
        }
    }
}

bool UpeAdvancedMovement_StairsDetected(const UpeAdvancedMovement *adv_move) {
    return adv_move ? adv_move->stairs.stair_detected : false;
}

float UpeAdvancedMovement_GetStairHeight(const UpeAdvancedMovement *adv_move) {
    return adv_move ? adv_move->stairs.stair_height : 0.0f;
}

void UpeAdvancedMovement_SetAutoStepping(
    UpeAdvancedMovement *adv_move,
    bool enable) {
    if (adv_move) {
        adv_move->stairs.auto_stepping = enable;
    }
}

/*============================================================================
  Ledge Grab System
============================================================================*/

void UpeAdvancedMovement_UpdateLedges(
    UpeAdvancedMovement *adv_move,
    UpeWorld *world,
    float dt)
{
    if (!adv_move || !adv_move->controller) return;
    (void)world;
    
    bool in_air = UpeCharacterController_IsInAir(adv_move->controller);
    
    if (in_air && adv_move->ledge.is_grabbing) {
        adv_move->ledge.grab_time += dt;
        
        /* Can grab for limited time */
        if (adv_move->ledge.grab_time > 10.0f) {
            adv_move->ledge.is_grabbing = false;
            adv_move->ledge.grab_time = 0.0f;
        }
        
        if (adv_move->state != UPE_ADVMOVE_STATE_LEDGE_GRAB) {
            adv_move->state = UPE_ADVMOVE_STATE_LEDGE_GRAB;
        }
    }
}

void UpeAdvancedMovement_RequestLedgeGrab(UpeAdvancedMovement *adv_move) {
    if (!adv_move) return;
    
    if (UpeCharacterController_IsInAir(adv_move->controller)) {
        adv_move->ledge.is_grabbing = true;
        adv_move->ledge.grab_time = 0.0f;
        adv_move->ledge.can_pull_up = true;
        adv_move->state = UPE_ADVMOVE_STATE_LEDGE_GRAB;
    }
}

bool UpeAdvancedMovement_IsGrabbingLedge(const UpeAdvancedMovement *adv_move) {
    return adv_move ? adv_move->ledge.is_grabbing : false;
}

void UpeAdvancedMovement_ReleaseLedgeGrab(UpeAdvancedMovement *adv_move) {
    if (adv_move) {
        adv_move->ledge.is_grabbing = false;
        adv_move->ledge.grab_time = 0.0f;
    }
}

void UpeAdvancedMovement_PullUpFromLedge(UpeAdvancedMovement *adv_move) {
    if (!adv_move || !adv_move->ledge.is_grabbing) return;
    
    /* Launch upward to pull to ledge top */
    UpeVec3 impulse = {0, 3.0f, 0};
    UpeCharacterController_ApplyKnockback(adv_move->controller,
        impulse,  /* Upward impulse */
        0.3f);                  /* Duration */
    
    adv_move->ledge.is_grabbing = false;
}

/*============================================================================
  Wallrun System
============================================================================*/

void UpeAdvancedMovement_UpdateWallrun(
    UpeAdvancedMovement *adv_move,
    UpeWorld *world,
    float dt)
{
    if (!adv_move || !adv_move->controller) return;
    
    UpeVec3 vel = UpeCharacterController_GetVelocity(adv_move->controller);
    float speed = sqrtf(vel.x * vel.x + vel.z * vel.z);
    bool in_air = UpeCharacterController_IsInAir(adv_move->controller);
    
    /* Check for wall proximity while in air */
    if (in_air && speed > adv_move->wall_run_min_speed) {
        /* Simplified: assume wall detected if moving fast in air */
        adv_move->wallrun.is_wallrunning = true;
        adv_move->wallrun.wall_run_time += dt;
        UpeVec3 wall_dir = {1, 0, 0};
        adv_move->wallrun.wall_normal = wall_dir;  /* Wall to right */
        adv_move->wallrun.is_ascending = vel.y > 0.1f;
        adv_move->state = UPE_ADVMOVE_STATE_WALLRUN;
        
        /* Time limit */
        if (adv_move->wallrun.wall_run_time >= adv_move->wall_run_max_time) {
            adv_move->wallrun.is_wallrunning = false;
        }
    } else if (adv_move->wallrun.is_wallrunning && !in_air) {
        /* Left wall */
        adv_move->wallrun.is_wallrunning = false;
        adv_move->wallrun.wall_run_time = 0.0f;
    }
    
    /* Reduce gravity while wallrunning */
    if (adv_move->wallrun.is_wallrunning) {
        /* Gravity reduction handled in character controller */
    }
}

bool UpeAdvancedMovement_IsWallrunning(const UpeAdvancedMovement *adv_move) {
    return adv_move ? adv_move->wallrun.is_wallrunning : false;
}

float UpeAdvancedMovement_GetWallrunTimeRemaining(
    const UpeAdvancedMovement *adv_move)
{
    if (!adv_move) return 0.0f;
    return adv_move->wall_run_max_time - adv_move->wallrun.wall_run_time;
}

UpeVec3 UpeAdvancedMovement_GetWallNormal(const UpeAdvancedMovement *adv_move) {
    if (!adv_move) {
        UpeVec3 zero = {0, 0, 0};
        return zero;
    }
    return adv_move->wallrun.wall_normal;
}

void UpeAdvancedMovement_RequestWallJump(
    UpeAdvancedMovement *adv_move,
    float force_multiplier)
{
    if (!adv_move || !adv_move->wallrun.is_wallrunning) return;
    
    /* Jump perpendicular to wall */
    UpeVec3 jump_dir = adv_move->wallrun.wall_normal;
    jump_dir.x *= force_multiplier;
    jump_dir.y = 1.0f;  /* Upward component */
    
    UpeCharacterController_ApplyKnockback(adv_move->controller, jump_dir, 0.2f);
    
    adv_move->wallrun.is_wallrunning = false;
    adv_move->wallrun.wall_run_time = 0.0f;
}

/*============================================================================
  Integrated Update
============================================================================*/

void UpeAdvancedMovement_Update(
    UpeAdvancedMovement *adv_move,
    UpeWorld *world,
    float dt)
{
    if (!adv_move) return;
    
    UpeAdvancedMovement_UpdateSlopes(adv_move, world, dt);
    UpeAdvancedMovement_UpdateStairs(adv_move, world, dt);
    UpeAdvancedMovement_UpdateLedges(adv_move, world, dt);
    UpeAdvancedMovement_UpdateWallrun(adv_move, world, dt);
}

/*============================================================================
  Animation Parameters
============================================================================*/

void UpeAdvancedMovement_GetAnimationParameters(
    const UpeAdvancedMovement *adv_move,
    float *out_slope_blend,
    float *out_climbing,
    bool *out_on_stairs,
    float *out_ledge_blend,
    float *out_wallrun_blend)
{
    if (!adv_move) return;
    
    if (out_slope_blend) {
        *out_slope_blend = adv_move->slope.is_on_slope ? 
            (adv_move->slope.slope_angle / adv_move->max_climbable_slope) : 0.0f;
    }
    
    if (out_climbing) {
        *out_climbing = adv_move->slope.is_on_slope && adv_move->slope.can_climb ? 1.0f : 0.0f;
    }
    
    if (out_on_stairs) {
        *out_on_stairs = adv_move->stairs.stair_detected;
    }
    
    if (out_ledge_blend) {
        *out_ledge_blend = adv_move->ledge.is_grabbing ? 1.0f : 0.0f;
    }
    
    if (out_wallrun_blend) {
        *out_wallrun_blend = adv_move->wallrun.is_wallrunning ? 
            (1.0f - (adv_move->wallrun.wall_run_time / adv_move->wall_run_max_time)) : 0.0f;
    }
}

/*============================================================================
  Configuration
============================================================================*/

UpeAdvancedMovement* UpeAdvancedMovement_GetDefaults(
    UpeCharacterController *controller)
{
    return UpeAdvancedMovement_Create(controller);
}

void UpeAdvancedMovement_SetAllFeaturesEnabled(
    UpeAdvancedMovement *adv_move,
    bool enable)
{
    if (!adv_move) return;
    
    if (enable) {
        adv_move->max_climbable_slope = 45.0f;
        adv_move->step_height = 0.3f;
        adv_move->wall_run_max_time = 2.0f;
    } else {
        adv_move->max_climbable_slope = 0.0f;
        adv_move->step_height = 0.0f;
        adv_move->wall_run_max_time = 0.0f;
    }
}

void UpeAdvancedMovement_SetMaxSlopeAngle(
    UpeAdvancedMovement *adv_move,
    float angle_degrees)
{
    if (adv_move) {
        adv_move->max_climbable_slope = fmax(0.0f, fmin(90.0f, angle_degrees));
    }
}

void UpeAdvancedMovement_SetMaxStepHeight(
    UpeAdvancedMovement *adv_move,
    float height)
{
    if (adv_move) {
        adv_move->step_height = fmax(0.0f, fmin(2.0f, height));
    }
}

void UpeAdvancedMovement_SetWallrunDuration(
    UpeAdvancedMovement *adv_move,
    float max_time)
{
    if (adv_move) {
        adv_move->wall_run_max_time = fmax(0.0f, max_time);
    }
}

/*============================================================================
  Debug & State Query
============================================================================*/

UpeAdvancedMovementState UpeAdvancedMovement_GetState(
    const UpeAdvancedMovement *adv_move)
{
    return adv_move ? adv_move->state : UPE_ADVMOVE_STATE_NORMAL;
}

char* UpeAdvancedMovement_GetDebugString(const UpeAdvancedMovement *adv_move) {
    if (!adv_move) return NULL;
    
    char *str = (char *)malloc(512);
    if (!str) return NULL;
    
    const char *state_str = "";
    switch (adv_move->state) {
        case UPE_ADVMOVE_STATE_NORMAL:
            state_str = "NORMAL";
            break;
        case UPE_ADVMOVE_STATE_SLOPE:
            state_str = "SLOPE";
            break;
        case UPE_ADVMOVE_STATE_CLIMBING:
            state_str = "CLIMBING";
            break;
        case UPE_ADVMOVE_STATE_STAIRS:
            state_str = "STAIRS";
            break;
        case UPE_ADVMOVE_STATE_LEDGE_GRAB:
            state_str = "LEDGE_GRAB";
            break;
        case UPE_ADVMOVE_STATE_WALLRUN:
            state_str = "WALLRUN";
            break;
    }
    
    snprintf(str, 512,
             "AdvMovement: %s\n"
             "Slope: %.1f° (can climb: %s)\n"
             "Stairs: %s (height: %.2f m)\n"
             "Ledge: %s (time: %.2f s)\n"
             "Wallrun: %s (time: %.2f/%.2f s)",
             state_str,
             adv_move->slope.slope_angle,
             adv_move->slope.can_climb ? "YES" : "NO",
             adv_move->stairs.stair_detected ? "YES" : "NO",
             adv_move->stairs.stair_height,
             adv_move->ledge.is_grabbing ? "YES" : "NO",
             adv_move->ledge.grab_time,
             adv_move->wallrun.is_wallrunning ? "YES" : "NO",
             adv_move->wallrun.wall_run_time,
             adv_move->wall_run_max_time);
    
    return str;
}
