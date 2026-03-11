#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Granular Materials - Discrete Element Method (DEM)
  
  Simulates individual grains (sand, gravel, debris, powder)
  with contact forces, friction, and cohesion
*/

#define UPE_DEM_MAX_PARTICLES 65536
#define UPE_DEM_MAX_CONTACTS 262144

typedef struct UpeDemParticle {
    UpeVec3 position;
    UpeVec3 velocity;
    UpeVec3 angular_velocity;
    float radius;
    float mass;
    float inv_mass;
    float restitution;
    float friction;
    float cohesion;          // Adhesion strength (for wet/sticky materials)
    uint32_t flags;
} UpeDemParticle;

typedef struct UpeDemContact {
    uint32_t particle_a;
    uint32_t particle_b;
    UpeVec3 normal;          // Contact normal (A to B)
    float penetration;       // Overlap depth
    UpeVec3 tangent_impulse; // Accumulated tangential impulse (friction)
} UpeDemContact;

typedef struct UpeDemSystem {
    UpeDemParticle *particles;
    uint32_t particle_count;
    uint32_t particle_capacity;
    
    UpeDemContact *contacts;
    uint32_t contact_count;
    uint32_t contact_capacity;
    
    // Spatial hashing for broadphase
    uint32_t *grid_cells;
    uint32_t *particle_cell_ids;
    uint32_t grid_size;
    float cell_size;
    
    // Material parameters
    float default_stiffness;     // Contact stiffness (N/m)
    float default_damping;       // Contact damping
    float default_friction;      // Coulomb friction
    float default_cohesion;      // Cohesion (for wet materials)
    
    // Simulation
    UpeVec3 gravity;
    float dt_fixed;
    uint32_t substeps;
    
    // Boundaries
    UpeVec3 bounds_min;
    UpeVec3 bounds_max;
} UpeDemSystem;

// System management
void UpeDem_Init(UpeDemSystem *dem, uint32_t max_particles);
void UpeDem_Destroy(UpeDemSystem *dem);

// Particle management
uint32_t UpeDem_AddParticle(UpeDemSystem *dem, UpeVec3 position, float radius, float density);
void UpeDem_RemoveParticle(UpeDemSystem *dem, uint32_t index);

// Simulation
void UpeDem_Step(UpeDemSystem *dem, float dt);

// Utilities
void UpeDem_SpawnPile(UpeDemSystem *dem, UpeVec3 center, float radius, uint32_t count, float particle_radius);
void UpeDem_ApplyForceRegion(UpeDemSystem *dem, UpeVec3 center, float radius, UpeVec3 force);

// Material presets
void UpeDem_SetMaterialDrySand(UpeDemSystem *dem);
void UpeDem_SetMaterialWetSand(UpeDemSystem *dem);
void UpeDem_SetMaterialGravel(UpeDemSystem *dem);
void UpeDem_SetMaterialSnow(UpeDemSystem *dem);

#ifdef __cplusplus
}
#endif
