#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "upe/upe_emitter.h"
#include "upe/upe_math.h"

static float Rand01(UpeRng *rng)
{
    return UpeRng_NextF32(rng);
}

// Generate random point in sphere
static UpeVec3 RandomPointInSphere(float radius, UpeRng *rng)
{
    float theta = Rand01(rng) * 6.28318f;        // 2*pi
    float phi = Rand01(rng) * 3.14159f;          // pi
    float r = cbrtf(Rand01(rng)) * radius;

    float sin_phi = sinf(phi);
    UpeVec3 point;
    point.x = r * sin_phi * cosf(theta);
    point.y = r * cosf(phi);
    point.z = r * sin_phi * sinf(theta);

    return point;
}

// Generate random point in cone
static UpeVec3 RandomPointInCone(float height, float radius, UpeRng *rng)
{
    float theta = Rand01(rng) * 6.28318f;
    float t = Rand01(rng);

    // Cone expands linearly with height
    float local_radius = radius * t;
    float z = height * t;

    UpeVec3 point;
    point.x = local_radius * cosf(theta);
    point.y = local_radius * sinf(theta);
    point.z = z;

    return point;
}

// Generate random point in box
static UpeVec3 RandomPointInBox(float half_size, UpeRng *rng)
{
    UpeVec3 point;
    point.x = (Rand01(rng) - 0.5f) * 2.0f * half_size;
    point.y = (Rand01(rng) - 0.5f) * 2.0f * half_size;
    point.z = (Rand01(rng) - 0.5f) * 2.0f * half_size;
    return point;
}

// Create emitter pool
UpeEmitterPool *UpeEmitterPool_Create(uint32_t max_emitters, uint32_t max_particles_per_frame)
{
    if (max_emitters == 0 || max_particles_per_frame == 0) {
        return NULL;
    }

    UpeEmitterPool *pool = (UpeEmitterPool *)malloc(sizeof(UpeEmitterPool));
    if (!pool) {
        return NULL;
    }

    memset(pool, 0, sizeof(UpeEmitterPool));

    static uint32_t next_pool_id = 1;
    pool->id = next_pool_id++;
    pool->max_emitters = max_emitters;
    pool->total_particle_capacity = max_particles_per_frame;
    pool->particles_emitted_this_frame = 0;

    pool->emitters = (UpeEmitter *)calloc(max_emitters, sizeof(UpeEmitter));
    if (!pool->emitters) {
        free(pool);
        return NULL;
    }

    // Initialize emitter IDs
    for (uint32_t i = 0; i < max_emitters; i++) {
        pool->emitters[i].id = i;
        pool->emitters[i].is_active = false;
        pool->emitters[i].time_active = 0.0f;
        pool->emitters[i].emission_rate_remaining = 0.0f;
    }

    UpeRng_Seed(&pool->rng, (uint64_t)pool->id ^ 0xA17E5D31u);

    return pool;
}

void UpeEmitterPool_SetSeed(UpeEmitterPool *pool, uint64_t seed)
{
    if (!pool) {
        return;
    }
    UpeRng_Seed(&pool->rng, seed ? seed : 0xA17E5D31u);
}

// Destroy emitter pool
void UpeEmitterPool_Destroy(UpeEmitterPool *pool)
{
    if (!pool) return;

    if (pool->emitters) {
        free(pool->emitters);
        pool->emitters = NULL;
    }

    free(pool);
}

// Add emitter to pool
uint32_t UpeEmitterPool_AddEmitter(UpeEmitterPool *pool, const UpeEmitterDesc *desc)
{
    if (!pool || !desc || pool->emitter_count >= pool->max_emitters) {
        return (uint32_t)-1;
    }

    UpeEmitter *em = &pool->emitters[pool->emitter_count];
    
    em->shape = desc->shape;
    em->mode = desc->mode;
    em->position = desc->position;
    em->rotation = desc->rotation;
    em->particles_per_frame = desc->particles_per_frame;
    em->emission_duration = desc->emission_duration;
    em->shape_scale = desc->shape_scale;
    em->velocity_spread = desc->velocity_spread;
    em->particle_template = desc->particle_template;
    em->gravity = desc->gravity;
    em->wind_velocity = desc->wind_velocity;
    em->damping = desc->damping;
    
    em->time_active = 0.0f;
    em->emission_rate_remaining = 0.0f;
    em->is_active = (desc->mode == UPE_EMITTER_CONTINUOUS || desc->mode == UPE_EMITTER_TIMED);

    uint32_t idx = pool->emitter_count;
    pool->emitter_count++;

    return idx;
}

