#include "upe/upe_particles.h"
#include <stdlib.h>

void UpeParticleSystem_Init(UpeParticleSystem *sys, uint32_t capacity) {
    if (!sys) {
        return;
    }
    sys->capacity = capacity;
    sys->count = 0;
    sys->particles = (UpeParticle *)malloc(capacity * sizeof(UpeParticle));
}

void UpeParticleSystem_Emit(UpeParticleSystem *sys, UpeVec3 position, UpeVec3 velocity) {
    UpeParticleSystem_EmitEx(sys, position, velocity, 1.0f, 5.0f, 1.0f, 0.0f);
}

void UpeParticleSystem_EmitEx(UpeParticleSystem *sys, UpeVec3 position, UpeVec3 velocity,
                              float mass, float life, float size, float drag) {
    if (!sys || sys->count >= sys->capacity) {
        return;
    }
    sys->particles[sys->count].position = position;
    sys->particles[sys->count].velocity = velocity;
    sys->particles[sys->count].mass = mass > 0.0f ? mass : 1.0f;
    sys->particles[sys->count].life = life > 0.0f ? life : 0.0f;
    sys->particles[sys->count].size = size > 0.0f ? size : 1.0f;
    sys->particles[sys->count].drag = drag > 0.0f ? drag : 0.0f;
    sys->particles[sys->count].flags = 0;
    sys->count++;
}

void UpeParticleSystem_EmitFromRequests(UpeParticleSystem *sys,
                                        const UpeParticleSpawnRequest *requests,
                                        uint32_t request_count) {
    if (!sys || !requests || request_count == 0) {
        return;
    }

    for (uint32_t i = 0; i < request_count && sys->count < sys->capacity; ++i) {
        UpeParticleSystem_EmitEx(sys,
                                 requests[i].position,
                                 requests[i].velocity,
                                 requests[i].mass,
                                 requests[i].lifetime,
                                 requests[i].size,
                                 requests[i].drag);
    }
}

void UpeParticleSystem_Step(UpeParticleSystem *sys, float dt, UpeVec3 gravity) {
    if (!sys) {
        return;
    }
    for (uint32_t i = 0; i < sys->count; ++i) {
        UpeParticle *p = &sys->particles[i];
        p->velocity = UpeVec3_Add(p->velocity, UpeVec3_Scale(gravity, dt));

        if (p->drag > 0.0f) {
            float drag_factor = 1.0f - p->drag * dt;
            if (drag_factor < 0.0f) {
                drag_factor = 0.0f;
            }
            p->velocity = UpeVec3_Scale(p->velocity, drag_factor);
        }

        p->position = UpeVec3_Add(p->position, UpeVec3_Scale(p->velocity, dt));
        p->life -= dt;
    }
    
    for (uint32_t i = 0; i < sys->count; ) {
        if (sys->particles[i].life <= 0.0f) {
            sys->particles[i] = sys->particles[sys->count - 1];
            sys->count--;
        } else {
            i++;
        }
    }
}

void UpeParticleSystem_Destroy(UpeParticleSystem *sys) {
    if (!sys) {
        return;
    }
    free(sys->particles);
}
