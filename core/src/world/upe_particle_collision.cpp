#include "upe/upe_particle_collision.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

// Spatial hash helper functions
static uint32_t UpeParticleCollision_HashPosition(UpeVec3 pos, float cell_size) {
    int32_t x = (int32_t)(pos.x / cell_size);
    int32_t y = (int32_t)(pos.y / cell_size);
    int32_t z = (int32_t)(pos.z / cell_size);
    
    // Simple hash function (multiplicative hash)
    uint32_t h = (uint32_t)x;
    h = h * 73856093 ^ (uint32_t)y;
    h = h * 19349663 ^ (uint32_t)z;
    return h;
}

static void UpeParticleCollision_ClearHash(UpeParticleCollisionSystem *sys) {
    for (uint32_t i = 0; i < sys->hash_grid_size; ++i) {
        sys->hash_grid[i].count = 0;
    }
}

static void UpeParticleCollision_InsertParticle(UpeParticleCollisionSystem *sys, uint32_t particle_idx, UpeVec3 pos) {
    uint32_t hash = UpeParticleCollision_HashPosition(pos, sys->cell_size) % sys->hash_grid_size;
    UpeSpatialHashCell *cell = &sys->hash_grid[hash];
    
    // Grow cell if needed
    if (cell->count >= cell->capacity) {
        uint32_t new_capacity = cell->capacity == 0 ? 8 : cell->capacity * 2;
        uint32_t *new_indices = (uint32_t *)malloc(new_capacity * sizeof(uint32_t));
        if (cell->particle_indices && cell->count > 0) {
            memcpy(new_indices, cell->particle_indices, cell->count * sizeof(uint32_t));
            free(cell->particle_indices);
        }
        cell->particle_indices = new_indices;
        cell->capacity = new_capacity;
    }
    
    cell->particle_indices[cell->count++] = particle_idx;
}

void UpeParticleCollisionSystem_Init(UpeParticleCollisionSystem *sys, uint32_t max_contacts)
{
    if (!sys) {
        return;
    }

    sys->contact_capacity = max_contacts > 0 ? max_contacts : 1000;
    sys->contact_count = 0;
    sys->contacts = (UpeParticleContact *)malloc(sys->contact_capacity * sizeof(UpeParticleContact));

    // Default parameters
    sys->params.ground_level = 0.0f;
    sys->params.particle_radius = 0.05f;
    sys->params.default_restitution = 0.5f;
    sys->params.default_friction = 0.4f;
    sys->params.resting_velocity_threshold = 0.1f;
    sys->params.use_ground_plane = 1;
    sys->params.use_particle_collision = 1;
    sys->params.repulsion_strength = 1.0f;
    
    // Initialize spatial hash (grid for particle-particle collision)
    sys->hash_grid_size = 64;  // 64x64x64 grid
    sys->cell_size = 0.5f;      // 0.5m cells
    sys->hash_grid = (UpeSpatialHashCell *)malloc(sys->hash_grid_size * sizeof(UpeSpatialHashCell));
    for (uint32_t i = 0; i < sys->hash_grid_size; ++i) {
        sys->hash_grid[i].particle_indices = NULL;
        sys->hash_grid[i].count = 0;
        sys->hash_grid[i].capacity = 0;
    }
}

void UpeParticleCollisionSystem_Destroy(UpeParticleCollisionSystem *sys)
{
    if (!sys) {
        return;
    }
    if (sys->contacts) {
        free(sys->contacts);
        sys->contacts = NULL;
    }
    if (sys->hash_grid) {
        for (uint32_t i = 0; i < sys->hash_grid_size; ++i) {
            if (sys->hash_grid[i].particle_indices) {
                free(sys->hash_grid[i].particle_indices);
            }
        }
        free(sys->hash_grid);
        sys->hash_grid = NULL;
    }
    sys->contact_count = 0;
    sys->contact_capacity = 0;
}

