#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "upe/upe_grapple.h"
#include "upe/upe_math.h"
#include "upe/upe_rope.h"

// Create grapple system
UpeGrapple *UpeGrapple_Create(const UpeGrappleDesc *desc)
{
    if (!desc || desc->max_active_hooks == 0) {
        return NULL;
    }

    UpeGrapple *grapple = (UpeGrapple *)malloc(sizeof(UpeGrapple));
    if (!grapple) return NULL;

    grapple->id = (uint32_t)((uintptr_t)grapple / sizeof(UpeGrapple));
    grapple->player_body_id = desc->player_body_id;
    grapple->max_rope_length = desc->max_rope_length;
    grapple->rope_stiffness = desc->rope_stiffness;
    grapple->pull_force = desc->pull_force;
    grapple->swing_damping = desc->swing_damping;
    grapple->max_hooks = desc->max_active_hooks;
    grapple->active_hook_count = 0;
    grapple->rope_system = NULL;
    grapple->world = NULL;

    // Allocate hook array
    grapple->hooks = (UpeGrappleHook *)calloc(desc->max_active_hooks, sizeof(UpeGrappleHook));
    if (!grapple->hooks) {
        free(grapple);
        return NULL;
    }

    // Initialize hook IDs
    for (uint32_t i = 0; i < desc->max_active_hooks; i++) {
        grapple->hooks[i].id = i;
        grapple->hooks[i].state = UPE_GRAPPLE_IDLE;
        grapple->hooks[i].rope_constraint_id = (uint32_t)-1;
        grapple->hooks[i].max_search_time = 2.0f;  // 2 second search timeout
    }

    return grapple;
}

// Destroy grapple
void UpeGrapple_Destroy(UpeGrapple *grapple)
{
    if (!grapple) return;

    if (grapple->hooks) {
        free(grapple->hooks);
        grapple->hooks = NULL;
    }

    free(grapple);
}

// Set rope system reference
void UpeGrapple_SetRopeSystem(UpeGrapple *grapple, UpeRope *rope)
{
    if (grapple) {
        grapple->rope_system = rope;
    }
}

// Set world reference
void UpeGrapple_SetWorld(UpeGrapple *grapple, struct UpeWorld *world)
{
    if (grapple) {
        grapple->world = world;
    }
}

// Fire grapple - find next available hook and raycast to anchor point
uint32_t UpeGrapple_Fire(UpeGrapple *grapple, UpeVec3 direction, float max_distance)
{
    if (!grapple || !grapple->rope_system || grapple->active_hook_count >= grapple->max_hooks) {
        return (uint32_t)-1;
    }

    // Normalize direction
    UpeVec3 normalized_dir = UpeVec3_Normalize(direction);

    // Find inactive hook slot
    UpeGrappleHook *free_hook = NULL;
    uint32_t free_idx = (uint32_t)-1;

    for (uint32_t i = 0; i < grapple->max_hooks; i++) {
        if (grapple->hooks[i].state == UPE_GRAPPLE_IDLE) {
            free_hook = &grapple->hooks[i];
            free_idx = i;
            break;
        }
    }

    if (!free_hook) {
        return (uint32_t)-1;
    }

    // Set up hook in aiming state (rope will search for target)
    free_hook->state = UPE_GRAPPLE_AIMING;
    free_hook->time_since_fired = 0.0f;
    free_hook->current_rope_length = 0.0f;
    free_hook->swing_energy = 0.0f;
    free_hook->is_breaking = false;
    free_hook->can_retract = false;
    free_hook->rope_constraint_id = (uint32_t)-1;

    // Store anchor direction (rope will trace along this)
    free_hook->anchor.world_position = normalized_dir;  // Use as direction vector
    free_hook->anchor.max_tension = max_distance * 100.0f;  // Estimate breaking point from range
    free_hook->anchor.friction_on_anchor = 0.8f;

    grapple->active_hook_count++;

    return free_idx;
}

// Release specific hook
void UpeGrapple_Release(UpeGrapple *grapple, uint32_t hook_id)
{
    if (!grapple || hook_id >= grapple->max_hooks) {
        return;
    }

    UpeGrappleHook *hook = &grapple->hooks[hook_id];

    if (hook->state != UPE_GRAPPLE_IDLE) {
        // Remove rope constraint if active
        if (grapple->rope_system && hook->rope_constraint_id != (uint32_t)-1) {
            UpeRope_RemoveConstraint(grapple->rope_system, hook->rope_constraint_id);
        }

        hook->state = UPE_GRAPPLE_RELEASED;
        hook->rope_constraint_id = (uint32_t)-1;
        if (grapple->active_hook_count > 0) {
            grapple->active_hook_count--;
        }
    }
}

// Release all hooks
void UpeGrapple_ReleaseAll(UpeGrapple *grapple)
{
    if (!grapple) return;

    for (uint32_t i = 0; i < grapple->max_hooks; i++) {
        if (grapple->hooks[i].state != UPE_GRAPPLE_IDLE) {
            UpeGrapple_Release(grapple, i);
        }
    }
}

