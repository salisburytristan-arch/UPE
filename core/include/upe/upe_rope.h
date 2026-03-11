#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

// Rope constraint: maximum distance between two bodies
// Useful for cables, ropes, chains (one-way constraint)
typedef struct UpeRopeConstraint {
    uint32_t body_a_id;          // First body ID
    uint32_t body_b_id;          // Second body ID
    UpeVec3 anchor_a;            // Attachment point on body A (local space)
    UpeVec3 anchor_b;            // Attachment point on body B (local space)
    float max_length;            // Maximum cable length
    float min_length;            // Minimum cable length (for motor)
    float elasticity;            // Stretch factor (0 = inelastic, >0 = elastic)
    int is_taut;                 // Whether rope is currently taut
    float current_length;        // Cache current distance
    float lambda_rope;           // Accumulated impulse (warm-starting)
    
    // Motor support
    uint8_t motor_enabled;       // Whether motor is active
    float motor_speed;           // Target winch/motor speed (negative = retract, positive = extend)
    float max_motor_force;       // Maximum force motor can apply
    uint32_t constraint_count;   // For force scaling
} UpeRopeConstraint;

typedef struct UpeRopeDesc {
    uint32_t body_a_id;
    uint32_t body_b_id;
    UpeVec3 anchor_a;
    UpeVec3 anchor_b;
    float max_length;
    float elasticity;            // Default: 1.0 (perfectly elastic)
} UpeRopeDesc;

typedef struct UpeRope {
    uint32_t id;
    UpeRopeConstraint *constraints;
    uint32_t constraint_count;
    uint32_t max_constraints;
} UpeRope;

// Create rope system
UpeRope *UpeRope_Create(uint32_t max_constraints);

// Destroy rope system
void UpeRope_Destroy(UpeRope *rope);

// Add rope constraint
uint32_t UpeRope_AddConstraint(UpeRope *rope, const UpeRopeDesc *desc);

// Remove rope constraint
void UpeRope_RemoveConstraint(UpeRope *rope, uint32_t idx);

// Get constraint
UpeRopeConstraint *UpeRope_GetConstraint(UpeRope *rope, uint32_t idx);

// Update rope state (call once per physics step)
void UpeRope_Update(UpeRope *rope, struct UpeWorld *world, float dt);

// Check if rope is taut
int UpeRope_IsTaut(UpeRope *rope, uint32_t idx);

// Get current rope tension (magnitude of constraint force)
float UpeRope_GetTension(UpeRope *rope, uint32_t idx);

// Motor control (for winches, pulleys, grapple hooks)
void UpeRope_EnableMotor(UpeRope *rope, uint32_t idx, float target_speed, float max_force);
void UpeRope_DisableMotor(UpeRope *rope, uint32_t idx);
void UpeRope_SetMotorSpeed(UpeRope *rope, uint32_t idx, float speed);

#ifdef __cplusplus
}
#endif