void UpeParticleCollisionSystem_SetParams(UpeParticleCollisionSystem *sys,
                                         const UpeParticleCollisionParams *params)
{
    if (!sys || !params) {
        return;
    }
    sys->params = *params;
}

uint32_t UpeParticleCollisionSystem_GetContactCount(const UpeParticleCollisionSystem *sys)
{
    if (!sys) return 0;
    return sys->contact_count;
}

UpeParticleContact *UpeParticleCollisionSystem_GetContact(UpeParticleCollisionSystem *sys,
                                                         uint32_t idx)
{
    if (!sys || idx >= sys->contact_count) {
        return NULL;
    }
    return &sys->contacts[idx];
}

int UpeParticleCollisionSystem_IsParticleInContact(const UpeParticleCollisionSystem *sys,
                                                   uint32_t particle_idx)
{
    if (!sys) {
        return 0;
    }
    for (uint32_t i = 0; i < sys->contact_count; ++i) {
        if (sys->contacts[i].particle_idx == particle_idx) {
            return 1;
        }
    }
    return 0;
}

UpeParticleContact *UpeParticleCollisionSystem_GetParticleContact(UpeParticleCollisionSystem *sys,
                                                                 uint32_t particle_idx)
{
    if (!sys) {
        return NULL;
    }
    for (uint32_t i = 0; i < sys->contact_count; ++i) {
        if (sys->contacts[i].particle_idx == particle_idx) {
            return &sys->contacts[i];
        }
    }
    return NULL;
}

