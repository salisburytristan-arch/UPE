/*
  UPE Character Controller - Physics-based character movement and control
  
  Provides a complete character controller system with:
  - Walk/run/sprint movement
  - Jumping with coyote time
  - Air control and falling
  - Slope climbing
  - Stairs stepping
  - Integration with animation system
  - Physics-based collision handling
  
  Designed for action games, RPGs, and third-person/first-person gameplay.
*/

#ifndef UPE_CHARACTER_CONTROLLER_H
#define UPE_CHARACTER_CONTROLLER_H

#include "upe_math.h"
#include "upe_body.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declare types */
typedef struct UpeWorld UpeWorld;
typedef struct UpeCharacter UpeCharacter;

/*============================================================================
  Types & Constants
============================================================================*/

/* Character controller modes */
typedef enum {
    UPE_CHARCTRL_MODE_GROUNDED = 0,   /* On ground, can walk/run */
    UPE_CHARCTRL_MODE_JUMPING = 1,    /* In jump arc */
    UPE_CHARCTRL_MODE_FALLING = 2,    /* Falling (no jump input) */
    UPE_CHARCTRL_MODE_LEDGE_GRAB = 3  /* Hanging from ledge */
} UpeCharacterControllerMode;

/* Movement states (animation-friendly) */
typedef enum {
    UPE_CHARCTRL_LOCOMOTION_IDLE = 0,
    UPE_CHARCTRL_LOCOMOTION_WALKING = 1,
    UPE_CHARCTRL_LOCOMOTION_RUNNING = 2,
    UPE_CHARCTRL_LOCOMOTION_SPRINTING = 3,
    UPE_CHARCTRL_LOCOMOTION_STRAFING = 4
} UpeCharacterLocomotion;

/* Controller configuration */
typedef struct {
    float height;              /* Character capsule height (m) */
    float radius;              /* Character capsule radius (m) */
    float mass;                /* Character mass (kg) */
    
    /* Movement */
    float walk_speed;          /* Walking movement speed (m/s) */
    float run_speed;           /* Running movement speed (m/s) */
    float sprint_speed;        /* Sprint movement speed (m/s) */
    float acceleration;        /* Ground acceleration (m/s²) */
    float deceleration;        /* Ground deceleration (m/s²) */
    
    /* Jumping */
    float jump_power;          /* Jump impulse (m/s) */
    float jump_apex_delay;     /* Time at apex (s) - reduces gravity briefly */
    float coyote_time;         /* Grace period for jump after leaving ground (s) */
    float jump_buffer_time;    /* Input buffering time (s) */
    
    /* Air control */
    float air_acceleration;    /* Air-based acceleration multiplier (0-1) */
    float air_friction;        /* Air-based friction (0-1) */
    float terminal_velocity;   /* Max falling speed (m/s, positive down) */
    
    /* Slope handling */
    float max_slope_angle;     /* Max climbable slope (degrees) */
    float slope_friction;      /* Friction on slopes (0-1) */
    
    /* Stairs */
    float step_height;         /* Max stair step height (m) */
    float step_reach;          /* How far ahead to check for stairs (m) */
    
    /* Collision */
    float ground_check_distance;  /* Raycast distance for ground detection (m) */
    float skin_width;             /* Collision skin width (m) */
} UpeCharacterControllerDesc;

/* Character controller main structure */
typedef struct UpeCharacterController {
    /* Configuration */
    UpeCharacterControllerDesc config;
    
    /* Physics integration */
    uint32_t body_id;              /* Physics rigid body ID */
    UpeVec3 root_position;          /* Root position (pelvis for animation) */
    UpeQuat root_rotation;           /* Root rotation */
    
    /* Movement state */
    UpeCharacterControllerMode mode;
    UpeCharacterLocomotion locomotion;
    UpeVec3 velocity;               /* Current velocity (m/s) */
    UpeVec3 desired_velocity;       /* Target velocity for frame */
    
    /* Jump tracking */
    bool can_jump;                 /* Currently has jump available */
    float coyote_counter;          /* Time since leaving ground (s) */
    float jump_buffer_counter;     /* Time since jump input (s) */
    bool pending_jump;             /* Jump requested this frame */
    
    /* Air state */
    float air_time;                /* Time spent in air (s) */
    float falling_distance;        /* Distance fallen (m) */
    bool is_falling;               /* True if falling (no jump input) */
    
    /* Slope state */
    UpeVec3 ground_normal;          /* Ground surface normal */
    float slope_angle;             /* Current slope angle (degrees) */
    bool on_slope;                 /* Currently on a slope */
    
    /* Moving platform support */
    uint32_t ground_body_id;       /* ID of body we're standing on (0=static ground) */
    UpeVec3 platform_velocity;     /* Velocity of platform we're on */
    
    /* Animation bridge */
    UpeCharacterLocomotion last_locomotion;  /* For animation transitions */
    float direction_angle;         /* Movement direction vs facing (degrees) */
    float speed_multiplier;        /* Animation speed blend (0-1) */
    
    /* Timers */
    float apex_time;               /* Time at jump apex */
    float ground_contact_time;     /* Time since last ground contact */
} UpeCharacterController;

