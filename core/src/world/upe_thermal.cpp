#include "upe/upe_thermal.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define ZERO_C_KELVIN 273.15f
#define WATER_LATENT_FUSION 334000.0f    // J/kg (ice to water)
#define WATER_LATENT_VAPORIZATION 2260000.0f  // J/kg (water to steam)

typedef enum UpeThermalPhase {
    THERMAL_PHASE_SOLID = 0,   // Ice
    THERMAL_PHASE_LIQUID = 1,  // Water
    THERMAL_PHASE_GAS = 2,     // Steam
} UpeThermalPhase;

typedef struct UpeThermalNodeExtended {
    float temperature;          // Kelvin
    float heat_capacity;        // J/(kg·K)
    float thermal_mass;         // kg
    float latent_heat_buffer;   // J (energy buffered during phase transition)
    uint8_t phase;              // THERMAL_PHASE_*
    uint8_t padding[3];
} UpeThermalNodeExtended;

// ============================================================================
// Thermal Grid Initialization and Management
// ============================================================================

void UpeThermalGrid_Init(UpeThermalGrid *grid, uint32_t w, uint32_t h, uint32_t d) {
    if (!grid) {
        return;
    }
    grid->width = w;
    grid->height = h;
    grid->depth = d;
    grid->conductivity = 0.5f;  // W/(m·K) default
    
    uint32_t count = w * h * d;
    grid->nodes = (UpeThermalNode *)malloc(count * sizeof(UpeThermalNode));
    if (!grid->nodes) return;
    
    for (uint32_t i = 0; i < count; ++i) {
        grid->nodes[i].temperature = 293.0f;        // Room temperature (20°C)
        grid->nodes[i].heat_capacity = 4186.0f;     // J/(kg·K) - water
        grid->nodes[i].thermal_mass = 1.0f;         // 1 kg per grid cell
    }
}

void UpeThermalGrid_Destroy(UpeThermalGrid *grid) {
    if (!grid) {
        return;
    }
    free(grid->nodes);
    memset(grid, 0, sizeof(*grid));
}

// ============================================================================
// Heat Diffusion (Fourier's Law)
// ============================================================================

void UpeThermalGrid_Step(UpeThermalGrid *grid, float dt) {
    if (!grid || dt <= 0.0f) {
        return;
    }
    
    uint32_t total = grid->width * grid->height * grid->depth;
    
    // Allocate temporary buffer for new temperatures
    float *temp_buffer = (float*)malloc(total * sizeof(float));
    if (!temp_buffer) return;
    
    memcpy(temp_buffer, &grid->nodes[0].temperature, total * sizeof(float));
    
    // 3D heat diffusion (Laplacian heat operator)
    for (uint32_t z = 1; z < grid->depth - 1; ++z) {
        for (uint32_t y = 1; y < grid->height - 1; ++y) {
            for (uint32_t x = 1; x < grid->width - 1; ++x) {
                uint32_t idx = z * (grid->width * grid->height) + y * grid->width + x;
                
                UpeThermalNode *node = &grid->nodes[idx];
                float heat_flow = 0.0f;
                
                // X direction neighbors
                uint32_t idx_x_minus = idx - 1;
                uint32_t idx_x_plus = idx + 1;
                heat_flow += (grid->nodes[idx_x_minus].temperature - node->temperature) * grid->conductivity;
                heat_flow += (grid->nodes[idx_x_plus].temperature - node->temperature) * grid->conductivity;
                
                // Y direction neighbors
                uint32_t idx_y_minus = idx - grid->width;
                uint32_t idx_y_plus = idx + grid->width;
                heat_flow += (grid->nodes[idx_y_minus].temperature - node->temperature) * grid->conductivity;
                heat_flow += (grid->nodes[idx_y_plus].temperature - node->temperature) * grid->conductivity;
                
                // Z direction neighbors
                uint32_t idx_z_minus = idx - (grid->width * grid->height);
                uint32_t idx_z_plus = idx + (grid->width * grid->height);
                heat_flow += (grid->nodes[idx_z_minus].temperature - node->temperature) * grid->conductivity;
                heat_flow += (grid->nodes[idx_z_plus].temperature - node->temperature) * grid->conductivity;
                
                // Update temperature: dT = Q / (m * c)
                float dT = heat_flow * dt / (node->thermal_mass * node->heat_capacity);
                temp_buffer[idx] = node->temperature + dT;
            }
        }
    }
    
    // Copy back with boundary conditions
    for (uint32_t i = 0; i < total; ++i) {
        grid->nodes[i].temperature = temp_buffer[i];
    }
    
    free(temp_buffer);
}

// ============================================================================
// Phase Transitions (Ice ↔ Water ↔ Steam)
// ============================================================================

