#include "upe/upe_electromagnetism.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

// ============================================================================
// ELECTROMAGNETISM SYSTEM
//
// - Maxwell equations on grid (simplified)
// - Lorentz force: F = q(E + v × B)
// - Charged particle integration in EM fields
// - Magnetic flux divergence-free constraint
// - Faraday and Ampere-Maxwell laws
// ============================================================================

#define C_LIGHT 3e8f  // Speed of light (m/s)
#define MU_0 1.25663706e-6f  // Permeability of free space
#define EPS_0 8.85418782e-12f  // Permittivity of free space

// ============================================================================
// ELECTROMAGNETIC FIELD MANAGEMENT
// ============================================================================

void UpeEM_GridInit(UpeEMGrid *grid, uint32_t res_x, uint32_t res_y, uint32_t res_z, float cell_size) {
    if (!grid) return;
    
    grid->res_x = res_x;
    grid->res_y = res_y;
    grid->res_z = res_z;
    grid->cell_size = cell_size;
    
    // Allocate field arrays (staggered grid: E at cell centers, B at edges)
    uint32_t num_cells = res_x * res_y * res_z;
    grid->E = (UpeVec3*)calloc(num_cells, sizeof(UpeVec3));
    grid->B = (UpeVec3*)calloc(num_cells, sizeof(UpeVec3));
    grid->rho = (float*)calloc(num_cells, sizeof(float));  // Charge density
    grid->J = (UpeVec3*)calloc(num_cells, sizeof(UpeVec3)); // Current density
}

void UpeEM_GridDestroy(UpeEMGrid *grid) {
    if (!grid) return;
    free(grid->E);
    free(grid->B);
    free(grid->rho);
    free(grid->J);
    memset(grid, 0, sizeof(UpeEMGrid));
}

// Cell index from coordinates
static uint32_t EMGrid_Index(const UpeEMGrid *grid, uint32_t x, uint32_t y, uint32_t z) {
    if (x >= grid->res_x) x = grid->res_x - 1;
    if (y >= grid->res_y) y = grid->res_y - 1;
    if (z >= grid->res_z) z = grid->res_z - 1;
    return z * grid->res_y * grid->res_x + y * grid->res_x + x;
}

// Set uniform field over grid
void UpeEM_SetUniformField(UpeEMGrid *grid, UpeVec3 E, UpeVec3 B) {
    if (!grid) return;
    
    uint32_t num_cells = grid->res_x * grid->res_y * grid->res_z;
    for (uint32_t i = 0; i < num_cells; ++i) {
        grid->E[i] = E;
        grid->B[i] = B;
    }
}

// ============================================================================
// LORENTZ FORCE COMPUTATION
// ============================================================================

// F = q(E + v × B)
UpeVec3 UpeEM_ComputeLorentzForce(float charge, UpeVec3 velocity, UpeVec3 E, UpeVec3 B) {
    // E component
    UpeVec3 force = {
        charge * E.x,
        charge * E.y,
        charge * E.z
    };
    
    // v × B component
    UpeVec3 cross = {
        velocity.y * B.z - velocity.z * B.y,
        velocity.z * B.x - velocity.x * B.z,
        velocity.x * B.y - velocity.y * B.x
    };
    
    force.x += charge * cross.x;
    force.y += charge * cross.y;
    force.z += charge * cross.z;
    
    return force;
}

// ============================================================================
// CHARGED PARTICLE INTEGRATION
// ============================================================================

