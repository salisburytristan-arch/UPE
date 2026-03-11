#include "upe/upe_acoustics.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

// ============================================================================
// ACOUSTICS SYSTEM
//
// - Acoustic wave equation on grid: ∂²p/∂t² = c² ∇²p
// - Sound propagation in 3D domains
// - Modal synthesis for impact sounds
// - Frequency-domain analysis and absorption
// - Sound pressure level tracking (dB SPL)
// ============================================================================

#define SPEED_OF_SOUND 343.0f  // m/s at 20°C in air
#define SOUND_REF_PRESSURE 2e-5f  // 20 μPa (reference for dB SPL)

// ============================================================================
// ACOUSTIC WAVE SOLVER
// ============================================================================

void UpeAcoustics_GridInit(UpeAcousticsGrid *grid, uint32_t res_x, uint32_t res_y, uint32_t res_z, float cell_size) {
    if (!grid) return;
    
    grid->res_x = res_x;
    grid->res_y = res_y;
    grid->res_z = res_z;
    grid->cell_size = cell_size;
    grid->speed_of_sound = SPEED_OF_SOUND;
    
    uint32_t num_cells = res_x * res_y * res_z;
    
    // Pressure field and time derivatives
    grid->pressure = (float*)calloc(num_cells, sizeof(float));
    grid->pressure_vel = (float*)calloc(num_cells, sizeof(float));
    grid->pressure_accel = (float*)calloc(num_cells, sizeof(float));
    
    // Absorption coefficient (0 = reflective, 1 = fully absorbing)
    grid->absorption = (float*)calloc(num_cells, sizeof(float));
}

void UpeAcoustics_GridDestroy(UpeAcousticsGrid *grid) {
    if (!grid) return;
    free(grid->pressure);
    free(grid->pressure_vel);
    free(grid->pressure_accel);
    free(grid->absorption);
    memset(grid, 0, sizeof(UpeAcousticsGrid));
}

// Cell index from coordinates
static uint32_t AcousticsGrid_Index(const UpeAcousticsGrid *grid, uint32_t x, uint32_t y, uint32_t z) {
    if (x >= grid->res_x) x = grid->res_x - 1;
    if (y >= grid->res_y) y = grid->res_y - 1;
    if (z >= grid->res_z) z = grid->res_z - 1;
    return z * grid->res_y * grid->res_x + y * grid->res_x + x;
}

// Set spatially uniform absorption (0 = reflective, 1 = absorbing)
void UpeAcoustics_SetAbsorption(UpeAcousticsGrid *grid, float absorption) {
    if (!grid || absorption < 0.0f || absorption > 1.0f) return;
    
    uint32_t num_cells = grid->res_x * grid->res_y * grid->res_z;
    for (uint32_t i = 0; i < num_cells; ++i) {
        grid->absorption[i] = absorption;
    }
}

// Excite grid with impulse at position
void UpeAcoustics_ExcitePulse(UpeAcousticsGrid *grid, UpeVec3 pos, float amplitude) {
    if (!grid) return;
    
    uint32_t x = (uint32_t)(pos.x / grid->cell_size);
    uint32_t y = (uint32_t)(pos.y / grid->cell_size);
    uint32_t z = (uint32_t)(pos.z / grid->cell_size);
    
    if (x < grid->res_x && y < grid->res_y && z < grid->res_z) {
        uint32_t idx = AcousticsGrid_Index(grid, x, y, z);
        grid->pressure[idx] = amplitude;
        grid->pressure_vel[idx] = 0.0f;
    }
}

// ============================================================================
// EXPLICIT WAVE EQUATION SOLVER (Finite Differences)
// ============================================================================