void UpeThermalGrid_UpdatePhaseTransitions(UpeThermalGrid *grid, float dt) {
    if (!grid || dt <= 0.0f) return;
    
    uint32_t total = grid->width * grid->height * grid->depth;
    
    for (uint32_t i = 0; i < total; ++i) {
        UpeThermalNode *node = &grid->nodes[i];
        
        // Phase transition thresholds (for water)
        float melting_point = ZERO_C_KELVIN;              // 0°C = 273.15K
        float boiling_point = ZERO_C_KELVIN + 100.0f;    // 100°C = 373.15K
        
        // Check for phase transitions
        if (node->temperature < melting_point - 1.0f) {
            // Solid (ice)
            node->heat_capacity = 2100.0f;  // J/(kg·K) - ice specific heat
        } else if (node->temperature < melting_point + 1.0f) {
            // Transition zone: ice to water
            // Absorb/release latent heat of fusion
            float transition_progress = (node->temperature - (melting_point - 1.0f)) / 2.0f;
            transition_progress = (transition_progress < 0.0f) ? 0.0f : (transition_progress > 1.0f) ? 1.0f : transition_progress;
            
            // Heat capacity blending during transition
            node->heat_capacity = 2100.0f * (1.0f - transition_progress) + 4186.0f * transition_progress;
            
            // Apply latent heat resistance (slower temperature change during phase transition)
            node->heat_capacity += 1000.0f;  // Latent heat increases apparent heat capacity
        } else if (node->temperature < boiling_point - 1.0f) {
            // Liquid (water)
            node->heat_capacity = 4186.0f;  // J/(kg·K) - water specific heat
        } else if (node->temperature < boiling_point + 1.0f) {
            // Transition zone: water to steam
            float transition_progress = (node->temperature - (boiling_point - 1.0f)) / 2.0f;
            transition_progress = (transition_progress < 0.0f) ? 0.0f : (transition_progress > 1.0f) ? 1.0f : transition_progress;
            
            // Heat capacity blending
            node->heat_capacity = 4186.0f * (1.0f - transition_progress) + 2000.0f * transition_progress;
            
            // Latent heat resistance during vaporization
            node->heat_capacity += 2500.0f;
        } else {
            // Gas (steam)
            node->heat_capacity = 2000.0f;  // J/(kg·K) - steam specific heat
        }
    }
}

// ============================================================================
// Thermal Expansion for Rigid Bodies
// ============================================================================

typedef struct UpeThermalExpansionParams {
    float linear_expansion_coeff;  // 1/K (typical: 12-20 × 10⁻⁶ for metals)
    float reference_temperature;   // K (usually 293K = 20°C)
    float max_expansion_factor;    // Safety limit (typical: 1.05 = 5% max)
} UpeThermalExpansionParams;

// Compute scale factor from temperature
float UpeThermal_ComputeExpansionScale(float temperature, UpeThermalExpansionParams *params) {
    if (!params || params->linear_expansion_coeff == 0.0f) return 1.0f;
    
    float dT = temperature - params->reference_temperature;
    
    // Linear expansion: L = L₀ * (1 + α * ΔT)
    // Volume expansion: V = V₀ * (1 + α * ΔT)³ ≈ V₀ * (1 + 3α * ΔT)
    float scale = 1.0f + 3.0f * params->linear_expansion_coeff * dT;
    
    // Clamp to safety limits
    if (scale > params->max_expansion_factor) scale = params->max_expansion_factor;
    if (scale < (1.0f / params->max_expansion_factor)) scale = 1.0f / params->max_expansion_factor;
    
    return scale;
}

// Thermal expansion stress calculation
// Force = E * A * α * ΔT (where E = Young's modulus, A = cross-section, α = expansion coeff)
float UpeThermal_ComputeExpansionStress(float temperature, 
                                        float youngs_modulus,
                                        float linear_expansion_coeff,
                                        float reference_temperature) {
    float dT = temperature - reference_temperature;
    
    // Stress = E * α * ΔT
    float stress = youngs_modulus * linear_expansion_coeff * dT;
    
    return stress;
}

// ============================================================================
// Heat Source Management
// ============================================================================