/*============================================================================
  Controller Lifecycle
============================================================================*/

/**
 * Create a default character controller configuration
 * 
 * Sensible defaults suitable for action games:
 * - Height: 1.8m, radius: 0.3m
 * - Walk: 2 m/s, Run: 5 m/s, Sprint: 8 m/s
 * - Jump power: 5 m/s
 * - Coyote time: 0.1s
 * - Max slope: 45 degrees
 */
UpeCharacterControllerDesc UpeCharacterController_GetDefaultConfig(void);

/**
 * Create a character controller
 * 
 * Creates a physics body (capsule) and initializes controller state
 * 
 * @param world Physics world
 * @param position Initial position (m)
 * @param config Controller configuration
 * @return Allocated controller, must be freed with UpeCharacterController_Destroy
 */
UpeCharacterController* UpeCharacterController_Create(
    UpeWorld *world,
    UpeVec3 position,
    const UpeCharacterControllerDesc *config
);

/**
 * Destroy a character controller
 * 
 * Frees controller memory and associated physics body
 * 
 * @param controller Controller to destroy
 */
void UpeCharacterController_Destroy(UpeCharacterController *controller);

/*============================================================================
  Input & Movement Control
============================================================================*/

/**
 * Set desired movement direction (local to character)
 * 
 * Values should be normalized:
 * - x: strafing (-1 left, 0 neutral, 1 right)
 * - z: forward (-1 backward, 0 neutral, 1 forward)
 * 
 * @param controller Character controller
 * @param direction Desired movement direction (normalized)
 * @param movement_type Walk/Run/Sprint speed to use
 */
void UpeCharacterController_SetMovementInput(
    UpeCharacterController *controller,
    UpeVec2 direction,
    UpeCharacterLocomotion movement_type
);

/**
 * Request a jump
 * 
 * Can be called even when not on ground (buffered for coyote/jump_buffer time)
 * 
 * @param controller Character controller
 */
void UpeCharacterController_RequestJump(UpeCharacterController *controller);

/**
 * Set desired facing direction
 * 
 * Character will gradually turn toward this direction
 * 
 * @param controller Character controller
 * @param facing Direction to face (world space, normalized)
 * @param turn_speed Rotation speed (degrees/s)
 */
void UpeCharacterController_SetFacingDirection(
    UpeCharacterController *controller,
    UpeVec3 facing,
    float turn_speed
);

/**
 * Request external velocity (knockback/push)
 * 
 * Adds to current velocity over time
 * 
 * @param controller Character controller
 * @param impulse Velocity to add (m/s)
 * @param duration Time to apply impulse (s)
 */
void UpeCharacterController_ApplyKnockback(
    UpeCharacterController *controller,
    UpeVec3 impulse,
    float duration
);

/*============================================================================
  Update & Physics Integration
============================================================================*/

/**
 * Update character controller
 * 
 * Called once per physics frame. Handles:
 * - Movement input processing
 * - Ground contact detection
 * - Jump state management
 * - Slope adjustment
 * - Velocity computation
 * 
 * @param controller Character controller
 * @param dt Delta time (s)
 * @param world Physics world (for raycasts)
 */
void UpeCharacterController_Update(
    UpeCharacterController *controller,
    float dt,
    UpeWorld *world
);

/**
 * Apply controller velocity to physics body
 * 
 * Called after UpeCharacterController_Update to sync with physics
 * 
 * @param controller Character controller
 * @param world Physics world
 */
void UpeCharacterController_ApplyVelocity(
    UpeCharacterController *controller,
    UpeWorld *world
);

/*============================================================================
  State Query
============================================================================*/

/**
 * Get current movement state
 * 
 * @param controller Character controller
 * @return Current locomotion state
 */
UpeCharacterLocomotion UpeCharacterController_GetLocomotion(
    const UpeCharacterController *controller
);

