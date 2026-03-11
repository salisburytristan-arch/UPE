#include "upe/upe_waves.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PI 3.14159265359f

// ============================================================================
// Basic Wave Grid (Shallow Water for near-field)
// ============================================================================

void UpeWaveGrid_Init(UpeWaveGrid *grid, uint32_t w, uint32_t h) {
    if (!grid) {
        return;
    }
    grid->width = w;
    grid->height = h;
    grid->wave_speed = 1.0f;
    grid->damping = 0.99f;
    uint32_t count = w * h;
    grid->wave_height = (float *)malloc(count * sizeof(float));
    grid->velocity = (float *)malloc(count * sizeof(float));
    memset(grid->wave_height, 0, count * sizeof(float));
    memset(grid->velocity, 0, count * sizeof(float));
}

void UpeWaveGrid_Step(UpeWaveGrid *grid, float dt) {
    if (!grid) {
        return;
    }
    
    for (uint32_t y = 1; y < grid->height - 1; ++y) {
        for (uint32_t x = 1; x < grid->width - 1; ++x) {
            uint32_t idx = y * grid->width + x;
            uint32_t left = idx - 1;
            uint32_t right = idx + 1;
            uint32_t up = idx - grid->width;
            uint32_t down = idx + grid->width;
            
            float laplacian = grid->wave_height[left] + grid->wave_height[right] + 
                            grid->wave_height[up] + grid->wave_height[down] - 
                            4.0f * grid->wave_height[idx];
            
            grid->velocity[idx] += laplacian * grid->wave_speed * dt;
            grid->velocity[idx] *= grid->damping;
        }
    }
    
    for (uint32_t i = 0; i < grid->width * grid->height; ++i) {
        grid->wave_height[i] += grid->velocity[i] * dt;
    }
}

void UpeWaveGrid_Disturb(UpeWaveGrid *grid, uint32_t x, uint32_t y, float magnitude) {
    if (!grid || x >= grid->width || y >= grid->height) {
        return;
    }
    uint32_t idx = y * grid->width + x;
    grid->wave_height[idx] += magnitude;
}

void UpeWaveGrid_Destroy(UpeWaveGrid *grid) {
    if (!grid) {
        return;
    }
    free(grid->wave_height);
    free(grid->velocity);
}

// ============================================================================
// FFT Ocean Spectrum (Phillips Spectrum for Deep Water Waves)
// ============================================================================

typedef struct UpeOceanSpectrum {
    float *spectrum;              // Precomputed Phillips spectrum
    uint32_t grid_size;           // Width/height of FFT grid
    float gravity;                // m/s² (9.81)
    float wind_speed;             // m/s (typical: 10-20)
    float wind_direction[2];      // Normalized (x, z)
    float smallest_wave;          // Wavelength cutoff
    float amplitude_scale;        // Overall amplitude
} UpeOceanSpectrum;

// Phillips spectrum: describes energy distribution of ocean waves
[[maybe_unused]] static float PhillipsSpectrum(float kx, float ky, float g, float V, float A) {
    // Phillips spectrum: S(k) = A * exp(-1/(kL)²) * exp(-k² * l²) * |w·k|²
    // Simplified version for performance
    
    float k_mag = sqrtf(kx * kx + ky * ky);
    if (k_mag < 0.001f) return 0.0f;
    
    float L = (V * V) / g;  // Largest wavelength
    float l = L / 25.0f;     // Smallest wavelength
    
    float k_dot_w = kx * 0.927f + ky * 0.371f;  // Assume wind direction
    bool is_with_wind = k_dot_w >= 0.0f;
    if (!is_with_wind) k_dot_w *= 0.25f;
    
    // Energy suppression
    float h = 1.0f / (k_mag * k_mag * l * l);
    if (h > 1.0f) h = 1.0f;
    
    float spectrum = A / (k_mag * k_mag * k_mag * k_mag);
    spectrum *= expf(-1.0f / (k_mag * L * k_mag * L));
    spectrum *= expf(-k_mag * k_mag * l * l);
    spectrum *= (k_dot_w * k_dot_w);
    
    return spectrum;
}