void UpeEM_StepChargedParticle(
    UpeVec3 *pos, UpeVec3 *vel,
    float mass, float charge, float dt,
    UpeVec3 E, UpeVec3 B)
{
    if (!pos || !vel || mass <= 0.0f || dt <= 0.0f) return;
    
    // Half-step velocity update (Boris integrator for B-field accuracy)
    // This is more accurate for magnetic fields than standard Euler
    
    // v_minus = v + (q/m) * E * dt/2 (electric accelerate, half step)
    UpeVec3 v_minus = {
        vel->x + (charge / mass) * E.x * dt * 0.5f,
        vel->y + (charge / mass) * E.y * dt * 0.5f,
        vel->z + (charge / mass) * E.z * dt * 0.5f
    };
    
    // Magnetic field rotation (using Rodrigues' formula approximation)
    float omega_mag = (charge / mass) * sqrtf(B.x * B.x + B.y * B.y + B.z * B.z);
    float theta = omega_mag * dt;
    float sin_t = sinf(theta);
    float cos_t = cosf(theta);
    
    // Normalized B direction
    float B_mag = sqrtf(B.x * B.x + B.y * B.y + B.z * B.z);
    UpeVec3 B_hat = {0, 0, 0};
    if (B_mag > 1e-8f) {
        B_hat.x = B.x / B_mag;
        B_hat.y = B.y / B_mag;
        B_hat.z = B.z / B_mag;
    }
    
    // v_plus = v_minus rotated around B axis
    // Using small angle approximation: v_plus ≈ v_minus + (B_hat × v_minus) * sin(θ)
    UpeVec3 cross_B_v = {
        B_hat.y * v_minus.z - B_hat.z * v_minus.y,
        B_hat.z * v_minus.x - B_hat.x * v_minus.z,
        B_hat.x * v_minus.y - B_hat.y * v_minus.x
    };
    
    UpeVec3 v_plus = {
        v_minus.x * cos_t + cross_B_v.x * sin_t,
        v_minus.y * cos_t + cross_B_v.y * sin_t,
        v_minus.z * cos_t + cross_B_v.z * sin_t
    };
    
    // Final half-step electric accelerate
    UpeVec3 v_new = {
        v_plus.x + (charge / mass) * E.x * dt * 0.5f,
        v_plus.y + (charge / mass) * E.y * dt * 0.5f,
        v_plus.z + (charge / mass) * E.z * dt * 0.5f
    };
    
    // Position update
    UpeVec3 pos_new = {
        pos->x + v_new.x * dt,
        pos->y + v_new.y * dt,
        pos->z + v_new.z * dt
    };
    
    *pos = pos_new;
    *vel = v_new;
}

// ============================================================================
// GRID-BASED FIELD INTERPOLATION
// ============================================================================

// Trilinear interpolation of E field at position
UpeVec3 UpeEM_InterpolateE(const UpeEMGrid *grid, UpeVec3 pos) {
    if (!grid) return UpeVec3{0, 0, 0};
    
    // Normalize position to grid
    float x = pos.x / grid->cell_size;
    float y = pos.y / grid->cell_size;
    float z = pos.z / grid->cell_size;
    
    // Integer and fractional parts
    uint32_t ix = (uint32_t)x;
    uint32_t iy = (uint32_t)y;
    uint32_t iz = (uint32_t)z;
    float fx = x - ix;
    float fy = y - iy;
    float fz = z - iz;
    
    // Sample 8 corners
    UpeVec3 E000 = grid->E[EMGrid_Index(grid, ix,     iy,     iz)];
    UpeVec3 E100 = grid->E[EMGrid_Index(grid, ix + 1, iy,     iz)];
    UpeVec3 E010 = grid->E[EMGrid_Index(grid, ix,     iy + 1, iz)];
    UpeVec3 E110 = grid->E[EMGrid_Index(grid, ix + 1, iy + 1, iz)];
    UpeVec3 E001 = grid->E[EMGrid_Index(grid, ix,     iy,     iz + 1)];
    UpeVec3 E101 = grid->E[EMGrid_Index(grid, ix + 1, iy,     iz + 1)];
    UpeVec3 E011 = grid->E[EMGrid_Index(grid, ix,     iy + 1, iz + 1)];
    UpeVec3 E111 = grid->E[EMGrid_Index(grid, ix + 1, iy + 1, iz + 1)];
    
    // Trilinear blend
    float w000 = (1 - fx) * (1 - fy) * (1 - fz);
    float w100 = fx * (1 - fy) * (1 - fz);
    float w010 = (1 - fx) * fy * (1 - fz);
    float w110 = fx * fy * (1 - fz);
    float w001 = (1 - fx) * (1 - fy) * fz;
    float w101 = fx * (1 - fy) * fz;
    float w011 = (1 - fx) * fy * fz;
    float w111 = fx * fy * fz;
    
    UpeVec3 E_interp = {
        w000 * E000.x + w100 * E100.x + w010 * E010.x + w110 * E110.x +
        w001 * E001.x + w101 * E101.x + w011 * E011.x + w111 * E111.x,
        w000 * E000.y + w100 * E100.y + w010 * E010.y + w110 * E110.y +
        w001 * E001.y + w101 * E101.y + w011 * E011.y + w111 * E111.y,
        w000 * E000.z + w100 * E100.z + w010 * E010.z + w110 * E110.z +
        w001 * E001.z + w101 * E101.z + w011 * E011.z + w111 * E111.z
    };
    
    return E_interp;
}