/**
 * Check if character is on ground
 * 
 * @param controller Character controller
 * @return True if grounded
 */
bool UpeCharacterController_IsGrounded(const UpeCharacterController *controller);

/**
 * Check if character is in air
 * 
 * @param controller Character controller
 * @return True if falling or jumping
 */
bool UpeCharacterController_IsInAir(const UpeCharacterController *controller);

/**
 * Get current movement velocity
 * 
 * @param controller Character controller
 * @return Velocity vector (m/s)
 */
UpeVec3 UpeCharacterController_GetVelocity(const UpeCharacterController *controller);

/**
 * Get current position
 * 
 * @param controller Character controller
 * @return Position vector (m)
 */
UpeVec3 UpeCharacterController_GetPosition(const UpeCharacterController *controller);

/**
 * Get distance fallen from peak
 * 
 * Useful for fall damage calculations
 * 
 * @param controller Character controller
 * @return Distance fallen (m)
 */
float UpeCharacterController_GetFallDistance(const UpeCharacterController *controller);

/**
 * Get animation blend parameters
 * 
 * Returns values suitable for driving animations:
 * - speed: 0.0-1.0 (0 = idle, 1 = max speed)
 * - direction: 0-360 (0 = forward, 90 = right)
 * - is_in_air: true if not grounded
 * 
 * @param controller Character controller
 * @param out_speed Speed blend value (0-1)
 * @param out_direction Direction angle (0-360)
 * @param out_is_in_air In-air flag
 */
void UpeCharacterController_GetAnimationParameters(
    const UpeCharacterController *controller,
    float *out_speed,
    float *out_direction,
    bool *out_is_in_air
);

/*============================================================================
  Configuration Query (Runtime Tweak)
============================================================================*/

/**
 * Set jump power
 * 
 * @param controller Character controller
 * @param jump_power New jump power (m/s)
 */
void UpeCharacterController_SetJumpPower(
    UpeCharacterController *controller,
    float jump_power
);

/**
 * Set maximum slope angle
 * 
 * @param controller Character controller
 * @param angle_degrees Max climbable slope (degrees)
 */
void UpeCharacterController_SetMaxSlope(
    UpeCharacterController *controller,
    float angle_degrees
);

/**
 * Set terminal velocity
 * 
 * @param controller Character controller
 * @param terminal_velocity Max falling speed (m/s, positive down)
 */
void UpeCharacterController_SetTerminalVelocity(
    UpeCharacterController *controller,
    float terminal_velocity
);

/*============================================================================
  Preset Controllers
============================================================================*/

/**
 * Create a controller for humanoid characters
 * 
 * Pre-configured for typical action game characters:
 * - Height: 1.8m
 * - Walk/Run/Sprint speeds suitable for 60 FPS gameplay
 * - Jump power for realistic platforming
 * - Coyote time for forgiving input
 * 
 * @param world Physics world
 * @param position Starting position
 * @return Allocated controller
 */
UpeCharacterController* UpeCharacterController_CreateHumanoid(
    UpeWorld *world,
    UpeVec3 position
);

/**
 * Create a controller for fast-paced action games
 * 
 * Higher speeds and air control:
 * - Walk/Run/Sprint: 3/7/10 m/s
 * - Jump apex delay: 0.15s (floatier feel)
 * - Air acceleration: 0.8 (more responsive mid-air)
 * 
 * @param world Physics world
 * @param position Starting position
 * @return Allocated controller
 */
UpeCharacterController* UpeCharacterController_CreateActionHero(
    UpeWorld *world,
    UpeVec3 position
);

/**
 * Create a controller for casual/exploration games
 * 
 * Slower, more deliberate movement:
 * - Walk/Run/Sprint: 1.5/3/5 m/s
 * - Coyote time: 0.2s (very forgiving)
 * - Max slope: 60 degrees (climbs more)
 * 
 * @param world Physics world
 * @param position Starting position
 * @return Allocated controller
 */
UpeCharacterController* UpeCharacterController_CreateExplorer(
    UpeWorld *world,
    UpeVec3 position
);

/*============================================================================
  Debug & Utility
============================================================================*/

/**
 * Get diagnostic info as string
 * 
 * Returns buffer with current state info (position, velocity, mode, etc)
 * Caller must free via free()
 * 
 * @param controller Character controller
 * @return Allocated diagnostic string
 */
char* UpeCharacterController_GetDebugString(const UpeCharacterController *controller);

#ifdef __cplusplus
}
#endif

#endif /* UPE_CHARACTER_CONTROLLER_H */
