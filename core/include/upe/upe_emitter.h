#pragma once

#include <stdint.h>
#include "upe/upe_math.h"
#include "upe/upe_rng.h"

#ifdef __cplusplus
extern "C" {
#endif

// Particle emitter: spawn particles with initial conditions, lifetime, forces
// Integrates with existing particle system for collision/update

typedef enum UpeEmitterShape {
    UPE_EMITTER_POINT,      // Single point source
    UPE_EMITTER_SPHERE,     // Spherical volume
    UPE_EMITTER_CONE,       // Cone-shaped spray
    UPE_EMITTER_BOX,        // Box volume
} UpeEmitterShape;

typedef enum UpeEmitterMode {
    UPE_EMITTER_CONTINUOUS, // Emit every frame
    UPE_EMITTER_BURST,      // Single emission event
    UPE_EMITTER_TIMED,      // Emit for duration then stop
} UpeEmitterMode;

typedef struct UpeParticleInitial {
    UpeVec3 velocity;       // Initial velocity
    float lifetime;         // Time until particle dies
    float mass;             // Particle mass
    float size;             // Visual size
    float drag;             // Air resistance
} UpeParticleInitial;

typedef struct UpeEmitterDesc {
    UpeEmitterShape shape;
    UpeEmitterMode mode;
    
    // Position and orientation
    UpeVec3 position;
    UpeQuat rotation;
    
    // Emission rate
    uint32_t particles_per_frame;
    float emission_duration;  // For TIMED mode
    
    // Particle properties
    UpeParticleInitial particle_template;
    
    // Volume/spread
    float shape_scale;        // Radius for sphere, height for cone, etc.
    float velocity_spread;    // Random velocity variation (0-1)
    
    // Forces
    UpeVec3 gravity;
    UpeVec3 wind_velocity;
    float damping;
} UpeEmitterDesc;

typedef struct UpeEmitter {
    uint32_t id;
    UpeEmitterShape shape;
    UpeEmitterMode mode;
    
    // Transform
    UpeVec3 position;
    UpeQuat rotation;
    
    // State
    float time_active;        // Time since start
    float emission_duration;  // Total lifetime for TIMED mode
    bool is_active;
    
    // Emission parameters
    uint32_t particles_per_frame;
    float emission_rate_remaining;  // Partial particle accumulation
    
    // Particle template
    UpeParticleInitial particle_template;
    
    // Volume
    float shape_scale;
    float velocity_spread;
    
    // Forces
    UpeVec3 gravity;
    UpeVec3 wind_velocity;
    float damping;
} UpeEmitter;

typedef struct UpeEmitterPool {
    uint32_t id;
    
    UpeEmitter *emitters;
    uint32_t emitter_count;
    uint32_t max_emitters;
    
    // Particle sink capacity
    uint32_t total_particle_capacity;
    uint32_t particles_emitted_this_frame;

    UpeRng rng;
} UpeEmitterPool;

// Create emitter pool
UpeEmitterPool *UpeEmitterPool_Create(uint32_t max_emitters, uint32_t max_particles_per_frame);

// Destroy emitter pool
void UpeEmitterPool_Destroy(UpeEmitterPool *pool);

// Create and add emitter (returns emitter index or -1)
uint32_t UpeEmitterPool_AddEmitter(UpeEmitterPool *pool, const UpeEmitterDesc *desc);

// Remove emitter
void UpeEmitterPool_RemoveEmitter(UpeEmitterPool *pool, uint32_t emitter_idx);

// Get emitter
UpeEmitter *UpeEmitterPool_GetEmitter(UpeEmitterPool *pool, uint32_t emitter_idx);

// Update all emitters (step time, determine emission count)
// Called once per physics frame
void UpeEmitterPool_Update(UpeEmitterPool *pool, float dt);

// Query particles to emit this frame
// Returns particle initial conditions array
// Caller is responsible for creating actual particles
typedef struct {
    UpeVec3 position;
    UpeVec3 velocity;
    float lifetime;
    float mass;
    float size;
    float drag;
} UpeParticleSpawnRequest;

// Get emission queue for current frame
// Returns number of particles and fills out_requests array
uint32_t UpeEmitterPool_GetEmissionQueue(UpeEmitterPool *pool, 
                                         UpeParticleSpawnRequest *out_requests,
                                         uint32_t max_requests);

// Force immediate burst emission
void UpeEmitterPool_EmitBurst(UpeEmitterPool *pool, uint32_t emitter_idx, uint32_t count);

// Stop/start emitter
void UpeEmitterPool_SetActive(UpeEmitterPool *pool, uint32_t emitter_idx, bool active);

// Update emitter transform (for moving emitters like engine exhaust)
void UpeEmitterPool_SetTransform(UpeEmitterPool *pool, uint32_t emitter_idx, 
                                 UpeVec3 position, UpeQuat rotation);

// Update emitter forces (wind, gravity changes)
void UpeEmitterPool_SetForces(UpeEmitterPool *pool, uint32_t emitter_idx,
                              UpeVec3 gravity, UpeVec3 wind);

// Query active emitter count
uint32_t UpeEmitterPool_GetActiveCount(UpeEmitterPool *pool);

// Debug: dump emitter state
void UpeEmitterPool_DebugPrint(UpeEmitterPool *pool, uint32_t emitter_idx);

// Deterministic seed for emitter randomness
void UpeEmitterPool_SetSeed(UpeEmitterPool *pool, uint64_t seed);

#ifdef __cplusplus
}
#endif

