#pragma once

#include <stdint.h>
#include "upe/upe_math.h"
#include "upe/upe_emitter.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeParticle {
    UpeVec3 position;
    UpeVec3 velocity;
    float mass;
    float life;
    float size;
    float drag;
    uint32_t flags;
} UpeParticle;

typedef struct UpeParticleSystem {
    UpeParticle *particles;
    uint32_t capacity;
    uint32_t count;
} UpeParticleSystem;

void UpeParticleSystem_Init(UpeParticleSystem *sys, uint32_t capacity);
void UpeParticleSystem_Emit(UpeParticleSystem *sys, UpeVec3 position, UpeVec3 velocity);
void UpeParticleSystem_EmitEx(UpeParticleSystem *sys, UpeVec3 position, UpeVec3 velocity,
                              float mass, float life, float size, float drag);
void UpeParticleSystem_EmitFromRequests(UpeParticleSystem *sys,
                                        const UpeParticleSpawnRequest *requests,
                                        uint32_t request_count);
void UpeParticleSystem_Step(UpeParticleSystem *sys, float dt, UpeVec3 gravity);
void UpeParticleSystem_Destroy(UpeParticleSystem *sys);

#ifdef __cplusplus
}
#endif
