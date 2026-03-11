/*
  Particle Emitter Demo

  Demonstrates:
  - Emitter pool with continuous emission
  - Emitter spawn requests feeding particle system
  - Basic gravity + drag integration
*/

#include "upe/upe_emitter.h"
#include "upe/upe_particles.h"
#include <stdio.h>

int main(void) {
    printf("=== UPE Particle Emitter Demo ===\n\n");

    /* Create particle system */
    UpeParticleSystem particle_system;
    UpeParticleSystem_Init(&particle_system, 2048);

    /* Create emitter pool */
    UpeEmitterPool *pool = UpeEmitterPool_Create(4, 256);
    if (!pool) {
        printf("Failed to create emitter pool\n");
        return 1;
    }

    /* Configure emitter */
    UpeEmitterDesc desc;
    desc.shape = UPE_EMITTER_SPHERE;
    desc.mode = UPE_EMITTER_CONTINUOUS;
    desc.position.x = 0.0f;
    desc.position.y = 2.0f;
    desc.position.z = 0.0f;
    desc.rotation.x = 0.0f;
    desc.rotation.y = 0.0f;
    desc.rotation.z = 0.0f;
    desc.rotation.w = 1.0f;
    desc.particles_per_frame = 10;
    desc.emission_duration = 0.0f;
    desc.particle_template.velocity.x = 0.5f;
    desc.particle_template.velocity.y = 2.5f;
    desc.particle_template.velocity.z = 0.0f;
    desc.particle_template.lifetime = 3.0f;
    desc.particle_template.mass = 1.0f;
    desc.particle_template.size = 0.1f;
    desc.particle_template.drag = 0.2f;
    desc.shape_scale = 0.5f;
    desc.velocity_spread = 0.6f;
    desc.gravity.x = 0.0f;
    desc.gravity.y = -9.81f;
    desc.gravity.z = 0.0f;
    desc.wind_velocity.x = 0.0f;
    desc.wind_velocity.y = 0.0f;
    desc.wind_velocity.z = 0.0f;
    desc.damping = 0.0f;

    uint32_t emitter_id = UpeEmitterPool_AddEmitter(pool, &desc);
    if (emitter_id == (uint32_t)-1) {
        printf("Failed to add emitter\n");
        UpeEmitterPool_Destroy(pool);
        return 1;
    }

    printf("Emitter created (id=%u)\n", emitter_id);
    printf("Simulating 2 seconds at 60 FPS...\n\n");

    const float dt = 1.0f / 60.0f;
    const int total_frames = 120;

    for (int frame = 0; frame < total_frames; ++frame) {
        UpeEmitterPool_Update(pool, dt);

        UpeParticleSpawnRequest requests[256];
        uint32_t request_count = UpeEmitterPool_GetEmissionQueue(pool, requests, 256);
        if (request_count > 0) {
            UpeParticleSystem_EmitFromRequests(&particle_system, requests, request_count);
        }

        UpeParticleSystem_Step(&particle_system, dt, desc.gravity);

        if (frame % 30 == 0) {
            printf("Frame %d: Particles=%u\n", frame, particle_system.count);
            if (particle_system.count > 0) {
                UpeParticle *p = &particle_system.particles[0];
                printf("  First particle pos=(%.2f, %.2f, %.2f) vel=(%.2f, %.2f, %.2f) life=%.2f\n",
                       p->position.x, p->position.y, p->position.z,
                       p->velocity.x, p->velocity.y, p->velocity.z,
                       p->life);
            }
        }
    }

    UpeEmitterPool_Destroy(pool);
    UpeParticleSystem_Destroy(&particle_system);

    printf("\n=== Demo Complete ===\n");
    return 0;
}