// ============================================================================
// FFT Ocean Simulation (Far-Field Water)
// ============================================================================

typedef struct UpeFFTOcean {
    // Ocean parameters
    float gravity;
    float wind_speed;
    float amplitude;
    
    // Grid for FFT evaluation
    uint32_t grid_size;           // Width x Height
    float grid_scale;             // Physical size per cell
    
    // Height field samples
    float *height_field;
    float *slope_field_x;
    float *slope_field_z;
    
    // Animation
    float time;
    float wave_amplitude_scale;
    
} UpeFFTOcean;

// Simple FFT Ocean initialization
void UpeFFTOcean_Create(UpeFFTOcean *ocean, uint32_t grid_size, float grid_scale) {
    if (!ocean) return;
    
    ocean->gravity = 9.81f;
    ocean->wind_speed = 15.0f;           // m/s
    ocean->amplitude = 0.5f;             // Max height
    ocean->grid_size = grid_size;
    ocean->grid_scale = grid_scale;      // Meters per cell
    ocean->time = 0.0f;
    ocean->wave_amplitude_scale = 1.0f;
    
    uint32_t total = grid_size * grid_size;
    ocean->height_field = (float*)malloc(total * sizeof(float));
    ocean->slope_field_x = (float*)malloc(total * sizeof(float));
    ocean->slope_field_z = (float*)malloc(total * sizeof(float));
    
    memset(ocean->height_field, 0, total * sizeof(float));
    memset(ocean->slope_field_x, 0, total * sizeof(float));
    memset(ocean->slope_field_z, 0, total * sizeof(float));
}

void UpeFFTOcean_Destroy(UpeFFTOcean *ocean) {
    if (!ocean) return;
    
    free(ocean->height_field);
    free(ocean->slope_field_x);
    free(ocean->slope_field_z);
    memset(ocean, 0, sizeof(*ocean));
}

// Simplified gerstner wave for FFT-like appearance without actual FFT
void UpeFFTOcean_UpdateGerstner(UpeFFTOcean *ocean, float dt) {
    if (!ocean || !ocean->height_field) return;
    
    ocean->time += dt;
    
    // Generate Gerstner waves (more realistic than simple sine)
    for (uint32_t y = 0; y < ocean->grid_size; ++y) {
        for (uint32_t x = 0; x < ocean->grid_size; ++x) {
            float world_x = (x - ocean->grid_size / 2) * ocean->grid_scale;
            float world_z = (y - ocean->grid_size / 2) * ocean->grid_scale;
            
            float height = 0.0f;
            float slope_x = 0.0f;
            float slope_z = 0.0f;
            
            // Superpose multiple waves (5 harmonics)
            float Q = 0.25f;  // Steepness factor
            float A = ocean->amplitude;
            
            for (uint32_t w = 0; w < 5; ++w) {
                float wavelength = 30.0f + w * 20.0f;  // Increasing wavelength
                float k = 2.0f * PI / wavelength;
                float c = sqrtf(ocean->gravity / k);
                
                // Wave direction angle (multiple directions)
                float angle = (float)w * PI / 2.5f;
                float dir_x = cosf(angle);
                float dir_z = sinf(angle);
                
                float dot = world_x * dir_x * k + world_z * dir_z * k - c * ocean->time;
                float amplitude_attenuated = A * (1.0f - w * 0.15f);
                
                // Gerstner wave contribution
                float cos_dot = cosf(dot);
                float sin_dot = sinf(dot);
                float Akc = amplitude_attenuated * k * Q;
                
                height += amplitude_attenuated * sin_dot;
                slope_x += Akc * dir_x * cos_dot;
                slope_z += Akc * dir_z * cos_dot;
            }
            
            uint32_t idx = y * ocean->grid_size + x;
            ocean->height_field[idx] = height * ocean->wave_amplitude_scale;
            ocean->slope_field_x[idx] = slope_x * ocean->wave_amplitude_scale;
            ocean->slope_field_z[idx] = slope_z * ocean->wave_amplitude_scale;
        }
    }
}

