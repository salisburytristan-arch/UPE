#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Material Point Method (MPM)
  
  - Hybrid Lagrangian-Eulerian method
  - Excellent for snow, sand, granular flows
  - Large deformations, topology changes
  - Based on Stomakhin et al. 2013 (Disney snow)
*/

#define UPE_MPM_MAX_PARTICLES 1000000

typedef struct UpeMpmParticle {
    UpeVec3 position;
    UpeVec3 velocity;
    UpeMatrix3 deformation_gradient; // F (deformation state)
    float mass;
    float volume;
    
    // Material properties
    float youngs_modulus;
    float poissons_ratio;
    float hardening;             // For snow (critical compression/stretch)
    
    uint32_t flags;
} UpeMpmParticle;

typedef struct UpeMpmGrid {
    UpeVec3 *velocity;           // Grid node velocities
    float *mass;                 // Grid node masses
    UpeVec3 *force;              // Forces on grid nodes
    
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    float cell_size;
    UpeVec3 origin;
    
} UpeMpmGrid;

typedef struct UpeMpmSystem {
    UpeMpmParticle *particles;
    uint32_t particle_count;
    uint32_t particle_capacity;
    
    UpeMpmGrid grid;
    
    UpeVec3 gravity;
    float dt_cfl_factor;         // CFL timestep coefficient
    
    // Material model
    uint32_t material_model;     // 0=elastic, 1=snow, 2=sand
    
} UpeMpmSystem;

void UpeMpm_Init(UpeMpmSystem *mpm, uint32_t grid_width, uint32_t grid_height, uint32_t grid_depth,
                float cell_size, uint32_t max_particles);
void UpeMpm_Destroy(UpeMpmSystem *mpm);

// Particle management
uint32_t UpeMpm_AddParticle(UpeMpmSystem *mpm, UpeVec3 position, UpeVec3 velocity, float mass);
void UpeMpm_CreateSnowball(UpeMpmSystem *mpm, UpeVec3 center, float radius, uint32_t particle_count);
void UpeMpm_CreateSandPile(UpeMpmSystem *mpm, UpeVec3 base, float height, uint32_t particle_count);

// Simulation
void UpeMpm_Step(UpeMpmSystem *mpm, float dt);

// MPM stages
void UpeMpm_ParticlesToGrid(UpeMpmSystem *mpm);        // P2G transfer
void UpeMpm_ComputeGridForces(UpeMpmSystem *mpm);
void UpeMpm_UpdateGrid(UpeMpmSystem *mpm, float dt);
void UpeMpm_GridBoundaryConditions(UpeMpmSystem *mpm);
void UpeMpm_GridToParticles(UpeMpmSystem *mpm, float dt); // G2P transfer
void UpeMpm_UpdateDeformationGradient(UpeMpmSystem *mpm, float dt);

// Constitutive models
void UpeMpm_ComputeStress_Elastic(const UpeMpmParticle *p, UpeMatrix3 *stress);
void UpeMpm_ComputeStress_Snow(const UpeMpmParticle *p, UpeMatrix3 *stress);
void UpeMpm_ComputeStress_DruckerPrager(const UpeMpmParticle *p, UpeMatrix3 *stress); // Sand

#ifdef __cplusplus
}
#endif
