#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UPE_SPH_MAX_PARTICLES 8192
#define UPE_SPH_MAX_NEIGHBORS 64

typedef struct UpeSphParticle {
    UpeVec3 position;
    UpeVec3 velocity;
    UpeVec3 force;
    float density;
    float pressure;
    float mass;
    uint32_t grid_hash;
} UpeSphParticle;

typedef struct UpeFluidSph {
    UpeSphParticle *particles;
    uint32_t count;
    uint32_t capacity;
    float smoothing_radius;
    float rest_density;
    float gas_constant;
    float viscosity;
    float surface_tension;
    float particle_mass;
    UpeVec3 gravity;
    UpeVec3 bounds_min;
    UpeVec3 bounds_max;
    
    // Spatial hash grid
    uint32_t *grid_cells;
    uint32_t *grid_counts;
    uint32_t *particle_cells;
    uint32_t grid_size[3];
    float grid_cell_size;
    
    // Precomputed kernel coefficients
    float poly6_coeff;
    float spiky_coeff;
    float viscosity_coeff;
} UpeFluidSph;

// Core API
void UpeFluidSph_Init(UpeFluidSph *fluid, uint32_t max_particles);
void UpeFluidSph_SetBounds(UpeFluidSph *fluid, UpeVec3 min, UpeVec3 max);
void UpeFluidSph_SetGravity(UpeFluidSph *fluid, UpeVec3 gravity);
uint32_t UpeFluidSph_AddParticle(UpeFluidSph *fluid, UpeVec3 position, UpeVec3 velocity);
void UpeFluidSph_RemoveParticle(UpeFluidSph *fluid, uint32_t index);
void UpeFluidSph_Step(UpeFluidSph *fluid, float dt);
void UpeFluidSph_Destroy(UpeFluidSph *fluid);

// Internal steps (exposed for advanced use)
void UpeFluidSph_BuildSpatialHash(UpeFluidSph *fluid);
void UpeFluidSph_ComputeDensityPressure(UpeFluidSph *fluid);
void UpeFluidSph_ComputeForces(UpeFluidSph *fluid);
void UpeFluidSph_ComputeSurfaceTension(UpeFluidSph *fluid);
void UpeFluidSph_Integrate(UpeFluidSph *fluid, float dt);
void UpeFluidSph_HandleBoundaries(UpeFluidSph *fluid);

// Surface effects
uint32_t UpeFluidSph_DetectSurface(UpeFluidSph *fluid, float threshold);
uint32_t UpeFluidSph_GenerateSpray(UpeFluidSph *fluid, uint32_t count, float velocity_threshold);

#ifdef __cplusplus
}
#endif
