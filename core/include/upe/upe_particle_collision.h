#pragma once

#include <stdint.h>
#include "upe/upe_math.h"
#include "upe/upe_particles.h"

#ifdef __cplusplus
extern "C" {
#endif

// Particle collision flags
#define UPE_PARTICLE_FLAG_COLLIDING (1 << 0)
#define UPE_PARTICLE_FLAG_RESTING   (1 << 1)
#define UPE_PARTICLE_FLAG_POOLED    (1 << 2)

// Particle collision contact
typedef struct UpeParticleContact {
    uint32_t particle_idx;
    UpeVec3 contact_point;
    UpeVec3 contact_normal;
    float penetration;
    float restitution;  // Bounce coefficient
    int object_type;    // 0=ground, 1=body, 2=trigger
} UpeParticleContact;

// Particle collision parameters
typedef struct UpeParticleCollisionParams {
    float ground_level;             // Default y-plane for ground collision
    float particle_radius;          // Effective radius for collision
    float default_restitution;      // Default bounce coefficient (0-1)
    float default_friction;         // Default friction coefficient
    float resting_velocity_threshold; // Below this, particle is "resting"
    int use_ground_plane;           // 1=enable Y=0 ground collision
    int use_particle_collision;     // 1=enable particle-particle collision
    float repulsion_strength;       // Particle-particle repulsion force
} UpeParticleCollisionParams;

// Spatial hash cell for particle-particle collision
typedef struct UpeSpatialHashCell {
    uint32_t *particle_indices;
    uint32_t count;
    uint32_t capacity;
} UpeSpatialHashCell;

// Particle collision system
typedef struct UpeParticleCollisionSystem {
    UpeParticleContact *contacts;
    uint32_t contact_count;
    uint32_t contact_capacity;
    UpeParticleCollisionParams params;
    
    // Spatial hash for particle-particle collision
    UpeSpatialHashCell *hash_grid;
    uint32_t hash_grid_size;      // Number of cells in hash grid
    float cell_size;               // Size of each hash cell
} UpeParticleCollisionSystem;

// Lifecycle
void UpeParticleCollisionSystem_Init(UpeParticleCollisionSystem *sys, uint32_t max_contacts);
void UpeParticleCollisionSystem_Destroy(UpeParticleCollisionSystem *sys);

// Main collision detection & response
void UpeParticleCollisionSystem_Update(UpeParticleCollisionSystem *sys, 
                                      UpeParticleSystem *particles, 
                                      float dt);

// Set collision parameters
void UpeParticleCollisionSystem_SetParams(UpeParticleCollisionSystem *sys,
                                         const UpeParticleCollisionParams *params);

// Get collision count for this frame
uint32_t UpeParticleCollisionSystem_GetContactCount(const UpeParticleCollisionSystem *sys);

// Get specific contact
UpeParticleContact *UpeParticleCollisionSystem_GetContact(UpeParticleCollisionSystem *sys,
                                                         uint32_t idx);

// Query: Check if particle is in contact
int UpeParticleCollisionSystem_IsParticleInContact(const UpeParticleCollisionSystem *sys,
                                                   uint32_t particle_idx);

// Query: Get contact info for particle (returns contact or NULL if not colliding)
UpeParticleContact *UpeParticleCollisionSystem_GetParticleContact(UpeParticleCollisionSystem *sys,
                                                                 uint32_t particle_idx);

#ifdef __cplusplus
}
#endif
