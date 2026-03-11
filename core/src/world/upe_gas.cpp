#include "upe/upe_gas.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

void UpeGas_Init(UpeGasSystem *gas, uint32_t width, uint32_t height, uint32_t depth, float cell_size) {
    if (!gas || width == 0 || height == 0 || depth == 0) {
        return;
    }
    
    uint32_t total_cells = width * height * depth;
    
    gas->grid.width = width;
    gas->grid.height = height;
    gas->grid.depth = depth;
    gas->grid.cell_size = cell_size;
    gas->grid.origin = {0.0f, 0.0f, 0.0f};
    
    // Staggered MAC grid velocities
    gas->grid.velocity_x = (float*)malloc(total_cells * sizeof(float));
    gas->grid.velocity_y = (float*)malloc(total_cells * sizeof(float));
    gas->grid.velocity_z = (float*)malloc(total_cells * sizeof(float));
    
    // Scalar fields
    gas->grid.density = (float*)malloc(total_cells * sizeof(float));
    gas->grid.temperature = (float*)malloc(total_cells * sizeof(float));
    gas->grid.pressure = (float*)malloc(total_cells * sizeof(float));
    
    memset(gas->grid.velocity_x, 0, total_cells * sizeof(float));
    memset(gas->grid.velocity_y, 0, total_cells * sizeof(float));
    memset(gas->grid.velocity_z, 0, total_cells * sizeof(float));
    memset(gas->grid.density, 0, total_cells * sizeof(float));
    memset(gas->grid.temperature, 0, total_cells * sizeof(float));
    memset(gas->grid.pressure, 0, total_cells * sizeof(float));
    
    // Parameters
    gas->ambient_temperature = 300.0f; // Kelvin
    gas->buoyancy_strength = 0.1f;
    gas->vorticity_strength = 0.5f;
    gas->dissipation_rate = 0.99f;
    gas->viscosity = 0.0001f;
    
    gas->source_count = 0;
    gas->smoke_sources = (UpeVec3*)malloc(16 * sizeof(UpeVec3));
    gas->source_densities = (float*)malloc(16 * sizeof(float));
    gas->source_temperatures = (float*)malloc(16 * sizeof(float));
    
    gas->obstacle_mask = (uint8_t*)malloc(total_cells * sizeof(uint8_t));
    memset(gas->obstacle_mask, 0, total_cells * sizeof(uint8_t));
    
    // Temporary buffers
    gas->velocity_x_temp = (float*)malloc(total_cells * sizeof(float));
    gas->velocity_y_temp = (float*)malloc(total_cells * sizeof(float));
    gas->velocity_z_temp = (float*)malloc(total_cells * sizeof(float));
    gas->density_temp = (float*)malloc(total_cells * sizeof(float));
}

void UpeGas_Destroy(UpeGasSystem *gas) {
    if (!gas) {
        return;
    }
    
    free(gas->grid.velocity_x);
    free(gas->grid.velocity_y);
    free(gas->grid.velocity_z);
    free(gas->grid.density);
    free(gas->grid.temperature);
    free(gas->grid.pressure);
    free(gas->smoke_sources);
    free(gas->source_densities);
    free(gas->source_temperatures);
    free(gas->obstacle_mask);
    free(gas->velocity_x_temp);
    free(gas->velocity_y_temp);
    free(gas->velocity_z_temp);
    free(gas->density_temp);
    
    memset(gas, 0, sizeof(UpeGasSystem));
}

void UpeGas_AddForces(UpeGasSystem *gas, float dt) {
    if (!gas || dt <= 0.0f) {
        return;
    }
    
    uint32_t total = gas->grid.width * gas->grid.height * gas->grid.depth;
    
    // Buoyancy force: hot smoke rises
    for (uint32_t i = 0; i < total; ++i) {
        if (gas->obstacle_mask[i]) {
            continue;
        }
        
        float temp_diff = gas->grid.temperature[i] - gas->ambient_temperature;
        float buoyancy = gas->buoyancy_strength * temp_diff * dt;
        
        // Apply to Y velocity (upward)
        gas->grid.velocity_y[i] += buoyancy;
        
        // Temperature gradually equalizes
        gas->grid.temperature[i] -= (gas->grid.temperature[i] - gas->ambient_temperature) * 0.01f * dt;
    }
}

