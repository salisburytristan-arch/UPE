#include "upe/upe_character.h"
#include "upe/upe_api.h"
#include <math.h>

void UpeCharacter_Init(UpeCharacter *character, const UpeCharacterDesc *desc) {
    if (!character || !desc) {
        return;
    }
    character->height = desc->height;
    character->radius = desc->radius;
    character->step_height = desc->step_height;
    character->max_slope_angle = desc->max_slope_angle;
    character->velocity = {0.0f, 0.0f, 0.0f};
    character->grounded = 0;
}

void UpeCharacter_Move(UpeCharacter *character, UpeVec3 direction, float dt) {
    if (!character) {
        return;
    }
    // Apply acceleration with damping
    const float max_speed = 10.0f;
    const float acceleration = 20.0f;
    
    UpeVec3 move_vel = UpeVec3_Scale(direction, acceleration * dt);
    character->velocity = UpeVec3_Add(character->velocity, move_vel);
    
    // Clamp horizontal speed
    float vx_sq = character->velocity.x * character->velocity.x + character->velocity.z * character->velocity.z;
    if (vx_sq > max_speed * max_speed) {
        float vx_len = sqrtf(vx_sq);
        character->velocity.x = character->velocity.x / vx_len * max_speed;
        character->velocity.z = character->velocity.z / vx_len * max_speed;
    }
    
    // Apply damping to horizontal velocity when not grounded
    if (!character->grounded) {
        const float air_damping = 0.9f;
        character->velocity.x *= air_damping;
        character->velocity.z *= air_damping;
    } else {
        // Apply friction when grounded
        const float ground_damping = 0.95f;
        character->velocity.x *= ground_damping;
        character->velocity.z *= ground_damping;
    }
}

void UpeCharacter_Jump(UpeCharacter *character, float impulse) {
    if (!character || !character->grounded) {
        return;
    }
    character->velocity.y = impulse;
    character->grounded = 0;
}
