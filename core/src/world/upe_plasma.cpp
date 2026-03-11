#include "upe/upe_plasma.h"
#include <stdlib.h>

void UpePlasmaSystem_Init(UpePlasmaSystem *sys, uint32_t max_particles) {
    if (!sys) {
        return;
    }
    sys->particles = (UpePlasmaParticle *)malloc(max_particles * sizeof(UpePlasmaParticle));
    sys->count = 0;
    sys->electric_field = {0.0f, 0.0f, 0.0f};
    sys->magnetic_field = {0.0f, 0.0f, 1.0f};
}

void UpePlasmaSystem_Step(UpePlasmaSystem *sys, float dt) {
    if (!sys) {
        return;
    }
    
    for (uint32_t i = 0; i < sys->count; ++i) {
        UpePlasmaParticle *p = &sys->particles[i];
        
        UpeVec3 electric_force = UpeVec3_Scale(sys->electric_field, p->charge);
        
        UpeVec3 v_cross_b = UpeVec3_Cross(p->velocity, sys->magnetic_field);
        UpeVec3 magnetic_force = UpeVec3_Scale(v_cross_b, p->charge);
        
        UpeVec3 total_force = UpeVec3_Add(electric_force, magnetic_force);
        UpeVec3 acceleration = UpeVec3_Scale(total_force, 1.0f / p->mass);
        
        p->velocity = UpeVec3_Add(p->velocity, UpeVec3_Scale(acceleration, dt));
        p->position = UpeVec3_Add(p->position, UpeVec3_Scale(p->velocity, dt));
    }
}

void UpePlasmaSystem_Destroy(UpePlasmaSystem *sys) {
    if (!sys) {
        return;
    }
    free(sys->particles);
}
