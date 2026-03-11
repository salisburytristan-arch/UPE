#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Shallow Water Equations
  
  - Height-field water simulation
  - Wave propagation (tsunami, rivers, lakes)
  - Dam break, flooding
  - Efficient large-scale water
*/

typedef struct UpeShallowWaterGrid {
    float *height;               // Water column height (h)
    float *velocity_x;           // Velocity in x direction (u)
    float *velocity_z;           // Velocity in z direction (v)
    float *terrain;              // Bathymetry (bottom elevation)
    
    uint32_t width;
    uint32_t depth;
    float cell_size;
    
    // Boundary conditions
    uint8_t *boundary_type;      // 0=open, 1=wall, 2=source
    
} UpeShallowWaterGrid;

typedef struct UpeShallowWaterSystem {
    UpeShallowWaterGrid grid;
    
    float gravity;
    float manning_coefficient;   // Surface roughness (friction)
    float min_height;            // Threshold for dry cells
    
    // Temporary buffers (for flux calculations)
    float *flux_x;
    float *flux_z;
    float *height_new;
    float *vel_x_new;
    float *vel_z_new;
    
} UpeShallowWaterSystem;

void UpeShallowWater_Init(UpeShallowWaterSystem *water, uint32_t grid_width, uint32_t grid_depth, float cell_size);
void UpeShallowWater_Destroy(UpeShallowWaterSystem *water);

// Simulation
void UpeShallowWater_Step(UpeShallowWaterSystem *water, float dt);

// Set terrain (bathymetry)
void UpeShallowWater_SetTerrain(UpeShallowWaterSystem *water, uint32_t x, uint32_t z, float elevation);

// Add water source
void UpeShallowWater_AddWater(UpeShallowWaterSystem *water, uint32_t x, uint32_t z, float amount);

// Query
float UpeShallowWater_GetHeight(const UpeShallowWaterSystem *water, uint32_t x, uint32_t z);
UpeVec2 UpeShallowWater_GetVelocity(const UpeShallowWaterSystem *water, uint32_t x, uint32_t z);

// Common scenarios
void UpeShallowWater_CreateDamBreak(UpeShallowWaterSystem *water, uint32_t dam_x);
void UpeShallowWater_CreateWave(UpeShallowWaterSystem *water, uint32_t source_x, uint32_t source_z, float amplitude);

#ifdef __cplusplus
}
#endif
