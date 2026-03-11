#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpePlasmaParticle {
    UpeVec3 position;
    UpeVec3 velocity;
    float charge;
    float mass;
} UpePlasmaParticle;

typedef struct UpePlasmaSystem {
    UpePlasmaParticle *particles;
    uint32_t count;
    UpeVec3 electric_field;
    UpeVec3 magnetic_field;
} UpePlasmaSystem;

void UpePlasmaSystem_Init(UpePlasmaSystem *sys, uint32_t max_particles);
void UpePlasmaSystem_Step(UpePlasmaSystem *sys, float dt);
void UpePlasmaSystem_Destroy(UpePlasmaSystem *sys);

#ifdef __cplusplus
}
#endif