// Trilinear interpolation of B field at position
UpeVec3 UpeEM_InterpolateB(const UpeEMGrid *grid, UpeVec3 pos) {
    if (!grid) return UpeVec3{0, 0, 0};
    
    // Similar to E field interpolation
    float x = pos.x / grid->cell_size;
    float y = pos.y / grid->cell_size;
    float z = pos.z / grid->cell_size;
    
    uint32_t ix = (uint32_t)x;
    uint32_t iy = (uint32_t)y;
    uint32_t iz = (uint32_t)z;
    float fx = x - ix;
    float fy = y - iy;
    float fz = z - iz;
    
    UpeVec3 B000 = grid->B[EMGrid_Index(grid, ix,     iy,     iz)];
    UpeVec3 B100 = grid->B[EMGrid_Index(grid, ix + 1, iy,     iz)];
    UpeVec3 B010 = grid->B[EMGrid_Index(grid, ix,     iy + 1, iz)];
    UpeVec3 B110 = grid->B[EMGrid_Index(grid, ix + 1, iy + 1, iz)];
    UpeVec3 B001 = grid->B[EMGrid_Index(grid, ix,     iy,     iz + 1)];
    UpeVec3 B101 = grid->B[EMGrid_Index(grid, ix + 1, iy,     iz + 1)];
    UpeVec3 B011 = grid->B[EMGrid_Index(grid, ix,     iy + 1, iz + 1)];
    UpeVec3 B111 = grid->B[EMGrid_Index(grid, ix + 1, iy + 1, iz + 1)];
    
    float w000 = (1 - fx) * (1 - fy) * (1 - fz);
    float w100 = fx * (1 - fy) * (1 - fz);
    float w010 = (1 - fx) * fy * (1 - fz);
    float w110 = fx * fy * (1 - fz);
    float w001 = (1 - fx) * (1 - fy) * fz;
    float w101 = fx * (1 - fy) * fz;
    float w011 = (1 - fx) * fy * fz;
    float w111 = fx * fy * fz;
    
    UpeVec3 B_interp = {
        w000 * B000.x + w100 * B100.x + w010 * B010.x + w110 * B110.x +
        w001 * B001.x + w101 * B101.x + w011 * B011.x + w111 * B111.x,
        w000 * B000.y + w100 * B100.y + w010 * B010.y + w110 * B110.y +
        w001 * B001.y + w101 * B101.y + w011 * B011.y + w111 * B111.y,
        w000 * B000.z + w100 * B100.z + w010 * B010.z + w110 * B110.z +
        w001 * B001.z + w101 * B101.z + w011 * B011.z + w111 * B111.z
    };
    
    return B_interp;
}

// ============================================================================
// PLASMA PHYSICS
// ============================================================================

void UpeEM_ComputeCyclotronFrequency(
    UpeEMChargedParticle *particle, UpeVec3 B)
{
    if (!particle) return;
    
    float B_mag = sqrtf(B.x * B.x + B.y * B.y + B.z * B.z);
    particle->cyclotron_freq = (particle->charge / particle->mass) * B_mag;
    particle->cyclotron_radius = particle->velocity_mag / (particle->cyclotron_freq + 1e-12f);
}