// Remove emitter
void UpeEmitterPool_RemoveEmitter(UpeEmitterPool *pool, uint32_t emitter_idx)
{
    if (!pool || emitter_idx >= pool->emitter_count) {
        return;
    }

    // Swap with last and shrink
    if (emitter_idx < pool->emitter_count - 1) {
        pool->emitters[emitter_idx] = pool->emitters[pool->emitter_count - 1];
    }
    pool->emitter_count--;
}

// Get emitter
UpeEmitter *UpeEmitterPool_GetEmitter(UpeEmitterPool *pool, uint32_t emitter_idx)
{
    if (!pool || emitter_idx >= pool->emitter_count) {
        return NULL;
    }

    return &pool->emitters[emitter_idx];
}

// Set emitter active state
void UpeEmitterPool_SetActive(UpeEmitterPool *pool, uint32_t emitter_idx, bool active)
{
    if (!pool || emitter_idx >= pool->emitter_count) {
        return;
    }

    pool->emitters[emitter_idx].is_active = active;
    if (!active) {
        pool->emitters[emitter_idx].time_active = 0.0f;
    }
}

// Set emitter transform
void UpeEmitterPool_SetTransform(UpeEmitterPool *pool, uint32_t emitter_idx,
                                 UpeVec3 position, UpeQuat rotation)
{
    if (!pool || emitter_idx >= pool->emitter_count) {
        return;
    }

    pool->emitters[emitter_idx].position = position;
    pool->emitters[emitter_idx].rotation = rotation;
}

// Set forces
void UpeEmitterPool_SetForces(UpeEmitterPool *pool, uint32_t emitter_idx,
                              UpeVec3 gravity, UpeVec3 wind)
{
    if (!pool || emitter_idx >= pool->emitter_count) {
        return;
    }

    pool->emitters[emitter_idx].gravity = gravity;
    pool->emitters[emitter_idx].wind_velocity = wind;
}

// Get active emitter count
uint32_t UpeEmitterPool_GetActiveCount(UpeEmitterPool *pool)
{
    if (!pool) return 0;

    uint32_t count = 0;
    for (uint32_t i = 0; i < pool->emitter_count; i++) {
        if (pool->emitters[i].is_active) {
            count++;
        }
    }
    return count;
}

// Emit burst
void UpeEmitterPool_EmitBurst(UpeEmitterPool *pool, uint32_t emitter_idx, uint32_t count)
{
    if (!pool || emitter_idx >= pool->emitter_count) {
        return;
    }

    UpeEmitter *em = &pool->emitters[emitter_idx];
    // Add to emission queue (handled in Update)
    em->emission_rate_remaining += (float)count;
}

// Update emitters (step time, accumulate emission)
void UpeEmitterPool_Update(UpeEmitterPool *pool, float dt)
{
    if (!pool) return;

    pool->particles_emitted_this_frame = 0;

    for (uint32_t i = 0; i < pool->emitter_count; i++) {
        UpeEmitter *em = &pool->emitters[i];

        if (!em->is_active) {
            continue;
        }

        em->time_active += dt;

        // Check if emitter should stop
        if (em->mode == UPE_EMITTER_TIMED && em->time_active >= em->emission_duration) {
            em->is_active = false;
            em->time_active = 0.0f;
            continue;
        }

        // Accumulate emission rate
        if (em->mode == UPE_EMITTER_CONTINUOUS || em->mode == UPE_EMITTER_TIMED) {
            float particle_rate = (float)em->particles_per_frame / (1.0f / 60.0f);  // Assume 60 fps base
            em->emission_rate_remaining += particle_rate * dt;
        }
    }
}