// Leap-frog time integration of wave equation
// p(t+dt) = 2*p(t) - p(t-dt) + c²*dt² * ∇²p(t)
void UpeAcoustics_StepFDTD(UpeAcousticsGrid *grid, float dt) {
    if (!grid || dt <= 0.0f) return;
    
    float c = grid->speed_of_sound;
    float dx = grid->cell_size;
    float stability_factor = (c * dt / dx) * (c * dt / dx);
    
    // Stability check: stability_factor <= 1/6 for 3D
    if (stability_factor > 0.15f) return;
    
    // Compute Laplacian via finite differences (central differences)
    uint32_t nx = grid->res_x, ny = grid->res_y, nz = grid->res_z;
    
    for (uint32_t z = 1; z < nz - 1; ++z) {
        for (uint32_t y = 1; y < ny - 1; ++y) {
            for (uint32_t x = 1; x < nx - 1; ++x) {
                uint32_t idx = AcousticsGrid_Index(grid, x, y, z);
                
                // Second derivatives (central difference: (f[i-1] - 2*f[i] + f[i+1]) / dx²)
                float p_xx = (grid->pressure[AcousticsGrid_Index(grid, x - 1, y, z)] -
                              2.0f * grid->pressure[idx] +
                              grid->pressure[AcousticsGrid_Index(grid, x + 1, y, z)]) / (dx * dx);
                
                float p_yy = (grid->pressure[AcousticsGrid_Index(grid, x, y - 1, z)] -
                              2.0f * grid->pressure[idx] +
                              grid->pressure[AcousticsGrid_Index(grid, x, y + 1, z)]) / (dx * dx);
                
                float p_zz = (grid->pressure[AcousticsGrid_Index(grid, x, y, z - 1)] -
                              2.0f * grid->pressure[idx] +
                              grid->pressure[AcousticsGrid_Index(grid, x, y, z + 1)]) / (dx * dx);
                
                float laplacian = p_xx + p_yy + p_zz;
                
                // Leap-frog update with absorption damping
                float alpha = 1.0f - grid->absorption[idx] * 0.1f;
                float p_new = 2.0f * grid->pressure[idx] - grid->pressure_vel[idx] +
                              c * c * dt * dt * laplacian;
                
                // Apply absorption (pressure exponential decay)
                p_new *= alpha;
                
                // Shift time steps
                grid->pressure_accel[idx] = (p_new - 2.0f * grid->pressure[idx] + grid->pressure_vel[idx]) / (dt * dt);
                grid->pressure_vel[idx] = grid->pressure[idx];
                grid->pressure[idx] = p_new;
            }
        }
    }
    
    // Boundary conditions (Dirichlet: p = 0 at boundaries)
    for (uint32_t i = 0; i < grid->res_x * grid->res_y * grid->res_z; ++i) {
        uint32_t z = i / (grid->res_y * grid->res_x);
        uint32_t y = (i % (grid->res_y * grid->res_x)) / grid->res_x;
        uint32_t x = i % grid->res_x;
        
        if (x == 0 || x == grid->res_x - 1 ||
            y == 0 || y == grid->res_y - 1 ||
            z == 0 || z == grid->res_z - 1) {
            grid->pressure[i] = 0.0f;
            grid->pressure_vel[i] = 0.0f;
        }
    }
}

// ============================================================================
// MODAL SYNTHESIS FOR IMPACT SOUNDS
// ============================================================================

// Initialize modal filter with resonant frequency and damping
void UpeAcoustics_InitModalFilter(
    UpeAcousticsModalFilter *filter,
    float frequency, float damping, float amplitude)
{
    if (!filter) return;
    
    filter->frequency = frequency;
    filter->damping = damping;
    filter->peak_amplitude = amplitude;
    
    // Compute filter coefficients (IIR resonant filter)
    // H(z) = (1-z⁻²) / (1 - 2*cos(ω)*z⁻¹ + z⁻²)
    float omega = 2.0f * 3.14159265f * frequency / 44100.0f;
    float cos_omega = cosf(omega);
    float sin_omega = sinf(omega);
    
    // Biquad coefficients
    filter->b0 = 1.0f;
    filter->b1 = 0.0f;
    filter->b2 = -1.0f;
    
    float Q = 1.0f / (2.0f * (1.0f - damping));
    float alpha = sin_omega / (2.0f * Q);
    
    float denom = 1.0f + alpha;
    filter->a1 = -2.0f * cos_omega / denom;
    filter->a2 = (1.0f - alpha) / denom;
    
    filter->x1 = 0.0f;
    filter->x2 = 0.0f;
    filter->y1 = 0.0f;
    filter->y2 = 0.0f;
}

// Process audio signal through modal filter
float UpeAcoustics_ModalFilterStep(UpeAcousticsModalFilter *filter, float input) {
    if (!filter) return 0.0f;
    
    // Biquad filter: y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
    float output = filter->b0 * input + filter->b1 * filter->x1 + filter->b2 * filter->x2 -
                   filter->a1 * filter->y1 - filter->a2 * filter->y2;
    
    // Shift states
    filter->x2 = filter->x1;
    filter->x1 = input;
    filter->y2 = filter->y1;
    filter->y1 = output;
    
    return output * filter->peak_amplitude;
}

// ============================================================================
// FREQUENCY DOMAIN ANALYSIS
// ============================================================================

// Compute sound pressure level (SPL) in decibels
// SPL = 20 * log10(p / p_ref) where p_ref = 20 μPa
float UpeAcoustics_ComputeSPL(float pressure) {
    if (pressure < SOUND_REF_PRESSURE) pressure = SOUND_REF_PRESSURE;
    return 20.0f * log10f(pressure / SOUND_REF_PRESSURE);
}

// Compute RMS pressure over domain
float UpeAcoustics_ComputeRMSPressure(const UpeAcousticsGrid *grid) {
    if (!grid) return 0.0f;
    
    uint32_t num_cells = grid->res_x * grid->res_y * grid->res_z;
    float sum_sq = 0.0f;
    
    for (uint32_t i = 0; i < num_cells; ++i) {
        float p = grid->pressure[i];
        sum_sq += p * p;
    }
    
    return sqrtf(sum_sq / num_cells);
}

