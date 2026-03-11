#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Gas & Smoke Simulation (Eulerian Grid-Based)
  
  - Navier-Stokes on grid
  - Temperature-driven buoyancy
  - Vorticity confinement (for swirling smoke)
  - Advection schemes (Semi-Lagrangian, MacCormack)
*/

typedef struct UpeGasGrid {
    float *velocity_x;           // Staggered MAC grid
    float *velocity_y;
    float *velocity_z;
    
    float *density;              // Smoke/gas density
    float *temperature;          // Temperature field
    float *pressure;
    
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    float cell_size;
    UpeVec3 origin;
    
} UpeGasGrid;

typedef struct UpeGasSystem {
    UpeGasGrid grid;
    
    float ambient_temperature;
    float buoyancy_strength;     // How much heat rises
    float vorticity_strength;    // Vorticity confinement
    float dissipation_rate;      // Smoke fades over time
    float viscosity;             // Fluid viscosity
    
    // Sources/sinks
    UpeVec3 *smoke_sources;
    float *source_densities;
    float *source_temperatures;
    uint32_t source_count;
    
    // Boundaries
    uint8_t *obstacle_mask;      // 1 = solid, 0 = fluid
    
    // Temporary buffers
    float *velocity_x_temp;
    float *velocity_y_temp;
    float *velocity_z_temp;
    float *density_temp;
    
} UpeGasSystem;

void UpeGas_Init(UpeGasSystem *gas, uint32_t width, uint32_t height, uint32_t depth, float cell_size);
void UpeGas_Destroy(UpeGasSystem *gas);

// Simulation step
void UpeGas_Step(UpeGasSystem *gas, float dt);

// Individual stages
void UpeGas_AddForces(UpeGasSystem *gas, float dt);          // Gravity, buoyancy
void UpeGas_AdvectVelocity(UpeGasSystem *gas, float dt);     // Self-advection
void UpeGas_AdvectDensity(UpeGasSystem *gas, float dt);
void UpeGas_AdvectTemperature(UpeGasSystem *gas, float dt);
void UpeGas_VorticityConfinement(UpeGasSystem *gas, float dt);
void UpeGas_SolvePressure(UpeGasSystem *gas, float dt);      // Make divergence-free
void UpeGas_ApplyDissipation(UpeGasSystem *gas, float dt);

// Sources
void UpeGas_AddSource(UpeGasSystem *gas, UpeVec3 position, float density, float temperature);

// Obstacles
void UpeGas_SetObstacle(UpeGasSystem *gas, uint32_t x, uint32_t y, uint32_t z, int is_solid);

// Query
float UpeGas_GetDensity(const UpeGasSystem *gas, uint32_t x, uint32_t y, uint32_t z);
UpeVec3 UpeGas_GetVelocity(const UpeGasSystem *gas, float world_x, float world_y, float world_z);

#ifdef __cplusplus
}
#endif
