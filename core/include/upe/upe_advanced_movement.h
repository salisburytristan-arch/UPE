/*
  UPE Advanced Movement - Terrain-aware character movement
  
  Extends the basic character controller with:
  - Slope climbing and sliding
  - Stair detection and auto-stepping
  - Ledge grabbing and hanging
  - Wall running and climbing
  
  All seamlessly integrated with physics and animation.
*/

#ifndef UPE_ADVANCED_MOVEMENT_H
#define UPE_ADVANCED_MOVEMENT_H

#include "upe_math.h"
#include "upe_character_controller.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
  Types & Enums
============================================================================*/

/* Advanced movement states */
typedef enum {
    UPE_ADVMOVE_STATE_NORMAL = 0,      /* Walking/running normally */
    UPE_ADVMOVE_STATE_SLOPE = 1,       /* On a slope */
    UPE_ADVMOVE_STATE_CLIMBING = 2,    /* Climbing a slope */
    UPE_ADVMOVE_STATE_STAIRS = 3,      /* On stairs */
    UPE_ADVMOVE_STATE_LEDGE_GRAB = 4,  /* Hanging from ledge */
    UPE_ADVMOVE_STATE_WALLRUN = 5      /* Running on wall */
} UpeAdvancedMovementState;

/* Slope climbing state tracking */
typedef struct {
    bool is_on_slope;           /* Currently on any slope */
    float slope_angle;          /* Angle of current slope (degrees, 0-90) */
    UpeVec3 slope_normal;        /* Ground normal vector */
    float slope_speed_factor;   /* Speed multiplier on slope (0-1) */
    bool can_climb;             /* Can climb current slope */
    float climbing_time;        /* Time spent climbing */
} UpeAdvancedSlopeState;

/* Stair detection state */
typedef struct {
    bool stair_detected;        /* Stair edge ahead detected */
    float stair_height;         /* Height of next step (m) */
    UpeVec3 stair_position;      /* Position of stair edge */
    bool auto_stepping;         /* Currently executing step */
    float step_progress;        /* 0.0-1.0 progress of step animation */
} UpeAdvancedStairState;

/* Ledge grab state */
typedef struct {
    bool is_grabbing;           /* Currently hanging from ledge */
    UpeVec3 grab_position;       /* Where character is holding */
    float grab_time;            /* Time spent grabbing (s) */
    bool can_pull_up;           /* Player can pull up from this ledge */
    UpeVec3 ledge_normal;        /* Direction away from ledge */
    float strength_requirement; /* Strength stat needed to climb (0-1) */
} UpeAdvancedLedgeGrabState;

/* Wall running state */
typedef struct {
    bool is_wallrunning;        /* Currently running on wall */
    UpeVec3 wall_normal;         /* Direction of wall surface */
    float wall_run_time;        /* Time spent on wall (s) */
    float wall_speed;           /* Speed along wall (m/s) */
    float max_wall_run_time;    /* Max time before drop (s) */
    bool is_ascending;          /* Running up wall */
} UpeAdvancedWallRunState;

/* Main advanced movement structure */
typedef struct {
    UpeAdvancedMovementState state;
    
    UpeAdvancedSlopeState slope;
    UpeAdvancedStairState stairs;
    UpeAdvancedLedgeGrabState ledge;
    UpeAdvancedWallRunState wallrun;
    
    /* Configuration */
    float max_climbable_slope;      /* Max slope to auto-climb (degrees) */
    float max_slope_speed_hold;     /* Max speed before sliding down (m/s) */
    float slide_friction;           /* Friction while sliding (0-1) */
    
    float step_height;              /* Max auto-step height (m) */
    float step_reach;               /* How far ahead to detect stairs (m) */
    float step_speed;               /* Speed of auto-step animation (m/s) */
    
    float ledge_grab_distance;      /* How far to detect ledges (m) */
    float ledge_grab_strength;      /* Ease of grabbing (0-1) */
    
    float wall_run_min_speed;       /* Min speed to start wallrun (m/s) */
    float wall_run_max_time;        /* Max time on wall (s) */
    float wall_gravity;             /* Gravity while on wall (0-1) */
    
    /* Bridge to character controller */
    UpeCharacterController *controller;
    
} UpeAdvancedMovement;

/*============================================================================
  Lifecycle
============================================================================*/

/**
 * Create an advanced movement system
 * 
 * Wraps and extends a character controller with terrain-aware features
 * 
 * @param controller Existing character controller to enhance
 * @return Allocated advanced movement system
 */
UpeAdvancedMovement* UpeAdvancedMovement_Create(
    UpeCharacterController *controller
);

