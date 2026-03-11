#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// ELECTROMAGNETISM HEADER
//
// - Maxwell equations (simplified grid-based)
// - Lorentz force: F = q(E + v × B)
// - Charged particle integration with Boris algorithm
// - Grid-based EM field storage and trilinear interpolation
// - Plasma physics (cyclotron frequency, plasma frequency)
// ============================================================================

// Electromagnetic grid structure
typedef struct UpeEMGrid {
    UpeVec3 *E;         // Electric field at cell centers
    UpeVec3 *B;         // Magnetic field at cell centers
    float *rho;         // Charge density
    UpeVec3 *J;         // Current density (A/m²)
    
    uint32_t res_x, res_y, res_z;
    float cell_size;
} UpeEMGrid;

// Charged particle representation for plasma/beam simulations
typedef struct UpeEMChargedParticle {
    float charge;           // Couloumbs
    float mass;             // kg
    float velocity_mag;     // Current speed
    float cyclotron_freq;   // ω_c = qB/m (rad/s)
    float cyclotron_radius; // r_c = v/ω_c (meters)
} UpeEMChargedParticle;

// Grid management
void UpeEM_GridInit(UpeEMGrid *grid, uint32_t res_x, uint32_t res_y, uint32_t res_z, float cell_size);
void UpeEM_GridDestroy(UpeEMGrid *grid);

// Field setup
void UpeEM_SetUniformField(UpeEMGrid *grid, UpeVec3 E, UpeVec3 B);

// Lorentz force: F = q(E + v × B)
UpeVec3 UpeEM_ComputeLorentzForce(float charge, UpeVec3 velocity, UpeVec3 E, UpeVec3 B);

// Charged particle integration (Boris algorithm)
void UpeEM_StepChargedParticle(
    UpeVec3 *pos, UpeVec3 *vel,
    float mass, float charge, float dt,
    UpeVec3 E, UpeVec3 B);

// Field interpolation at arbitrary positions (trilinear)
UpeVec3 UpeEM_InterpolateE(const UpeEMGrid *grid, UpeVec3 pos);
UpeVec3 UpeEM_InterpolateB(const UpeEMGrid *grid, UpeVec3 pos);

// Plasma physics
void UpeEM_ComputeCyclotronFrequency(UpeEMChargedParticle *particle, UpeVec3 B);
void UpeEM_ComputePlasmaFrequency(float charge_density, float particle_mass, float *out_plasma_freq);

// Energy
float UpeEM_ComputeFieldEnergy(const UpeEMGrid *grid);
float UpeEM_ComputeParticleEnergy(float mass, UpeVec3 velocity);

/* Ohmic heating power density  P = J^2 / sigma  [W/m^3] */
float UpeEM_OhmicHeatPower(float J_density, float conductivity);

/* Eddy-current braking linear deceleration [m/s^2] */
float UpeEM_EddyBrakingAccel(float B, float sigma, float thickness, float velocity);

/* EMP field at distance: rough E-field [V/m] and B-field [T] */
void  UpeEM_EMPulseField(float energy_J, float distance_m,
                          float *out_E_V_m, float *out_B_T);

/* Dielectric breakdown check: returns 1 if field exceeds threshold */
int   UpeEM_ESDBreakdownCheck(float E_field_V_m, float dielectric_strength_V_m);

/* Simplified BH ferromagnetic curve (Frohlich-Kennelly model) */
float UpeEM_FerromagneticBH(float H_field, float B_sat, float mu_r);

/* Solenoid inductance  L = mu0 * mu_r * N^2 * A / l  [H] */
float UpeEM_InductanceSolenoid(float N_turns, float area_m2, float length_m);

#ifdef __cplusplus
}
#endif