float UpeFFTOcean_GetHeight(UpeFFTOcean *ocean, float x, float z) {
    if (!ocean || !ocean->height_field) return 0.0f;
    
    // Sample height field with bilinear interpolation
    float grid_x = (x / ocean->grid_scale) + ocean->grid_size / 2.0f;
    float grid_z = (z / ocean->grid_scale) + ocean->grid_size / 2.0f;
    
    int ix = (int)grid_x;
    int iz = (int)grid_z;
    
    if (ix < 0 || ix >= (int)ocean->grid_size - 1 ||
        iz < 0 || iz >= (int)ocean->grid_size - 1) {
        return 0.0f;
    }
    
    float fx = grid_x - ix;
    float fz = grid_z - iz;
    
    float h00 = ocean->height_field[iz * ocean->grid_size + ix];
    float h10 = ocean->height_field[iz * ocean->grid_size + (ix + 1)];
    float h01 = ocean->height_field[(iz + 1) * ocean->grid_size + ix];
    float h11 = ocean->height_field[(iz + 1) * ocean->grid_size + (ix + 1)];
    
    float h0 = h00 * (1.0f - fx) + h10 * fx;
    float h1 = h01 * (1.0f - fx) + h11 * fx;
    
    return h0 * (1.0f - fz) + h1 * fz;
}

// ============================================================================
// Hybrid Water System (FFT far-field + SPH near-field)
// ============================================================================

typedef struct UpeHybridWater {
    // Near-field: SPH simulation
    void *sph_fluid;              // UpeFluidSph* (void* to avoid header dependency)
    float near_field_radius;      // Transition zone
    
    // Far-field: FFT Ocean
    UpeFFTOcean ocean;
    
    // Blending parameters
    float blend_distance;         // Transition zone width
    float far_field_amplitude;    // Ocean amplitude scale
    
} UpeHybridWater;

void UpeHybridWater_Init(UpeHybridWater *hybrid, void *sph_fluid, 
                         float near_field_radius,
                         uint32_t fft_grid_size, float fft_scale) {
    if (!hybrid) return;
    
    memset(hybrid, 0, sizeof(*hybrid));
    
    hybrid->sph_fluid = sph_fluid;
    hybrid->near_field_radius = near_field_radius;
    hybrid->blend_distance = near_field_radius * 0.3f;
    hybrid->far_field_amplitude = 1.0f;
    
    UpeFFTOcean_Create(&hybrid->ocean, fft_grid_size, fft_scale);
}

void UpeHybridWater_Destroy(UpeHybridWater *hybrid) {
    if (!hybrid) return;
    
    UpeFFTOcean_Destroy(&hybrid->ocean);
    memset(hybrid, 0, sizeof(*hybrid));
}

void UpeHybridWater_Step(UpeHybridWater *hybrid, float dt) {
    if (!hybrid) return;
    
    // Update FFT ocean waves
    UpeFFTOcean_UpdateGerstner(&hybrid->ocean, dt);
    
    // SPH fluid updates separately (called externally)
}

// Get blended water height at world position
float UpeHybridWater_GetHeight(UpeHybridWater *hybrid, float x, float z, float distance_to_player) {
    if (!hybrid) return 0.0f;
    
    float fft_height = UpeFFTOcean_GetHeight(&hybrid->ocean, x, z);
    
    // Blend based on distance to player/camera
    float blend = 1.0f;
    if (distance_to_player < hybrid->near_field_radius) {
        // Near field: mostly SPH
        blend = 0.1f;
    } else if (distance_to_player < hybrid->near_field_radius + hybrid->blend_distance) {
        // Transition zone: smooth blend
        float t = (distance_to_player - hybrid->near_field_radius) / hybrid->blend_distance;
        blend = 0.1f + t * 0.9f;
    }
    // Far field: mostly FFT
    
    return fft_height * blend * hybrid->far_field_amplitude;
}