// Get emission queue for current frame
uint32_t UpeEmitterPool_GetEmissionQueue(UpeEmitterPool *pool,
                                         UpeParticleSpawnRequest *out_requests,
                                         uint32_t max_requests)
{
    if (!pool || !out_requests || max_requests == 0) {
        return 0;
    }

    uint32_t total_emitted = 0;

    for (uint32_t i = 0; i < pool->emitter_count && total_emitted < max_requests; i++) {
        UpeEmitter *em = &pool->emitters[i];

        if (!em->is_active || em->emission_rate_remaining <= 0.0f) {
            continue;
        }

        // Determine particles to emit this frame
        uint32_t particles_to_emit = (uint32_t)em->emission_rate_remaining;
        if (particles_to_emit + total_emitted > max_requests) {
            particles_to_emit = max_requests - total_emitted;
        }

        em->emission_rate_remaining -= (float)particles_to_emit;

        // Generate spawn requests
        for (uint32_t p = 0; p < particles_to_emit; p++) {
            UpeParticleSpawnRequest *req = &out_requests[total_emitted];

            // Generate spawn point based on shape
            UpeRng *rng = &pool->rng;
            UpeVec3 local_pos;
            if (em->shape == UPE_EMITTER_POINT) {
                local_pos.x = 0.0f;
                local_pos.y = 0.0f;
                local_pos.z = 0.0f;
            } else if (em->shape == UPE_EMITTER_SPHERE) {
                local_pos = RandomPointInSphere(em->shape_scale, rng);
            } else if (em->shape == UPE_EMITTER_CONE) {
                local_pos = RandomPointInCone(em->shape_scale, em->shape_scale * 0.5f, rng);
            } else {  // BOX
                local_pos = RandomPointInBox(em->shape_scale * 0.5f, rng);
            }

            // Transform to world space
            UpeVec3 rotated_pos = UpeQuat_RotateVec3(em->rotation, local_pos);
            req->position = UpeVec3_Add(em->position, rotated_pos);

            // Generate velocity with spread
            UpeVec3 base_vel = em->particle_template.velocity;
            UpeVec3 random_spread;
            random_spread.x = (Rand01(rng) - 0.5f) * 2.0f * em->velocity_spread;
            random_spread.y = (Rand01(rng) - 0.5f) * 2.0f * em->velocity_spread;
            random_spread.z = (Rand01(rng) - 0.5f) * 2.0f * em->velocity_spread;

            UpeVec3 local_vel = UpeVec3_Add(base_vel, random_spread);
            req->velocity = UpeQuat_RotateVec3(em->rotation, local_vel);

            // Particle properties from template
            req->lifetime = em->particle_template.lifetime;
            req->mass = em->particle_template.mass;
            req->size = em->particle_template.size;
            req->drag = em->particle_template.drag;

            total_emitted++;
        }
    }

    pool->particles_emitted_this_frame = total_emitted;
    return total_emitted;
}

// Debug print
void UpeEmitterPool_DebugPrint(UpeEmitterPool *pool, uint32_t emitter_idx)
{
    if (!pool || emitter_idx >= pool->emitter_count) {
        return;
    }

    UpeEmitter *em = &pool->emitters[emitter_idx];
    
    const char *shape_names[] = { "POINT", "SPHERE", "CONE", "BOX" };
    const char *mode_names[] = { "CONTINUOUS", "BURST", "TIMED" };

    printf("Emitter %u:\n", emitter_idx);
    printf("  Shape: %s, Mode: %s\n", shape_names[em->shape], mode_names[em->mode]);
    printf("  Active: %s, Time: %.2f/%.2f s\n", em->is_active ? "yes" : "no",
           em->time_active, em->emission_duration);
    printf("  Position: (%.2f, %.2f, %.2f)\n", em->position.x, em->position.y, em->position.z);
    printf("  Rate: %u particles/frame (pending: %.1f)\n", 
           em->particles_per_frame, em->emission_rate_remaining);
    printf("  Properties: lifetime=%.2f, mass=%.2f, size=%.2f\n",
           em->particle_template.lifetime, em->particle_template.mass, em->particle_template.size);
}