/**
 * Destroy advanced movement system
 * 
 * NOTE: Does not destroy the underlying controller
 * 
 * @param adv_move Advanced movement system
 */
void UpeAdvancedMovement_Destroy(UpeAdvancedMovement *adv_move);

/*============================================================================
  Slope System
============================================================================*/

/**
 * Update slope detection and handling
 * 
 * Detects if character is on slope, adjusts speed accordingly
 * 
 * @param adv_move Advanced movement system
 * @param world Physics world (for raycasting)
 * @param dt Delta time (s)
 */
void UpeAdvancedMovement_UpdateSlopes(
    UpeAdvancedMovement *adv_move,
    UpeWorld *world,
    float dt
);

/**
 * Get current slope angle
 * 
 * @param adv_move Advanced movement system
 * @return Slope angle in degrees (0 = flat, 90 = vertical)
 */
float UpeAdvancedMovement_GetSlopeAngle(const UpeAdvancedMovement *adv_move);

/**
 * Check if character can climb current slope
 * 
 * @param adv_move Advanced movement system
 * @return True if slope is climbable
 */
bool UpeAdvancedMovement_CanClimbSlope(const UpeAdvancedMovement *adv_move);

/**
 * Enable/disable slope sliding
 * 
 * @param adv_move Advanced movement system
 * @param enable Allow sliding down slopes
 */
void UpeAdvancedMovement_SetSlopeSliding(
    UpeAdvancedMovement *adv_move,
    bool enable
);

/*============================================================================
  Stair System
============================================================================*/

/**
 * Update stair detection and auto-stepping
 * 
 * Detects stairs ahead and automatically steps over them
 * 
 * @param adv_move Advanced movement system
 * @param world Physics world (for raycasting)
 * @param dt Delta time (s)
 */
void UpeAdvancedMovement_UpdateStairs(
    UpeAdvancedMovement *adv_move,
    UpeWorld *world,
    float dt
);

/**
 * Check if stairs are detected ahead
 * 
 * @param adv_move Advanced movement system
 * @return True if stair edge detected
 */
bool UpeAdvancedMovement_StairsDetected(const UpeAdvancedMovement *adv_move);

/**
 * Get height of next stair step
 * 
 * @param adv_move Advanced movement system
 * @return Height in meters
 */
float UpeAdvancedMovement_GetStairHeight(const UpeAdvancedMovement *adv_move);

/**
 * Enable/disable auto-stepping
 * 
 * @param adv_move Advanced movement system
 * @param enable Automatically step over detected stairs
 */
void UpeAdvancedMovement_SetAutoStepping(
    UpeAdvancedMovement *adv_move,
    bool enable
);

/*============================================================================
  Ledge Grab System
============================================================================*/

/**
 * Update ledge detection and grabbing
 * 
 * Detects overhangs/ledges character can grab
 * 
 * @param adv_move Advanced movement system
 * @param world Physics world (for raycasting)
 * @param dt Delta time (s)
 */
void UpeAdvancedMovement_UpdateLedges(
    UpeAdvancedMovement *adv_move,
    UpeWorld *world,
    float dt
);

/**
 * Request to grab a ledge
 * 
 * Called when player attempts to grab (e.g., near wall while falling)
 * 
 * @param adv_move Advanced movement system
 */
void UpeAdvancedMovement_RequestLedgeGrab(UpeAdvancedMovement *adv_move);

/**
 * Check if currently holding a ledge
 * 
 * @param adv_move Advanced movement system
 * @return True if hanging from ledge
 */
bool UpeAdvancedMovement_IsGrabbingLedge(const UpeAdvancedMovement *adv_move);

/**
 * Release from ledge grab
 * 
 * Character will fall or vault depending on direction
 * 
 * @param adv_move Advanced movement system
 */
void UpeAdvancedMovement_ReleaseLedgeGrab(UpeAdvancedMovement *adv_move);

/**
 * Pull up from ledge (climb to top)
 * 
 * @param adv_move Advanced movement system
 */
void UpeAdvancedMovement_PullUpFromLedge(UpeAdvancedMovement *adv_move);

/*============================================================================
  Wallrun System
============================================================================*/

/**
 * Update wall detection and wallrunning
 * 
 * Detects walls character can run on, maintains wallrun state
 * 
 * @param adv_move Advanced movement system
 * @param world Physics world (for raycasting)
 * @param dt Delta time (s)
 */
void UpeAdvancedMovement_UpdateWallrun(
    UpeAdvancedMovement *adv_move,
    UpeWorld *world,
    float dt
);

/**
 * Check if currently wallrunning
 * 
 * @param adv_move Advanced movement system
 * @return True if running on wall
 */