// Estimate dominant frequency via peak detection in frequency domain
float UpeAcoustics_EstimateDominantFrequency(
    const UpeAcousticsModalFilter *filters, uint32_t num_filters)
{
    if (!filters || num_filters == 0) return 0.0f;
    
    float max_amplitude = 0.0f;
    float dominant_freq = 0.0f;
    
    for (uint32_t i = 0; i < num_filters; ++i) {
        if (filters[i].peak_amplitude > max_amplitude) {
            max_amplitude = filters[i].peak_amplitude;
            dominant_freq = filters[i].frequency;
        }
    }
    
    return dominant_freq;
}

// ============================================================================
// SOUND PROPAGATION AND ATTENUATION
// ============================================================================

// Compute geometric attenuation (inverse square law): A(r) = 1 / r²
float UpeAcoustics_GeometricAttenuation(float distance) {
    if (distance < 1e-6f) distance = 1e-6f;
    return 1.0f / (distance * distance);
}

// Compute atmospheric absorption (frequency and humidity dependent)
// Simplified: absorption increases with frequency
float UpeAcoustics_AtmosphericAbsorption(float frequency, float distance, float humidity) {
    // Simplified model: higher frequencies attenuate more
    float freq_factor = (frequency / 1000.0f) * (frequency / 1000.0f);  // Frequency dependency
    float absorption_coeff = 0.001f * freq_factor;  // dB/meter
    
    return expf(-absorption_coeff * distance);
}

// Compute total attenuation factor for sound propagation
float UpeAcoustics_ComputeAttenuation(
    float distance, float frequency, float humidity)
{
    float geometric = UpeAcoustics_GeometricAttenuation(distance);
    float atmospheric = UpeAcoustics_AtmosphericAbsorption(frequency, distance, humidity);
    
    return geometric * atmospheric;
}

// ============================================================================
// ENERGY AND POWER
// ============================================================================

// Compute acoustic intensity: I = p² / (ρ * c)
// ρ ≈ 1.2 kg/m³ (air), c ≈ 343 m/s
float UpeAcoustics_ComputeIntensity(float pressure) {
    const float RHO = 1.2f;
    return (pressure * pressure) / (RHO * SPEED_OF_SOUND);
}

// Total acoustic energy in domain
float UpeAcoustics_ComputeTotalEnergy(const UpeAcousticsGrid *grid) {
    if (!grid) return 0.0f;
    
    uint32_t num_cells = grid->res_x * grid->res_y * grid->res_z;
    float total_energy = 0.0f;
    float cell_vol = grid->cell_size * grid->cell_size * grid->cell_size;
    
    for (uint32_t i = 0; i < num_cells; ++i) {
        // Energy density: (1/2) * ρ * v²
        // where v ≈ p / (ρ * c) for acoustic velocity
        float energy_density = (grid->pressure[i] * grid->pressure[i]) / (1.2f * SPEED_OF_SOUND);
        total_energy += energy_density * cell_vol;
    }
    
    return total_energy;
}

// Sound power (energy flux through surface)
float UpeAcoustics_ComputeSoundPower(const UpeAcousticsGrid *grid, uint32_t axis) {
    if (!grid || axis > 2) return 0.0f;
    
    // Integrate intensity over cross-sectional area
    float power = 0.0f;
    
    if (axis == 0) {  // x-axis
        for (uint32_t y = 0; y < grid->res_y; ++y) {
            for (uint32_t z = 0; z < grid->res_z; ++z) {
                uint32_t idx = AcousticsGrid_Index(grid, 1, y, z);
                float intensity = UpeAcoustics_ComputeIntensity(grid->pressure[idx]);
                power += intensity * grid->cell_size * grid->cell_size;
            }
        }
    } else if (axis == 1) {  // y-axis
        for (uint32_t x = 0; x < grid->res_x; ++x) {
            for (uint32_t z = 0; z < grid->res_z; ++z) {
                uint32_t idx = AcousticsGrid_Index(grid, x, 1, z);
                float intensity = UpeAcoustics_ComputeIntensity(grid->pressure[idx]);
                power += intensity * grid->cell_size * grid->cell_size;
            }
        }
    } else {  // z-axis
        for (uint32_t x = 0; x < grid->res_x; ++x) {
            for (uint32_t y = 0; y < grid->res_y; ++y) {
                uint32_t idx = AcousticsGrid_Index(grid, x, y, 1);
                float intensity = UpeAcoustics_ComputeIntensity(grid->pressure[idx]);
                power += intensity * grid->cell_size * grid->cell_size;
            }
        }
    }
    
    return power;
}