void UpeThermalGrid_AddHeatSource(UpeThermalGrid *grid, 
                                  uint32_t x, uint32_t y, uint32_t z,
                                  float power_watts, float radius_cells) {
    if (!grid || x >= grid->width || y >= grid->height || z >= grid->depth) return;
    
    // Distributed Gaussian heat source
    for (int dz = -(int)radius_cells; dz <= (int)radius_cells; ++dz) {
        for (int dy = -(int)radius_cells; dy <= (int)radius_cells; ++dy) {
            for (int dx = -(int)radius_cells; dx <= (int)radius_cells; ++dx) {
                int nx = x + dx;
                int ny = y + dy;
                int nz = z + dz;
                
                if (nx >= 0 && nx < (int)grid->width &&
                    ny >= 0 && ny < (int)grid->height &&
                    nz >= 0 && nz < (int)grid->depth) {
                    
                    float dist_sq = dx*dx + dy*dy + dz*dz;
                    float r_sq = radius_cells * radius_cells;
                    
                    // Gaussian falloff: exp(-r²/r₀²)
                    float falloff = expf(-dist_sq / r_sq);
                    
                    uint32_t idx = nz * (grid->width * grid->height) + ny * grid->width + nx;
                    
                    // Add heat: Q = P * dt / (thermal_mass * specific_heat)
                    // Here we add temperature directly (power normalized)
                    grid->nodes[idx].temperature += (power_watts * falloff) / 100.0f;
                }
            }
        }
    }
}

void UpeThermalGrid_AddColdSource(UpeThermalGrid *grid,
                                  uint32_t x, uint32_t y, uint32_t z,
                                  float power_watts, float radius_cells) {
    if (!grid) return;
    
    // Cold source = negative heat
    UpeThermalGrid_AddHeatSource(grid, x, y, z, -power_watts, radius_cells);
}

// ============================================================================
// Utilities
// ============================================================================

float UpeThermalGrid_GetAverageTemperature(UpeThermalGrid *grid) {
    if (!grid || grid->width == 0 || grid->height == 0 || grid->depth == 0) return 0.0f;
    
    uint32_t total = grid->width * grid->height * grid->depth;
    float sum = 0.0f;
    
    for (uint32_t i = 0; i < total; ++i) {
        sum += grid->nodes[i].temperature;
    }
    
    return sum / total;
}

void UpeThermalGrid_SetUniformTemperature(UpeThermalGrid *grid, float temperature) {
    if (!grid) return;
    
    uint32_t total = grid->width * grid->height * grid->depth;
    for (uint32_t i = 0; i < total; ++i) {
        grid->nodes[i].temperature = temperature;
    }
}

float UpeThermalGrid_SampleTemperature(UpeThermalGrid *grid, float x, float y, float z) {
    if (!grid) return 0.0f;
    
    // Trilinear interpolation
    int ix = (int)x;
    int iy = (int)y;
    int iz = (int)z;
    
    if (ix < 0 || ix >= (int)grid->width - 1 ||
        iy < 0 || iy >= (int)grid->height - 1 ||
        iz < 0 || iz >= (int)grid->depth - 1) {
        return grid->nodes[0].temperature;  // Default to first cell if out of bounds
    }
    
    float fx = x - ix;
    float fy = y - iy;
    float fz = z - iz;
    
    uint32_t idx000 = iz * (grid->width * grid->height) + iy * grid->width + ix;
    uint32_t idx100 = iz * (grid->width * grid->height) + iy * grid->width + (ix + 1);
    uint32_t idx010 = iz * (grid->width * grid->height) + (iy + 1) * grid->width + ix;
    uint32_t idx110 = iz * (grid->width * grid->height) + (iy + 1) * grid->width + (ix + 1);
    uint32_t idx001 = (iz + 1) * (grid->width * grid->height) + iy * grid->width + ix;
    uint32_t idx101 = (iz + 1) * (grid->width * grid->height) + iy * grid->width + (ix + 1);
    uint32_t idx011 = (iz + 1) * (grid->width * grid->height) + (iy + 1) * grid->width + ix;
    uint32_t idx111 = (iz + 1) * (grid->width * grid->height) + (iy + 1) * grid->width + (ix + 1);
    
    float t000 = grid->nodes[idx000].temperature;
    float t100 = grid->nodes[idx100].temperature;
    float t010 = grid->nodes[idx010].temperature;
    float t110 = grid->nodes[idx110].temperature;
    float t001 = grid->nodes[idx001].temperature;
    float t101 = grid->nodes[idx101].temperature;
    float t011 = grid->nodes[idx011].temperature;
    float t111 = grid->nodes[idx111].temperature;
    
    float t00 = t000 * (1.0f - fx) + t100 * fx;
    float t10 = t010 * (1.0f - fx) + t110 * fx;
    float t01 = t001 * (1.0f - fx) + t101 * fx;
    float t11 = t011 * (1.0f - fx) + t111 * fx;
    
    float t0 = t00 * (1.0f - fy) + t10 * fy;
    float t1 = t01 * (1.0f - fy) + t11 * fy;
    
    return t0 * (1.0f - fz) + t1 * fz;
}

