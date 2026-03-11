#pragma once

#include <stdint.h>
#include "upe/upe_math.h"
#include "upe/upe_rope.h"

#ifdef __cplusplus
extern "C" {
#endif

// Grapple hook system: swing on ropes, attach to surfaces, dynamic anchor points
// Combines rope constraints, raycasts, and world forces into gameplay mechanics

typedef enum UpeGrappleState {
    UPE_GRAPPLE_IDLE,          // No active grapple
    UPE_GRAPPLE_AIMING,        // Rope fired, searching for anchor
    UPE_GRAPPLE_ATTACHED,      // Successfully hooked to surface
    UPE_GRAPPLE_SWINGING,      // Swinging on rope
    UPE_GRAPPLE_RETRACTING,    // Pulling back to source
    UPE_GRAPPLE_RELEASED,      // Rope cut or released
} UpeGrappleState;

typedef struct UpeGrappleAnchor {
    UpeVec3 world_position;     // Position on surface
    UpeVec3 surface_normal;     // Normal for friction
    uint32_t anchor_body_id;    // What body we're attached to (0 = environment)
    uint32_t anchor_shape_id;   // Which shape on compound body
    float max_tension;          // Rope breaking point (force units)
    float friction_on_anchor;   // Grip strength (0-1)
} UpeGrappleAnchor;

typedef struct UpeGrappleDesc {
    uint32_t player_body_id;    // Character/player body
    float max_rope_length;      // Maximum extension
    float rope_stiffness;       // Spring constant for rope
    float pull_force;           // Retracting force magnitude
    float swing_damping;        // Swing oscillation damping
    uint32_t max_active_hooks;  // Allow multi-grapple
} UpeGrappleDesc;

typedef struct UpeGrappleHook {
    uint32_t id;
    UpeGrappleState state;
    
    // Rope constraint (active when attached/swinging)
    uint32_t rope_constraint_id;  // From rope system
    
    // Anchor point
    UpeGrappleAnchor anchor;
    
    // Swing physics
    float current_rope_length;     // Active extension
    UpeVec3 swing_velocity;        // Velocity contribution from swinging
    float swing_energy;            // For smart damping
    
    // Timers
    float time_since_fired;        // For aiming/search phase
    float max_search_time;         // Give up after duration
    
    // States
    bool is_breaking;              // Exceeded tension
    bool can_retract;              // Player may pull back
} UpeGrappleHook;

typedef struct UpeGrapple {
    uint32_t id;
    uint32_t player_body_id;
    
    // Hook array
    UpeGrappleHook *hooks;
    uint32_t active_hook_count;
    uint32_t max_hooks;
    
    // Physics parameters
    float max_rope_length;
    float rope_stiffness;
    float pull_force;
    float swing_damping;
    
    // Reference to rope system (will point to world's rope)
    UpeRope *rope_system;
    
    // World reference for queries
    struct UpeWorld *world;
} UpeGrapple;

// Create grapple system for player
UpeGrapple *UpeGrapple_Create(const UpeGrappleDesc *desc);

// Destroy grapple system
void UpeGrapple_Destroy(UpeGrapple *grapple);

// Fire grapple toward target direction
// Returns hook index on success, or -1 if no hooks available
uint32_t UpeGrapple_Fire(UpeGrapple *grapple, UpeVec3 direction, float max_distance);

// Release specific hook (detach rope)
void UpeGrapple_Release(UpeGrapple *grapple, uint32_t hook_id);

// Release all hooks
void UpeGrapple_ReleaseAll(UpeGrapple *grapple);

// Pull/retract rope (shorten to player)
void UpeGrapple_Retract(UpeGrapple *grapple, uint32_t hook_id, float speed);

// Query hook state
UpeGrappleHook *UpeGrapple_GetHook(UpeGrapple *grapple, uint32_t hook_id);

// Get primary (most recently attached) hook
UpeGrappleHook *UpeGrapple_GetPrimaryHook(UpeGrapple *grapple);

// Step grapple physics (search, swing updates, breaking checks)
// Called once per physics frame
void UpeGrapple_Update(UpeGrapple *grapple, float dt);

// Set rope system reference (called by world after creation)
void UpeGrapple_SetRopeSystem(UpeGrapple *grapple, UpeRope *rope);

// Set world reference (for raycasts)
void UpeGrapple_SetWorld(UpeGrapple *grapple, struct UpeWorld *world);

// Query active rope tension
float UpeGrapple_GetTension(UpeGrapple *grapple, uint32_t hook_id);

// Check if grapple is active (any hook attached/swinging)
bool UpeGrapple_IsActive(UpeGrapple *grapple);

#ifdef __cplusplus
}
#endif

