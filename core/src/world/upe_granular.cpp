#include "upe/upe_granular.h"
#include "upe/upe_math.h"
#include <stdlib.h>
#include <math.h>

void UpeGranularSystem_Init(UpeGranularSystem *sys, uint32_t max_particles) {
    if (!sys) {
        return;
    }

    sys->particles = (UpeGranularParticle *)malloc(max_particles * sizeof(UpeGranularParticle));
    sys->count = 0;
    sys->cohesion = 0.1f;
    sys->angle_of_repose = 0.5236f;

    if (sys->particles) {
        for (uint32_t i = 0; i < max_particles; ++i) {
            sys->particles[i].position.x = 0.0f;
            sys->particles[i].position.y = 0.0f;
            sys->particles[i].position.z = 0.0f;
            sys->particles[i].velocity.x = 0.0f;
            sys->particles[i].velocity.y = 0.0f;
            sys->particles[i].velocity.z = 0.0f;
            sys->particles[i].radius = 0.05f;
            sys->particles[i].friction_angle = sys->angle_of_repose;
        }
    }
}

void UpeGranularSystem_Step(UpeGranularSystem *sys, float dt, UpeVec3 gravity) {
    if (!sys || !sys->particles || dt <= 0.0f) {
        return;
    }

    for (uint32_t i = 0; i < sys->count; ++i) {
        sys->particles[i].velocity = UpeVec3_Add(sys->particles[i].velocity, UpeVec3_Scale(gravity, dt));
    }

    for (uint32_t i = 0; i < sys->count; ++i) {
        for (uint32_t j = i + 1; j < sys->count; ++j) {
            UpeVec3 delta = UpeVec3_Sub(sys->particles[j].position, sys->particles[i].position);
            float dist2 = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
            float contact_dist = sys->particles[i].radius + sys->particles[j].radius;
            float contact_dist2 = contact_dist * contact_dist;

            if (dist2 < contact_dist2 && dist2 > 1e-8f) {
                float dist = sqrtf(dist2);
                UpeVec3 normal = UpeVec3_Scale(delta, 1.0f / dist);
                float overlap = contact_dist - dist;

                UpeVec3 correction = UpeVec3_Scale(normal, overlap * 0.5f);
                sys->particles[i].position = UpeVec3_Sub(sys->particles[i].position, correction);
                sys->particles[j].position = UpeVec3_Add(sys->particles[j].position, correction);

                UpeVec3 rel_v = UpeVec3_Sub(sys->particles[j].velocity, sys->particles[i].velocity);
                float rel_n = UpeVec3_Dot(rel_v, normal);
                if (rel_n < 0.0f) {
                    float restitution = 0.2f;
                    float impulse = -(1.0f + restitution) * rel_n * 0.5f;
                    UpeVec3 impulse_v = UpeVec3_Scale(normal, impulse);
                    sys->particles[i].velocity = UpeVec3_Sub(sys->particles[i].velocity, impulse_v);
                    sys->particles[j].velocity = UpeVec3_Add(sys->particles[j].velocity, impulse_v);
                }
            }
        }
    }

    float global_damping = 0.995f;
    for (uint32_t i = 0; i < sys->count; ++i) {
        sys->particles[i].velocity = UpeVec3_Scale(sys->particles[i].velocity, global_damping);
        sys->particles[i].position = UpeVec3_Add(sys->particles[i].position,
                                                 UpeVec3_Scale(sys->particles[i].velocity, dt));

        float min_y = sys->particles[i].radius;
        if (sys->particles[i].position.y < min_y) {
            sys->particles[i].position.y = min_y;
            if (sys->particles[i].velocity.y < 0.0f) {
                sys->particles[i].velocity.y *= -0.2f;
            }

            float slide = 1.0f - fminf(0.9f, sys->cohesion + 0.1f);
            sys->particles[i].velocity.x *= slide;
            sys->particles[i].velocity.z *= slide;
        }
    }
}

void UpeGranularSystem_Destroy(UpeGranularSystem *sys) {
    if (!sys) {
        return;
    }
    free(sys->particles);
    sys->particles = NULL;
    sys->count = 0;
}