static float UpeGas_SemiLagrangianSample(const float *field, uint32_t w, uint32_t h, uint32_t d,
                                         float x, float y, float z) {
    // Clamp to grid bounds
    int ix = (int)x;
    int iy = (int)y;
    int iz = (int)z;
    
    if (ix < 0) {
        ix = 0;
    }
    if (ix >= (int)w - 1) {
        ix = (int)w - 2;
    }
    if (iy < 0) {
        iy = 0;
    }
    if (iy >= (int)h - 1) {
        iy = (int)h - 2;
    }
    if (iz < 0) {
        iz = 0;
    }
    if (iz >= (int)d - 1) {
        iz = (int)d - 2;
    }
    
    float fx = x - ix;
    float fy = y - iy;
    float fz = z - iz;
    
    // Trilinear interpolation
    uint32_t i000 = ix + iy * w + iz * w * h;
    uint32_t i100 = (ix + 1) + iy * w + iz * w * h;
    uint32_t i010 = ix + (iy + 1) * w + iz * w * h;
    uint32_t i110 = (ix + 1) + (iy + 1) * w + iz * w * h;
    uint32_t i001 = ix + iy * w + (iz + 1) * w * h;
    uint32_t i101 = (ix + 1) + iy * w + (iz + 1) * w * h;
    uint32_t i011 = ix + (iy + 1) * w + (iz + 1) * w * h;
    uint32_t i111 = (ix + 1) + (iy + 1) * w + (iz + 1) * w * h;
    
    float v000 = field[i000];
    float v100 = field[i100];
    float v010 = field[i010];
    float v110 = field[i110];
    float v001 = field[i001];
    float v101 = field[i101];
    float v011 = field[i011];
    float v111 = field[i111];
    
    float v00 = v000 * (1 - fx) + v100 * fx;
    float v10 = v010 * (1 - fx) + v110 * fx;
    float v01 = v001 * (1 - fx) + v101 * fx;
    float v11 = v011 * (1 - fx) + v111 * fx;
    
    float v0 = v00 * (1 - fy) + v10 * fy;
    float v1 = v01 * (1 - fy) + v11 * fy;
    
    return v0 * (1 - fz) + v1 * fz;
}

void UpeGas_Advect(UpeGasSystem *gas, float dt) {
    if (!gas || dt <= 0.0f) {
        return;
    }
    
    uint32_t w = gas->grid.width;
    uint32_t h = gas->grid.height;
    uint32_t d = gas->grid.depth;
    uint32_t total = w * h * d;
    float cell = gas->grid.cell_size;
    
    // Semi-Lagrangian advection for density
    memcpy(gas->density_temp, gas->grid.density, total * sizeof(float));
    
    for (uint32_t i = 0; i < total; ++i) {
        int z = i / (w * h);
        int y = (i / w) % h;
        int x = i % w;
        
        // Backtrack to find source position
        float bx = (x + 0.5f) - gas->grid.velocity_x[i] * dt / cell;
        float by = (y + 0.5f) - gas->grid.velocity_y[i] * dt / cell;
        float bz = (z + 0.5f) - gas->grid.velocity_z[i] * dt / cell;
        
        gas->grid.density[i] = UpeGas_SemiLagrangianSample(gas->density_temp, w, h, d,
                                                           bx, by, bz);
        
        // Apply dissipation
        gas->grid.density[i] *= gas->dissipation_rate;
    }
}

void UpeGas_AddSources(UpeGasSystem *gas, float dt) {
    if (!gas) {
        return;
    }
    
    for (uint32_t s = 0; s < gas->source_count; ++s) {
        UpeVec3 pos = gas->smoke_sources[s];
        
        int x = (int)(pos.x / gas->grid.cell_size);
        int y = (int)(pos.y / gas->grid.cell_size);
        int z = (int)(pos.z / gas->grid.cell_size);
        
        if (x >= 0 && x < (int)gas->grid.width &&
            y >= 0 && y < (int)gas->grid.height &&
            z >= 0 && z < (int)gas->grid.depth) {
            
            uint32_t idx = x + y * gas->grid.width + z * gas->grid.width * gas->grid.height;
            
            gas->grid.density[idx] += gas->source_densities[s] * dt;
            gas->grid.temperature[idx] += gas->source_temperatures[s] * dt;
        }
    }
}

void UpeGas_Step(UpeGasSystem *gas, float dt) {
    if (!gas || dt <= 0.0f) {
        return;
    }
    
    // Add source terms (smoke/heat sources)
    UpeGas_AddSources(gas, dt);
    
    // Add forces (buoyancy, etc.)
    UpeGas_AddForces(gas, dt);
    
    // Advect density and temperature
    UpeGas_Advect(gas, dt);
}

