#pragma once

#include <stdint.h>
#include "upe/upe_math.h"
#include "upe/upe_fluid_sph.h"
#include "upe/upe_emitter.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  SPH Fluid Spray/Foam Emission
  
  Generates secondary particles (spray, foam, bubbles) from SPH fluid 
  based on physical criteria:
  - High velocity particles emit spray
  - High curvature (surface tension) regions emit foam
  - Trapped air pockets emit bubbles
*/

typedef struct UpeFluidSprayParams {
    // Spray emission (high velocity)
    float spray_velocity_threshold;   // m/s (default: 2.0)
    float spray_emission_rate;        // particles per m/s above threshold (default: 0.5)
    float spray_velocity_multiplier;  // velocity scaling factor (default: 1.0)
    float spray_size;                 // particle size (default: 0.05)
    float spray_lifetime;             // seconds (default: 1.0)
    
    // Foam emission (surface tension/curvature)
    float foam_curvature_threshold;   // 1/m (default: 10.0)
    float foam_emission_rate;         // particles per frame (default: 0.2)
    float foam_size;                  // particle size (default: 0.03)
    float foam_lifetime;              // seconds (default: 2.0)
    
    // Bubble emission (negative pressure)
    float bubble_pressure_threshold;  // Pa (default: -500)
    float bubble_emission_rate;       // particles per frame (default: 0.1)
    float bubble_size;                // particle size (default: 0.04)
    float bubble_lifetime;            // seconds (default: 3.0)
    float bubble_buoyancy;            // upward velocity (default: 0.5)
    
    // Global controls
    float emission_probability;       // 0-1 (default: 0.3)
    uint32_t max_emissions_per_frame; // Cap total emissions (default: 100)
} UpeFluidSprayParams;

typedef struct UpeFluidSpraySystem {
    UpeFluidSph *fluid;               // Source SPH fluid
    UpeEmitterPool *emitter_pool;     // Target emitter pool for secondary particles
    UpeFluidSprayParams params;
    
    // Runtime state
    uint32_t frame_counter;
    uint32_t total_spray_emitted;
    uint32_t total_foam_emitted;
    uint32_t total_bubbles_emitted;
} UpeFluidSpraySystem;

// Initialize spray system
void UpeFluidSpray_Init(UpeFluidSpraySystem *spray, 
                        UpeFluidSph *fluid, 
                        UpeEmitterPool *emitter_pool);

// Set default parameters
void UpeFluidSpray_SetDefaultParams(UpeFluidSprayParams *params);

// Update spray emission (call per frame after SPH step)
void UpeFluidSpray_Update(UpeFluidSpraySystem *spray, float dt);

// Query statistics
uint32_t UpeFluidSpray_GetTotalEmitted(const UpeFluidSpraySystem *spray);

// Get emission counts by type
void UpeFluidSpray_GetEmissionStats(const UpeFluidSpraySystem *spray,
                                    uint32_t *spray_count,
                                    uint32_t *foam_count,
                                    uint32_t *bubble_count);

#ifdef __cplusplus
}
#endif
