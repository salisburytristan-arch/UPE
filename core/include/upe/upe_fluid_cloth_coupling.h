#pragma once

#include <stdint.h>
#include "upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct UpeFluidSph UpeFluidSph;

/* Fluid-Cloth Coupling System
 * 
 * Enables realistic interaction between fluids and deformable bodies:
 * - Cloth interaction with water (buoyancy, drag, splashing)
 * - Hair simulation in wind fields
 * - Soft body deformation from fluid pressure
 * - Two-way coupling: fluid affects cloth, cloth affects fluid
 * - Splashing and spray generation from cloth motion
 */

typedef struct UpeFluidClothCoupling UpeFluidClothCoupling;

typedef struct {
    float buoyancy_ratio;       /* How much buoyancy vs drag [0, 1] */
    float drag_coefficient;     /* Fluid resistance multiplier */
    float damping;              /* Velocity damping in fluid */
    int create_spray;           /* Generate spray particles on emission */
    int two_way_coupling;       /* Affects fluid particles back */
} UpeFluidClothCouplingParams;

/* Create coupling system */
UpeFluidClothCoupling *UpeFluidClothCoupling_Create(uint32_t max_cloth_particles, 
                                                     uint32_t max_fluid_particles);
void UpeFluidClothCoupling_Destroy(UpeFluidClothCoupling *coupling);

/* Register cloth particles for coupling */
void UpeFluidClothCoupling_RegisterClothMesh(UpeFluidClothCoupling *coupling,
    UpeVec3 *particles, uint32_t particle_count, float particle_mass);

/* Register SPH fluid system */
void UpeFluidClothCoupling_RegisterFluid(UpeFluidClothCoupling *coupling,
    UpeFluidSph *fluid);

/* Set coupling parameters */
void UpeFluidClothCoupling_SetParams(UpeFluidClothCoupling *coupling,
    const UpeFluidClothCouplingParams *params);

/* Execute coupling step (call between cloth and fluid updates) */
void UpeFluidClothCoupling_Step(UpeFluidClothCoupling *coupling, float dt);

/* Query interaction statistics */
typedef struct {
    uint32_t cloth_particles_in_fluid;
    float total_buoyancy_force;
    float total_drag_force;
    uint32_t spray_particles_generated;
} UpeFluidClothCouplingStats;

void UpeFluidClothCoupling_GetStats(UpeFluidClothCoupling *coupling,
    UpeFluidClothCouplingStats *out_stats);

#ifdef __cplusplus
}
#endif