void UpeEM_ComputePlasmaFrequency(
    float charge_density, float particle_mass,
    float *out_plasma_freq)
{
    if (!out_plasma_freq) return;
    
    // ω_p = sqrt(ne*q^2 / (m*ε_0))
    float ne = charge_density / (1.602e-19f);  // Convert to particle number density
    *out_plasma_freq = sqrtf(ne * 1e-38f / (particle_mass * EPS_0));
}

// ============================================================================
// ENERGY COMPUTATION
// ============================================================================

// Electromagnetic field energy: (E^2 + c^2*B^2) / 2
float UpeEM_ComputeFieldEnergy(const UpeEMGrid *grid) {
    if (!grid) return 0.0f;
    
    float energy = 0.0f;
    uint32_t num_cells = grid->res_x * grid->res_y * grid->res_z;
    
    for (uint32_t i = 0; i < num_cells; ++i) {
        float E_mag_sq = grid->E[i].x * grid->E[i].x + grid->E[i].y * grid->E[i].y + grid->E[i].z * grid->E[i].z;
        float B_mag_sq = grid->B[i].x * grid->B[i].x + grid->B[i].y * grid->B[i].y + grid->B[i].z * grid->B[i].z;
        
        energy += 0.5f * EPS_0 * E_mag_sq;
        energy += 0.5f * B_mag_sq / MU_0;
    }
    
    // Normalize by volume
    float cell_vol = grid->cell_size * grid->cell_size * grid->cell_size;
    return energy * cell_vol;
}

// Kinetic energy of charged particle
float UpeEM_ComputeParticleEnergy(float mass, UpeVec3 velocity) {
    float v_sq = velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z;
    return 0.5f * mass * v_sq;
}

float UpeEM_OhmicHeatPower(float J_density, float conductivity) {
    if (conductivity <= 0.0f) return 0.0f;
    return (J_density * J_density) / conductivity;
}

float UpeEM_EddyBrakingAccel(float B, float sigma, float thickness, float velocity) {
    if (sigma <= 0.0f || thickness <= 0.0f) return 0.0f;
    /* a ~ k * sigma * B^2 * thickness * v */
    float k = 0.01f;
    float a = k * sigma * B * B * thickness * velocity;
    return (a >= 0.0f) ? -a : a;
}

void UpeEM_EMPulseField(float energy_J, float distance_m,
                          float *out_E_V_m, float *out_B_T) {
    if (!out_E_V_m || !out_B_T || energy_J <= 0.0f || distance_m <= 0.0f) {
        if (out_E_V_m) *out_E_V_m = 0.0f;
        if (out_B_T) *out_B_T = 0.0f;
        return;
    }
    float r2 = distance_m * distance_m;
    float flux = energy_J / (4.0f * 3.14159265f * r2);
    *out_E_V_m = sqrtf(fabsf(flux) / EPS_0);
    *out_B_T   = *out_E_V_m / C_LIGHT;
}

int UpeEM_ESDBreakdownCheck(float E_field_V_m, float dielectric_strength_V_m) {
    return (E_field_V_m >= dielectric_strength_V_m) ? 1 : 0;
}

float UpeEM_FerromagneticBH(float H_field, float B_sat, float mu_r) {
    /* Frohlich-Kennelly-like saturation: B = (mu0*mu_r*H) / (1 + |H|/Hk) */
    const float MU0 = 1.2566370614e-6f;
    if (mu_r <= 0.0f) mu_r = 1.0f;
    float Hk = (B_sat > 0.0f) ? (B_sat / (MU0 * mu_r)) : 1.0f;
    float B = (MU0 * mu_r * H_field) / (1.0f + fabsf(H_field) / Hk);
    if (B > B_sat) B = B_sat;
    if (B < -B_sat) B = -B_sat;
    return B;
}

float UpeEM_InductanceSolenoid(float N_turns, float area_m2, float length_m) {
    const float MU0 = 1.2566370614e-6f;
    if (N_turns <= 0.0f || area_m2 <= 0.0f || length_m <= 0.0f) return 0.0f;
    return MU0 * N_turns * N_turns * area_m2 / length_m;
}
