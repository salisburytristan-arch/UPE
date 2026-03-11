#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeGranularParticle {
    UpeVec3 position;
    UpeVec3 velocity;
    float radius;
    float friction_angle;
} UpeGranularParticle;

typedef struct UpeGranularSystem {
    UpeGranularParticle *particles;
    uint32_t count;
    float cohesion;
    float angle_of_repose;
} UpeGranularSystem;

void UpeGranularSystem_Init(UpeGranularSystem *sys, uint32_t max_particles);
void UpeGranularSystem_Step(UpeGranularSystem *sys, float dt, UpeVec3 gravity);
void UpeGranularSystem_Destroy(UpeGranularSystem *sys);

#ifdef __cplusplus
}
#endif