void UpeGas_ProjectPressure(UpeGasSystem *gas, uint32_t iterations) {
    if (!gas) {
        return;
    }
    
    uint32_t w = gas->grid.width;
    uint32_t h = gas->grid.height;
    uint32_t d = gas->grid.depth;
    
    // Simple iterative pressure projection (simplified Poisson solver)
    for (uint32_t iter = 0; iter < iterations; ++iter) {
        for (uint32_t z = 1; z < d - 1; ++z) {
            for (uint32_t y = 1; y < h - 1; ++y) {
                for (uint32_t x = 1; x < w - 1; ++x) {
                    if (gas->obstacle_mask[x + y * w + z * w * h]) {
                        continue;
                    }
                    
                    uint32_t idx = x + y * w + z * w * h;
                    uint32_t left = (x - 1) + y * w + z * w * h;
                    uint32_t right = (x + 1) + y * w + z * w * h;
                    uint32_t down = x + (y - 1) * w + z * w * h;
                    uint32_t up = x + (y + 1) * w + z * w * h;
                    uint32_t back = x + y * w + (z - 1) * w * h;
                    uint32_t front = x + y * w + (z + 1) * w * h;
                    
                    float avg = (gas->grid.pressure[left] + gas->grid.pressure[right] +
                                gas->grid.pressure[down] + gas->grid.pressure[up] +
                                gas->grid.pressure[back] + gas->grid.pressure[front]) / 6.0f;
                    
                    gas->grid.pressure[idx] = avg * 0.9f;
                }
            }
        }
    }
}

void UpeGas_SubtractPressureGradient(UpeGasSystem *gas) {
    if (!gas) {
        return;
    }
    
    uint32_t w = gas->grid.width;
    uint32_t h = gas->grid.height;
    uint32_t d = gas->grid.depth;
    
    for (uint32_t z = 1; z < d - 1; ++z) {
        for (uint32_t y = 1; y < h - 1; ++y) {
            for (uint32_t x = 1; x < w - 1; ++x) {
                if (gas->obstacle_mask[x + y * w + z * w * h]) {
                    continue;
                }
                
                uint32_t idx = x + y * w + z * w * h;
                float grad_x = (gas->grid.pressure[(x + 1) + y * w + z * w * h] -
                               gas->grid.pressure[(x - 1) + y * w + z * w * h]) * 0.5f;
                float grad_y = (gas->grid.pressure[x + (y + 1) * w + z * w * h] -
                               gas->grid.pressure[x + (y - 1) * w + z * w * h]) * 0.5f;
                float grad_z = (gas->grid.pressure[x + y * w + (z + 1) * w * h] -
                               gas->grid.pressure[x + y * w + (z - 1) * w * h]) * 0.5f;
                
                gas->grid.velocity_x[idx] -= grad_x;
                gas->grid.velocity_y[idx] -= grad_y;
                gas->grid.velocity_z[idx] -= grad_z;
            }
        }
    }
}

void UpeGas_ApplyVorticityConfinement(UpeGasSystem *gas) {
    if (!gas || gas->vorticity_strength <= 0.0f) {
        return;
    }
    
    // Simplified vorticity confinement (swirl preservation)
    // This helps smoke maintain spiral/swirling motion
    uint32_t w = gas->grid.width;
    uint32_t h = gas->grid.height;
    uint32_t d = gas->grid.depth;
    
    for (uint32_t z = 1; z < d - 1; ++z) {
        for (uint32_t y = 1; y < h - 1; ++y) {
            for (uint32_t x = 1; x < w - 1; ++x) {
                if (gas->obstacle_mask[x + y * w + z * w * h]) {
                    continue;
                }
                
                // Compute curl/vorticity
                uint32_t idx = x + y * w + z * w * h;
                uint32_t right = (x + 1) + y * w + z * w * h;
                uint32_t left = (x - 1) + y * w + z * w * h;
                uint32_t up = x + (y + 1) * w + z * w * h;
                uint32_t down = x + (y - 1) * w + z * w * h;
                
                float vort_z = (gas->grid.velocity_y[right] - gas->grid.velocity_y[left]) * 0.5f -
                              (gas->grid.velocity_x[up] - gas->grid.velocity_x[down]) * 0.5f;
                
                // Apply confinement force (enhance swirling)
                if (fabsf(vort_z) > 1.0e-6f) {
                    gas->grid.velocity_x[idx] += vort_z * gas->vorticity_strength * 0.01f;
                    gas->grid.velocity_y[idx] -= vort_z * gas->vorticity_strength * 0.01f;
                }
            }
        }
    }
}
