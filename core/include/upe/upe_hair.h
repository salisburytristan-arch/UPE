#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Hair Simulation
  
  - Guide hair strands with interpolation
  - Hair-hair interaction (density-based)
  - Aerodynamic forces (wind response)
  - Collision with body/head geometry
*/

#define UPE_HAIR_MAX_STRANDS 100000
#define UPE_HAIR_MAX_PARTICLES_PER_STRAND 32

typedef struct UpeHairParticle {
    UpeVec3 position;
    UpeVec3 velocity;
    float mass;
    float radius;
} UpeHairParticle;

typedef struct UpeHairStrand {
    UpeHairParticle *particles;
    uint32_t particle_count;
    
    UpeVec3 root_position;       // Attached to scalp/body
    uint32_t root_bone_id;       // For skeletal attachment
    
    float stiffness;
    float damping;
    float length_compliance;     // XPBD compliance for length
    float bending_compliance;
    
    uint32_t flags;              // GUIDE_STRAND, INTERPOLATED, etc.
} UpeHairStrand;

typedef struct UpeHairSystem {
    UpeHairStrand *strands;
    uint32_t strand_count;
    uint32_t strand_capacity;
    
    // Guide strands (actually simulated)
    uint32_t *guide_indices;
    uint32_t guide_count;
    
    // Global parameters
    float gravity;
    UpeVec3 wind_velocity;
    float air_drag;
    float hair_friction;         // Hair-hair friction
    
    // Collision capsules (for head/body)
    UpeVec3 *capsule_starts;
    UpeVec3 *capsule_ends;
    float *capsule_radii;
    uint32_t capsule_count;
    
    // Density grid (for hair-hair interaction)
    float *density_grid;
    uint32_t grid_width;
    uint32_t grid_height;
    uint32_t grid_depth;
    float grid_cell_size;
    
} UpeHairSystem;

void UpeHair_Init(UpeHairSystem *hair, uint32_t max_strands);
void UpeHair_Destroy(UpeHairSystem *hair);

// Strand creation
uint32_t UpeHair_AddStrand(UpeHairSystem *hair, UpeVec3 root, UpeVec3 direction, float length,
                          uint32_t particle_count, int is_guide);
void UpeHair_AttachToBone(UpeHairSystem *hair, uint32_t strand_id, uint32_t bone_id);

// Simulation
void UpeHair_Step(UpeHairSystem *hair, float dt);
void UpeHair_IntegrateGuideStrands(UpeHairSystem *hair, float dt);
void UpeHair_SolveConstraints(UpeHairSystem *hair, uint32_t iterations);
void UpeHair_InterpolateStrands(UpeHairSystem *hair);  // Interpolate between guide strands
void UpeHair_ResolveCollisions(UpeHairSystem *hair);

// Forces
void UpeHair_ApplyWind(UpeHairSystem *hair, UpeVec3 wind, float dt);
void UpeHair_ApplyDensityForces(UpeHairSystem *hair);  // Hair-hair repulsion

// Collision volumes
void UpeHair_AddCollisionCapsule(UpeHairSystem *hair, UpeVec3 start, UpeVec3 end, float radius);

#ifdef __cplusplus
}
#endif
