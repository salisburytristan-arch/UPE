#pragma once

#include <stdint.h>
#include "upe/upe_math.h"
#include "upe/upe_fluid_sph.h"
#include "upe/upe_body.h"

#ifdef __cplusplus
extern "C" {
#endif

// Fluid-body coupling parameters
typedef struct UpeFluidCouplingParams {
    float buoyancy_strength;        // Multiplier for buoyancy force (default 1.0)
    float drag_coefficient;         // Drag in water (default 0.5)
    float water_density;            // Density of water (default 1000 kg/m3)
    float water_level;              // Y-plane of water surface
    float wetness_decay_rate;       // How fast wetness dries (per second)
    int use_two_way_coupling;       // 1=bodies affect water, 0=one-way
} UpeFluidCouplingParams;

// Fluid-body coupling system
typedef struct UpeFluidCoupling {
    UpeFluidCouplingParams params;
    float *body_wetness;            // Wetness factor per body (0-1)
    uint32_t body_capacity;
} UpeFluidCoupling;

// Lifecycle
void UpeFluidCoupling_Init(UpeFluidCoupling *coupling, uint32_t max_bodies);
void UpeFluidCoupling_Destroy(UpeFluidCoupling *coupling);

// Apply fluid coupling forces to a body
void UpeFluidCoupling_ApplyForcesToBody(UpeFluidCoupling *coupling,
                                       UpeFluidSph *fluid,
                                       UpeBody *body,
                                       uint32_t body_idx,
                                       float dt);

// Update all couplings for a set of bodies
void UpeFluidCoupling_Update(UpeFluidCoupling *coupling,
                            UpeFluidSph *fluid,
                            UpeBody *bodies,
                            uint32_t body_count,
                            float dt);

// Set coupling parameters
void UpeFluidCoupling_SetParams(UpeFluidCoupling *coupling,
                               const UpeFluidCouplingParams *params);

// Query: Get body wetness factor
float UpeFluidCoupling_GetBodyWetness(const UpeFluidCoupling *coupling, uint32_t body_idx);

// Query: Check if body is submerged
int UpeFluidCoupling_IsBodySubmerged(const UpeFluidCoupling *coupling,
                                     const UpeBody *body);

// Query: Calculate buoyancy force for a partially submerged body
UpeVec3 UpeFluidCoupling_CalculateBuoyancyForce(const UpeFluidCoupling *coupling,
                                                const UpeBody *body,
                                                float submerged_volume);

// Query: Calculate drag force for a body moving through fluid
UpeVec3 UpeFluidCoupling_CalculateDragForce(const UpeFluidCoupling *coupling,
                                            const UpeBody *body);

#ifdef __cplusplus
}
#endif