bool UpeAdvancedMovement_IsWallrunning(const UpeAdvancedMovement *adv_move);

/**
 * Get wallrun time remaining
 * 
 * @param adv_move Advanced movement system
 * @return Time before character drops from wall (s)
 */
float UpeAdvancedMovement_GetWallrunTimeRemaining(
    const UpeAdvancedMovement *adv_move
);

/**
 * Get wall normal (direction away from wall)
 * 
 * @param adv_move Advanced movement system
 * @return Surface normal
 */
UpeVec3 UpeAdvancedMovement_GetWallNormal(const UpeAdvancedMovement *adv_move);

/**
 * Request jump off wall (wall jump)
 * 
 * Launches character perpendicular to wall
 * 
 * @param adv_move Advanced movement system
 * @param force_multiplier Jump force relative to normal (0.5-2.0)
 */
void UpeAdvancedMovement_RequestWallJump(
    UpeAdvancedMovement *adv_move,
    float force_multiplier
);

/*============================================================================
  Integrated Update
============================================================================*/

/**
 * Update all advanced movement systems
 * 
 * Call once per frame before physics step
 * 
 * @param adv_move Advanced movement system
 * @param world Physics world
 * @param dt Delta time (s)
 */
void UpeAdvancedMovement_Update(
    UpeAdvancedMovement *adv_move,
    UpeWorld *world,
    float dt
);

/*============================================================================
  Animation Parameters
============================================================================*/

/**
 * Get advanced animation parameters
 * 
 * Returns blended animation parameters including:
 * - slope_blend: Animation slope adaption (0=flat, 1=steep)
 * - climbing: Are we climbing (0-1)
 * - on_stairs: Are we on stairs (0=no, 1=yes)
 * - ledge_blend: Animation for ledge hanging (0=off, 1=full)
 * - wallrun_blend: Animation for wallrunning (0=off, 1=full)
 * 
 * @param adv_move Advanced movement system
 * @param out_slope_blend Slope animation blend (0-1)
 * @param out_climbing Climbing state (0-1)
 * @param out_on_stairs On stairs flag
 * @param out_ledge_blend Ledge animation (0-1)
 * @param out_wallrun_blend Wallrun animation (0-1)
 */
void UpeAdvancedMovement_GetAnimationParameters(
    const UpeAdvancedMovement *adv_move,
    float *out_slope_blend,
    float *out_climbing,
    bool *out_on_stairs,
    float *out_ledge_blend,
    float *out_wallrun_blend
);

/*============================================================================
  Configuration
============================================================================*/

/**
 * Get default configuration for advanced movement
 * 
 * @return Default config suitable for most games
 */
UpeAdvancedMovement* UpeAdvancedMovement_GetDefaults(
    UpeCharacterController *controller
);

/**
 * Enable/disable all advanced features
 * 
 * @param adv_move Advanced movement system
 * @param enable True = slopes/stairs/ledges/wallrun enabled
 */
void UpeAdvancedMovement_SetAllFeaturesEnabled(
    UpeAdvancedMovement *adv_move,
    bool enable
);

/**
 * Set maximum climbable slope angle
 * 
 * @param adv_move Advanced movement system
 * @param angle_degrees Max slope to auto-climb (0-90)
 */
void UpeAdvancedMovement_SetMaxSlopeAngle(
    UpeAdvancedMovement *adv_move,
    float angle_degrees
);

/**
 * Set maximum auto-step height
 * 
 * @param adv_move Advanced movement system
 * @param height Maximum step height (m)
 */
void UpeAdvancedMovement_SetMaxStepHeight(
    UpeAdvancedMovement *adv_move,
    float height
);

/**
 * Set wallrun duration limit
 * 
 * @param adv_move Advanced movement system
 * @param max_time Maximum time on wall (s)
 */
void UpeAdvancedMovement_SetWallrunDuration(
    UpeAdvancedMovement *adv_move,
    float max_time
);

/*============================================================================
  Debug & State Query
============================================================================*/

/**
 * Get current advanced movement state
 * 
 * @param adv_move Advanced movement system
 * @return Current movement state
 */
UpeAdvancedMovementState UpeAdvancedMovement_GetState(
    const UpeAdvancedMovement *adv_move
);

/**
 * Get state as string for debugging
 * 
 * @param adv_move Advanced movement system
 * @return Allocated debug string (caller must free)
 */
char* UpeAdvancedMovement_GetDebugString(const UpeAdvancedMovement *adv_move);

#ifdef __cplusplus
}
#endif

#endif /* UPE_ADVANCED_MOVEMENT_H */