// Retract rope - shorten length and pull toward player
void UpeGrapple_Retract(UpeGrapple *grapple, uint32_t hook_id, float speed)
{
    if (!grapple || hook_id >= grapple->max_hooks) {
        return;
    }

    UpeGrappleHook *hook = &grapple->hooks[hook_id];

    if (hook->state == UPE_GRAPPLE_ATTACHED || hook->state == UPE_GRAPPLE_SWINGING) {
        hook->can_retract = true;
        // Shorten rope (will be handled in update)
        hook->current_rope_length -= speed;
        if (hook->current_rope_length < 0.5f) {
            UpeGrapple_Release(grapple, hook_id);
        }
    }
}

// Get hook
UpeGrappleHook *UpeGrapple_GetHook(UpeGrapple *grapple, uint32_t hook_id)
{
    if (!grapple || hook_id >= grapple->max_hooks) {
        return NULL;
    }

    return &grapple->hooks[hook_id];
}

// Get primary hook (most recent active)
UpeGrappleHook *UpeGrapple_GetPrimaryHook(UpeGrapple *grapple)
{
    if (!grapple || grapple->active_hook_count == 0) {
        return NULL;
    }

    // Find most recently attached hook
    for (int32_t i = grapple->max_hooks - 1; i >= 0; i--) {
        if (grapple->hooks[i].state == UPE_GRAPPLE_ATTACHED ||
            grapple->hooks[i].state == UPE_GRAPPLE_SWINGING) {
            return &grapple->hooks[i];
        }
    }

    return NULL;
}

// Get rope tension
float UpeGrapple_GetTension(UpeGrapple *grapple, uint32_t hook_id)
{
    if (!grapple || hook_id >= grapple->max_hooks || !grapple->rope_system) {
        return 0.0f;
    }

    UpeGrappleHook *hook = &grapple->hooks[hook_id];

    if (hook->rope_constraint_id != (uint32_t)-1) {
        return UpeRope_GetTension(grapple->rope_system, hook->rope_constraint_id);
    }

    return 0.0f;
}

// Check if grapple is active
bool UpeGrapple_IsActive(UpeGrapple *grapple)
{
    if (!grapple) return false;

    for (uint32_t i = 0; i < grapple->max_hooks; i++) {
        if (grapple->hooks[i].state == UPE_GRAPPLE_ATTACHED ||
            grapple->hooks[i].state == UPE_GRAPPLE_SWINGING) {
            return true;
        }
    }

    return false;
}

// Update grapple system - handle aiming, attachment, swinging, breaking
void UpeGrapple_Update(UpeGrapple *grapple, float dt)
{
    if (!grapple || !grapple->rope_system) {
        return;
    }

    for (uint32_t i = 0; i < grapple->max_hooks; i++) {
        UpeGrappleHook *hook = &grapple->hooks[i];

        switch (hook->state) {
            case UPE_GRAPPLE_AIMING:
                // Simulate rope tracing through space
                hook->time_since_fired += dt;
                hook->current_rope_length += 100.0f * dt;  // Rope extends at speed

                // Timeout if no anchor found
                if (hook->time_since_fired > hook->max_search_time) {
                    hook->state = UPE_GRAPPLE_IDLE;
                    grapple->active_hook_count--;
                }
                // In real implementation, would raycast here to detect anchors
                // For now, simulate automatic attachment after distance traveled
                else if (hook->current_rope_length > grapple->max_rope_length * 0.5f) {
                    // Auto-attach when halfway extended (simplified)
                    hook->state = UPE_GRAPPLE_ATTACHED;
                    // Create rope constraint (simplified - just anchor point)
                    // Real implementation would use rope system's AddConstraint
                }
                break;

            case UPE_GRAPPLE_ATTACHED:
                // Rope is taut, ready to swing
                hook->state = UPE_GRAPPLE_SWINGING;
                hook->can_retract = true;
                break;

            case UPE_GRAPPLE_SWINGING: {
                // Apply swing damping
                float damping_factor = 1.0f - (grapple->swing_damping * dt);
                if (damping_factor < 0.0f) damping_factor = 0.0f;
                
                hook->swing_energy *= damping_factor;
                
                // Shorten rope if retracting
                if (hook->can_retract && hook->current_rope_length > 1.0f) {
                    hook->current_rope_length -= grapple->pull_force * dt;
                }

                // Check for breaking
                float tension = UpeGrapple_GetTension(grapple, i);
                if (tension > hook->anchor.max_tension) {
                    hook->is_breaking = true;
                    UpeGrapple_Release(grapple, i);
                }
                break;
            }

            case UPE_GRAPPLE_RELEASED:
                // Transition to idle after brief release state
                hook->state = UPE_GRAPPLE_IDLE;
                break;

            case UPE_GRAPPLE_IDLE:
            case UPE_GRAPPLE_RETRACTING:
            default:
                break;
        }
    }
}