void UpeParticleCollisionSystem_Update(UpeParticleCollisionSystem *sys, 
                                      UpeParticleSystem *particles, 
                                      float dt)
{
    if (!sys || !particles) {
        return;
    }

    // Clear previous contacts
    sys->contact_count = 0;
    
    // Clear spatial hash
    if (sys->params.use_particle_collision) {
        UpeParticleCollision_ClearHash(sys);
        
        // Build spatial hash
        for (uint32_t i = 0; i < particles->count; ++i) {
            UpeParticle *p = &particles->particles[i];
            if (p->life > 0.0f && !(p->flags & UPE_PARTICLE_FLAG_POOLED)) {
                UpeParticleCollision_InsertParticle(sys, i, p->position);
            }
        }
    }

    // Process each particle
    for (uint32_t i = 0; i < particles->count; ++i) {
        UpeParticle *p = &particles->particles[i];

        // Skip dead or pooled particles
        if (p->life <= 0.0f || (p->flags & UPE_PARTICLE_FLAG_POOLED)) {
            continue;
        }
        
        // Particle-particle collision (soft repulsion)
        if (sys->params.use_particle_collision) {
            uint32_t hash = UpeParticleCollision_HashPosition(p->position, sys->cell_size) % sys->hash_grid_size;
            UpeSpatialHashCell *cell = &sys->hash_grid[hash];
            
            UpeVec3 repulsion_force = {0.0f, 0.0f, 0.0f};
            
            // Check nearby particles in same cell
            for (uint32_t c = 0; c < cell->count; ++c) {
                uint32_t other_idx = cell->particle_indices[c];
                if (other_idx == i) continue;  // Skip self
                
                UpeParticle *other = &particles->particles[other_idx];
                if (other->life <= 0.0f || (other->flags & UPE_PARTICLE_FLAG_POOLED)) {
                    continue;
                }
                
                // Compute distance
                UpeVec3 delta = {
                    p->position.x - other->position.x,
                    p->position.y - other->position.y,
                    p->position.z - other->position.z
                };
                float dist_sq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
                float min_dist = sys->params.particle_radius * 2.0f;
                float min_dist_sq = min_dist * min_dist;
                
                if (dist_sq < min_dist_sq && dist_sq > 1e-6f) {
                    // Particles are overlapping - apply soft repulsion
                    float dist = sqrtf(dist_sq);
                    float overlap = min_dist - dist;
                    
                    // Normalize direction
                    float inv_dist = 1.0f / dist;
                    delta.x *= inv_dist;
                    delta.y *= inv_dist;
                    delta.z *= inv_dist;
                    
                    // Repulsion force proportional to overlap
                    float force_mag = overlap * sys->params.repulsion_strength;
                    repulsion_force.x += delta.x * force_mag;
                    repulsion_force.y += delta.y * force_mag;
                    repulsion_force.z += delta.z * force_mag;
                }
            }
            
            // Apply repulsion force (inverse mass scaling)
            if (p->mass > 0.0f) {
                float inv_mass = 1.0f / p->mass;
                p->velocity.x += repulsion_force.x * inv_mass * dt;
                p->velocity.y += repulsion_force.y * inv_mass * dt;
                p->velocity.z += repulsion_force.z * inv_mass * dt;
            }
        }

        // Check ground plane collision
        if (sys->params.use_ground_plane) {
            float ground_y = sys->params.ground_level;
            float particle_bottom = p->position.y - sys->params.particle_radius;

            if (particle_bottom < ground_y) {
                // Particle is below ground - collision!
                if (sys->contact_count >= sys->contact_capacity) {
                    // Grow contact array if needed
                    uint32_t new_capacity = sys->contact_capacity * 2;
                    UpeParticleContact *new_contacts = (UpeParticleContact *)malloc(
                        new_capacity * sizeof(UpeParticleContact)
                    );
                    if (new_contacts) {
                        memcpy(new_contacts, sys->contacts, 
                               sys->contact_count * sizeof(UpeParticleContact));
                        free(sys->contacts);
                        sys->contacts = new_contacts;
                        sys->contact_capacity = new_capacity;
                    }
                }

                if (sys->contact_count < sys->contact_capacity) {
                    UpeParticleContact *contact = &sys->contacts[sys->contact_count++];
                    contact->particle_idx = i;
                    contact->contact_point = p->position;
                    contact->contact_point.y = ground_y;
                    contact->contact_normal = {0.0f, 1.0f, 0.0f};
                    contact->penetration = ground_y - particle_bottom;
                    contact->restitution = sys->params.default_restitution;
                    contact->object_type = 0;  // Ground

                    // Mark particle as colliding
                    p->flags |= UPE_PARTICLE_FLAG_COLLIDING;

                    // Bounce response with restitution
                    float penetration_depth = ground_y - particle_bottom;
                    
                    // Push particle out of ground
                    p->position.y += penetration_depth + 0.001f;

                    // Apply bounce (reverse vertical velocity with restitution)
                    float bounce_factor = contact->restitution;
                    if (p->velocity.y < 0.0f) {
                        p->velocity.y = -p->velocity.y * bounce_factor;
                        
                        // Apply friction to horizontal velocity
                        float friction = sys->params.default_friction;
                        p->velocity.x *= (1.0f - friction * dt);
                        p->velocity.z *= (1.0f - friction * dt);
                    }

                    // Check if particle is resting
                    float vel_mag = sqrtf(p->velocity.x * p->velocity.x + 
                                         p->velocity.y * p->velocity.y + 
                                         p->velocity.z * p->velocity.z);
                    if (vel_mag < sys->params.resting_velocity_threshold) {
                        p->flags |= UPE_PARTICLE_FLAG_RESTING;
                        // Stop small movements
                        p->velocity.x = 0.0f;
                        p->velocity.y = 0.0f;
                        p->velocity.z = 0.0f;
                    }
                }
            } else {
                // Clear collision flags if not colliding
                p->flags &= ~UPE_PARTICLE_FLAG_COLLIDING;
                p->flags &= ~UPE_PARTICLE_FLAG_RESTING;
            }
        }

        // Apply gravity to non-resting particles
        if (!(p->flags & UPE_PARTICLE_FLAG_RESTING)) {
            p->velocity.y -= 9.8f * dt;  // Gravity
        }

        // Integrate position
        p->position.x += p->velocity.x * dt;
        p->position.y += p->velocity.y * dt;
        p->position.z += p->velocity.z * dt;

        // Decay lifetime
        p->life -= dt;
    }
}
