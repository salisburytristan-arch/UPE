#pragma once

#include <stdint.h>
#include "upe/upe_math.h"
#include "upe/upe_body.h"
#include "upe/upe_fluid_sph.h"

#ifdef __cplusplus
extern "C" {
#endif

// Buoyancy system for fluid-rigid body coupling
// Computes buoyancy forces based on submerged volume and local fluid density

#define UPE_BUOYANCY_MAX_SAMPLES 32

typedef struct UpeBuoyancyDesc {
    float fluid_density;           // Density of fluid (kg/m³, e.g., 1000 for water)
    UpeVec3 fluid_surface_normal;  // Up vector for fluid surface
    float fluid_surface_height;    // Y position of fluid surface
    float drag_coefficient;        // Fluid drag coefficient (0.5-1.0)
    float angular_drag;            // Rotational drag factor
    uint32_t sample_points;        // Number of volume samples (for precision)
} UpeBuoyancyDesc;

typedef struct UpeBuoyancyState {
    float submerged_volume;        // Current submerged volume (m³)
    float submerged_ratio;         // Fraction submerged (0-1)
    UpeVec3 center_of_buoyancy;    // Center of submerged volume
    UpeVec3 buoyancy_force;        // Total buoyancy force
    UpeVec3 drag_force;            // Fluid drag force
    UpeVec3 torque;                // Torque from buoyancy
} UpeBuoyancyState;

// Apply buoyancy to a single body in a uniform fluid
void UpeBuoyancy_ApplyUniform(
    UpeBody* body,
    const UpeBuoyancyDesc* desc,
    UpeBuoyancyState* state_out
);

// Apply buoyancy from SPH fluid (spatially varying density)
void UpeBuoyancy_ApplySph(
    UpeBody* body,
    const UpeFluidSph* fluid,
    float drag_coeff,
    UpeBuoyancyState* state_out
);

// Compute submerged volume for common shapes
float UpeBuoyancy_ComputeSubmergedVolume_Sphere(
    UpeVec3 center, float radius,
    UpeVec3 surface_normal, float surface_height
);

float UpeBuoyancy_ComputeSubmergedVolume_Box(
    UpeVec3 center, UpeQuat orientation, UpeVec3 half_extents,
    UpeVec3 surface_normal, float surface_height,
    uint32_t sample_count
);

float UpeBuoyancy_ComputeSubmergedVolume_Capsule(
    UpeVec3 center, UpeQuat orientation, float radius, float half_height,
    UpeVec3 surface_normal, float surface_height,
    uint32_t sample_count
);

// Helper: Calculate center of buoyancy for submerged volume
UpeVec3 UpeBuoyancy_CalculateCenterOfBuoyancy(
    const UpeBody* body,
    UpeVec3 surface_normal,
    float surface_height,
    uint32_t sample_count
);

#ifdef __cplusplus
}
#endif
