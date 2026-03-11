#pragma once

#include <stdint.h>
#include "upe/upe_math.h"
#include "upe/upe_body.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeCharacterDesc {
    float height;
    float radius;
    float step_height;
    float max_slope_angle;
} UpeCharacterDesc;

typedef struct UpeCharacter {
    uint32_t body_id;
    float height;
    float radius;
    float step_height;
    float max_slope_angle;
    UpeVec3 velocity;
    int grounded;
} UpeCharacter;

void UpeCharacter_Init(UpeCharacter *character, const UpeCharacterDesc *desc);
void UpeCharacter_Move(UpeCharacter *character, UpeVec3 direction, float dt);
void UpeCharacter_Jump(UpeCharacter *character, float impulse);

#ifdef __cplusplus
}
#endif
