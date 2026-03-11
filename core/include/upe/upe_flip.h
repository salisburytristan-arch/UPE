#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  FLIP/PIC Fluid Solver (Fluid-Implicit-Particle / Particle-In-Cell)
  
  - Hybrid Eulerian-Lagrangian approach
  - Better energy conservation than pure SPH
  - Grid for pressure solve, particles for advection
  - Ideal for visual effects (splashes, foam)
*/

#define UPE_FLIP_MAX_PARTICLES 1000000

typedef struct UpeFlipParticle {
    UpeVec3 position;
    UpeVec3 velocity;
    float mass;
    uint32_t flags;
} UpeFlipParticle;

typedef struct UpeFlipGrid {
    // MAC grid (staggered velocities)
    float *velocity_x;           // U component (face-centered)
    float *velocity_y;           // V component
    float *velocity_z;           // W component
    
    float *pressure;             // Cell-centered
    uint8_t *cell_type;          // FLUID=1, SOLID=2, AIR=0
    
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    float cell_size;
    UpeVec3 origin;
} UpeFlipGrid;

typedef struct UpeFlipSystem {
    UpeFlipParticle *particles;
    uint32_t particle_count;
    uint32_t particle_capacity;
    
    UpeFlipGrid grid;
    
    float flip_ratio;            // 0 = PIC (smooth), 1 = FLIP (energetic)
    float gravity;
    float density;
    
    // Temporary buffers for velocity transfer
    float *grid_velocity_old_x;
    float *grid_velocity_old_y;
    float *grid_velocity_old_z;
    float *grid_weights;
    
} UpeFlipSystem;

void UpeFlip_Init(UpeFlipSystem *flip, uint32_t grid_width, uint32_t grid_height, uint32_t grid_depth,
                  float cell_size, uint32_t max_particles);
void UpeFlip_Destroy(UpeFlipSystem *flip);

// Particle management
uint32_t UpeFlip_AddParticle(UpeFlipSystem *flip, UpeVec3 position, UpeVec3 velocity);
void UpeFlip_RemoveParticle(UpeFlipSystem *flip, uint32_t particle_id);

// Simulation step
void UpeFlip_Step(UpeFlipSystem *flip, float dt);

// Individual stages
void UpeFlip_TransferToGrid(UpeFlipSystem *flip);           // Particle → Grid
void UpeFlip_ClassifyCells(UpeFlipSystem *flip);            // Mark fluid/solid/air
void UpeFlip_AddGravity(UpeFlipSystem *flip, float dt);
void UpeFlip_SolvePressure(UpeFlipSystem *flip, float dt);  // Incompressibility
void UpeFlip_TransferToParticles(UpeFlipSystem *flip);      // Grid → Particles (PIC + FLIP)
void UpeFlip_AdvectParticles(UpeFlipSystem *flip, float dt);

// Utilities
void UpeFlip_CreateFluidBox(UpeFlipSystem *flip, UpeVec3 min_corner, UpeVec3 max_corner);

#ifdef __cplusplus
}
#endif
