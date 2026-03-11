#include "upe/upe_fluid_sph.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float ClampFloat(float value, float min_value, float max_value) {
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static int IsFiniteVec3(UpeVec3 v) {
    return isfinite(v.x) && isfinite(v.y) && isfinite(v.z);
}

// Poly6 kernel: W(r, h) = (315 / (64 * pi * h^9)) * (h^2 - r^2)^3
static float Poly6Kernel(float r_sq, float h) {
    if (r_sq >= h * h) return 0.0f;
    float h_sq = h * h;
    float diff = h_sq - r_sq;
    return (315.0f / (64.0f * (float)M_PI * powf(h, 9.0f))) * diff * diff * diff;
}

// Poly6 gradient kernel
static UpeVec3 Poly6GradientKernel(UpeVec3 r, float r_len, float h) {
    UpeVec3 result;
    result.x = 0; result.y = 0; result.z = 0;
    
    if (r_len >= h || r_len < 1e-6f) return result;
    
    float h_sq = h * h;
    float r_sq = r_len * r_len;
    float diff = h_sq - r_sq;
    float coeff = -(945.0f / (32.0f * (float)M_PI * powf(h, 9.0f))) * diff * diff;
    
    result.x = coeff * r.x;
    result.y = coeff * r.y;
    result.z = coeff * r.z;
    
    return result;
}

// Poly6 laplacian kernel (for curvature)
static float Poly6LaplacianKernel(float r_len, float h) {
    if (r_len >= h) return 0.0f;
    
    float h_sq = h * h;
    float r_sq = r_len * r_len;
    float coeff = -(945.0f / (32.0f * (float)M_PI * powf(h, 9.0f)));
    
    return coeff * (h_sq - r_sq) * (3.0f * h_sq - 7.0f * r_sq);
}

// Spiky kernel gradient: grad W(r, h) = -(45 / (pi * h^6)) * (h - r)^2 * (r / |r|)
static UpeVec3 SpikyGradient(UpeVec3 r, float r_len, float h) {
    UpeVec3 result;
    result.x = 0; result.y = 0; result.z = 0;
    
    if (r_len >= h || r_len < 1e-6f) return result;
    
    float coeff = -45.0f / ((float)M_PI * powf(h, 6.0f)) * powf(h - r_len, 2.0f) / r_len;
    result.x = coeff * r.x;
    result.y = coeff * r.y;
    result.z = coeff * r.z;
    
    return result;
}

// Viscosity laplacian: lap W(r, h) = (45 / (pi * h^6)) * (h - r)
static float ViscosityLaplacian(float r_len, float h) {
    if (r_len >= h) return 0.0f;
    return 45.0f / ((float)M_PI * powf(h, 6.0f)) * (h - r_len);
}

void UpeFluidSph_Init(UpeFluidSph *fluid, uint32_t max_particles) {
    if (!fluid) return;
    
    memset(fluid, 0, sizeof(UpeFluidSph));
    
    if (max_particles > UPE_SPH_MAX_PARTICLES) {
        max_particles = UPE_SPH_MAX_PARTICLES;
    }
    
    fluid->capacity = max_particles;
    fluid->particles = (UpeSphParticle*)malloc(max_particles * sizeof(UpeSphParticle));
    fluid->count = 0;
    
    // Default parameters (water)
    fluid->smoothing_radius = 0.1f;
    fluid->rest_density = 1000.0f;  // kg/m^3
    fluid->gas_constant = 2000.0f;
    fluid->viscosity = 0.001f;
    fluid->surface_tension = 0.0728f;  // N/m (water at 20°C)
    fluid->particle_mass = 0.02f;  // 20 grams
    
    fluid->gravity.x = 0;
    fluid->gravity.y = -9.81f;
    fluid->gravity.z = 0;
    
    // Initialize spatial hash grid
    fluid->grid_cell_size = fluid->smoothing_radius;
    fluid->grid_size[0] = 64;
    fluid->grid_size[1] = 64;
    fluid->grid_size[2] = 64;
    
    uint32_t total_cells = fluid->grid_size[0] * fluid->grid_size[1] * fluid->grid_size[2];
    fluid->grid_cells = (uint32_t*)malloc(total_cells * UPE_SPH_MAX_NEIGHBORS * sizeof(uint32_t));
    fluid->grid_counts = (uint32_t*)malloc(total_cells * sizeof(uint32_t));
    fluid->particle_cells = (uint32_t*)malloc(max_particles * sizeof(uint32_t));
    
    // Precompute kernel coefficients
    float h = fluid->smoothing_radius;
    fluid->poly6_coeff = 315.0f / (64.0f * (float)M_PI * powf(h, 9.0f));
    fluid->spiky_coeff = -45.0f / ((float)M_PI * powf(h, 6.0f));
    fluid->viscosity_coeff = 45.0f / ((float)M_PI * powf(h, 6.0f));
}

void UpeFluidSph_SetBounds(UpeFluidSph *fluid, UpeVec3 min, UpeVec3 max) {
    if (!fluid) return;
    fluid->bounds_min = min;
    fluid->bounds_max = max;
}

void UpeFluidSph_SetGravity(UpeFluidSph *fluid, UpeVec3 gravity) {
    if (!fluid) return;
    fluid->gravity = gravity;
}

uint32_t UpeFluidSph_AddParticle(UpeFluidSph *fluid, UpeVec3 position, UpeVec3 velocity) {
    if (!fluid || fluid->count >= fluid->capacity) {
        return 0xFFFFFFFF;
    }
    
    uint32_t index = fluid->count++;
    UpeSphParticle *p = &fluid->particles[index];
    
    p->position = position;
    p->velocity = velocity;
    p->force.x = 0; p->force.y = 0; p->force.z = 0;
    p->density = fluid->rest_density;
    p->pressure = 0;
    p->mass = fluid->particle_mass;
    p->grid_hash = 0;
    
    return index;
}

void UpeFluidSph_RemoveParticle(UpeFluidSph *fluid, uint32_t index) {
    if (!fluid || index >= fluid->count) return;
    
    // Swap with last particle
    if (index < fluid->count - 1) {
        fluid->particles[index] = fluid->particles[fluid->count - 1];
    }
    fluid->count--;
}

void UpeFluidSph_BuildSpatialHash(UpeFluidSph *fluid) {
    if (!fluid) return;
    
    // Clear grid
    uint32_t total_cells = fluid->grid_size[0] * fluid->grid_size[1] * fluid->grid_size[2];
    memset(fluid->grid_counts, 0, total_cells * sizeof(uint32_t));
    
    // Hash particles into grid
    for (uint32_t i = 0; i < fluid->count; ++i) {
        UpeSphParticle *p = &fluid->particles[i];
        
        // Compute grid cell
        int gx = (int)((p->position.x - fluid->bounds_min.x) / fluid->grid_cell_size);
        int gy = (int)((p->position.y - fluid->bounds_min.y) / fluid->grid_cell_size);
        int gz = (int)((p->position.z - fluid->bounds_min.z) / fluid->grid_cell_size);
        
        // Clamp to grid bounds
        if (gx < 0) gx = 0;
        if (gy < 0) gy = 0;
        if (gz < 0) gz = 0;
        if (gx >= (int)fluid->grid_size[0]) gx = fluid->grid_size[0] - 1;
        if (gy >= (int)fluid->grid_size[1]) gy = fluid->grid_size[1] - 1;
        if (gz >= (int)fluid->grid_size[2]) gz = fluid->grid_size[2] - 1;
        
        uint32_t hash = (uint32_t)(gx + gy * fluid->grid_size[0] + gz * fluid->grid_size[0] * fluid->grid_size[1]);
        p->grid_hash = hash;
        fluid->particle_cells[i] = hash;
        
        // Add to cell if there's room
        if (fluid->grid_counts[hash] < UPE_SPH_MAX_NEIGHBORS) {
            uint32_t offset = hash * UPE_SPH_MAX_NEIGHBORS + fluid->grid_counts[hash];
            fluid->grid_cells[offset] = i;
            fluid->grid_counts[hash]++;
        }
    }
}

void UpeFluidSph_ComputeDensityPressure(UpeFluidSph *fluid) {
    if (!fluid) return;

    float h = fluid->smoothing_radius;
    if (h <= 0.0f) {
        return;
    }
    if (fluid->grid_cell_size != h) {
        fluid->grid_cell_size = h;
        fluid->poly6_coeff = 315.0f / (64.0f * (float)M_PI * powf(h, 9.0f));
        fluid->spiky_coeff = -45.0f / ((float)M_PI * powf(h, 6.0f));
        fluid->viscosity_coeff = 45.0f / ((float)M_PI * powf(h, 6.0f));
    }

    UpeFluidSph_BuildSpatialHash(fluid);

    float h_sq = h * h;
    
    for (uint32_t i = 0; i < fluid->count; ++i) {
        UpeSphParticle *pi = &fluid->particles[i];
        float density = pi->mass * Poly6Kernel(0.0f, h);
        
        // Get grid cell
        int gx = (int)((pi->position.x - fluid->bounds_min.x) / fluid->grid_cell_size);
        int gy = (int)((pi->position.y - fluid->bounds_min.y) / fluid->grid_cell_size);
        int gz = (int)((pi->position.z - fluid->bounds_min.z) / fluid->grid_cell_size);
        
        // Search neighboring cells
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dz = -1; dz <= 1; ++dz) {
                    int nx = gx + dx;
                    int ny = gy + dy;
                    int nz = gz + dz;
                    
                    if (nx < 0 || ny < 0 || nz < 0 ||
                        nx >= (int)fluid->grid_size[0] ||
                        ny >= (int)fluid->grid_size[1] ||
                        nz >= (int)fluid->grid_size[2]) {
                        continue;
                    }
                    
                    uint32_t cell_hash = (uint32_t)(nx + ny * fluid->grid_size[0] + nz * fluid->grid_size[0] * fluid->grid_size[1]);
                    uint32_t count = fluid->grid_counts[cell_hash];
                    
                    for (uint32_t k = 0; k < count; ++k) {
                        uint32_t j = fluid->grid_cells[cell_hash * UPE_SPH_MAX_NEIGHBORS + k];
                        if (j == i) {
                            continue;
                        }
                        UpeSphParticle *pj = &fluid->particles[j];
                        
                        UpeVec3 diff;
                        diff.x = pi->position.x - pj->position.x;
                        diff.y = pi->position.y - pj->position.y;
                        diff.z = pi->position.z - pj->position.z;
                        
                        float r_sq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                        
                        if (r_sq < h_sq) {
                            density += pj->mass * Poly6Kernel(r_sq, h);
                        }
                    }
                }
            }
        }
        
        float min_density = fluid->rest_density * 0.01f;
        if (density < min_density) {
            density = min_density;
        }

        pi->density = density;

        float pressure = fluid->gas_constant * (density - fluid->rest_density);
        if (pressure < 0.0f) {
            pressure = 0.0f;
        }
        pi->pressure = pressure;
    }
}

void UpeFluidSph_ComputeForces(UpeFluidSph *fluid) {
    if (!fluid) return;
    
    float h = fluid->smoothing_radius;
    
    for (uint32_t i = 0; i < fluid->count; ++i) {
        UpeSphParticle *pi = &fluid->particles[i];
        
        UpeVec3 pressure_force;
        pressure_force.x = 0; pressure_force.y = 0; pressure_force.z = 0;
        
        UpeVec3 viscosity_force;
        viscosity_force.x = 0; viscosity_force.y = 0; viscosity_force.z = 0;
        
        
        // Get grid cell
        int gx = (int)((pi->position.x - fluid->bounds_min.x) / fluid->grid_cell_size);
        int gy = (int)((pi->position.y - fluid->bounds_min.y) / fluid->grid_cell_size);
        int gz = (int)((pi->position.z - fluid->bounds_min.z) / fluid->grid_cell_size);
        
        // Search neighboring cells
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dz = -1; dz <= 1; ++dz) {
                    int nx = gx + dx;
                    int ny = gy + dy;
                    int nz = gz + dz;
                    
                    if (nx < 0 || ny < 0 || nz < 0 ||
                        nx >= (int)fluid->grid_size[0] ||
                        ny >= (int)fluid->grid_size[1] ||
                        nz >= (int)fluid->grid_size[2]) {
                        continue;
                    }
                    
                    uint32_t cell_hash = (uint32_t)(nx + ny * fluid->grid_size[0] + nz * fluid->grid_size[0] * fluid->grid_size[1]);
                    uint32_t count = fluid->grid_counts[cell_hash];
                    
                    for (uint32_t k = 0; k < count; ++k) {
                        uint32_t j = fluid->grid_cells[cell_hash * UPE_SPH_MAX_NEIGHBORS + k];
                        if (i == j) continue;
                        
                        UpeSphParticle *pj = &fluid->particles[j];
                        
                        UpeVec3 diff;
                        diff.x = pi->position.x - pj->position.x;
                        diff.y = pi->position.y - pj->position.y;
                        diff.z = pi->position.z - pj->position.z;
                        
                        float r_sq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                        float r_len = sqrtf(r_sq);
                        
                        if (r_len < h && r_len > 1e-6f) {
                            // Pressure force (using Spiky kernel gradient)
                            UpeVec3 grad = SpikyGradient(diff, r_len, h);
                            float safe_density_j = pj->density;
                            if (safe_density_j < fluid->rest_density * 0.01f) {
                                safe_density_j = fluid->rest_density * 0.01f;
                            }

                            float pressure_term = (pi->pressure + pj->pressure) / (2.0f * safe_density_j);
                            
                            pressure_force.x -= pj->mass * pressure_term * grad.x;
                            pressure_force.y -= pj->mass * pressure_term * grad.y;
                            pressure_force.z -= pj->mass * pressure_term * grad.z;
                            
                            // Viscosity force
                            UpeVec3 vel_diff;
                            vel_diff.x = pj->velocity.x - pi->velocity.x;
                            vel_diff.y = pj->velocity.y - pi->velocity.y;
                            vel_diff.z = pj->velocity.z - pi->velocity.z;
                            
                            float visc_lap = ViscosityLaplacian(r_len, h);
                            float visc_term = fluid->viscosity * (pj->mass / safe_density_j) * visc_lap;
                            
                            viscosity_force.x += visc_term * vel_diff.x;
                            viscosity_force.y += visc_term * vel_diff.y;
                            viscosity_force.z += visc_term * vel_diff.z;
                            
                        }
                    }
                }
            }
        }

        // Combine pressure/viscosity forces and gravity
        pi->force.x = pressure_force.x + viscosity_force.x;
        pi->force.y = pressure_force.y + viscosity_force.y + pi->mass * fluid->gravity.y;
        pi->force.z = pressure_force.z + viscosity_force.z;

        if (!IsFiniteVec3(pi->force)) {
            pi->force.x = 0.0f;
            pi->force.y = pi->mass * fluid->gravity.y;
            pi->force.z = 0.0f;
        }
    }
}

void UpeFluidSph_ComputeSurfaceTension(UpeFluidSph *fluid) {
    if (!fluid) return;

    float h = fluid->smoothing_radius;

    for (uint32_t i = 0; i < fluid->count; ++i) {
        UpeSphParticle *pi = &fluid->particles[i];

        UpeVec3 surface_normal;
        surface_normal.x = 0; surface_normal.y = 0; surface_normal.z = 0;

        float curvature = 0.0f;

        int gx = (int)((pi->position.x - fluid->bounds_min.x) / fluid->grid_cell_size);
        int gy = (int)((pi->position.y - fluid->bounds_min.y) / fluid->grid_cell_size);
        int gz = (int)((pi->position.z - fluid->bounds_min.z) / fluid->grid_cell_size);

        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dz = -1; dz <= 1; ++dz) {
                    int nx = gx + dx;
                    int ny = gy + dy;
                    int nz = gz + dz;

                    if (nx < 0 || ny < 0 || nz < 0 ||
                        nx >= (int)fluid->grid_size[0] ||
                        ny >= (int)fluid->grid_size[1] ||
                        nz >= (int)fluid->grid_size[2]) {
                        continue;
                    }

                    uint32_t cell_hash = (uint32_t)(nx + ny * fluid->grid_size[0] +
                                                   nz * fluid->grid_size[0] * fluid->grid_size[1]);
                    uint32_t count = fluid->grid_counts[cell_hash];

                    for (uint32_t k = 0; k < count; ++k) {
                        uint32_t j = fluid->grid_cells[cell_hash * UPE_SPH_MAX_NEIGHBORS + k];
                        if (i == j) continue;

                        UpeSphParticle *pj = &fluid->particles[j];

                        UpeVec3 diff;
                        diff.x = pi->position.x - pj->position.x;
                        diff.y = pi->position.y - pj->position.y;
                        diff.z = pi->position.z - pj->position.z;

                        float r_sq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                        float r_len = sqrtf(r_sq);

                        if (r_len < h && r_len > 1e-6f) {
                            UpeVec3 color_grad = Poly6GradientKernel(diff, r_len, h);
                            surface_normal.x += (pj->mass / pj->density) * color_grad.x;
                            surface_normal.y += (pj->mass / pj->density) * color_grad.y;
                            surface_normal.z += (pj->mass / pj->density) * color_grad.z;

                            float color_lap = Poly6LaplacianKernel(r_len, h);
                            curvature += (pj->mass / pj->density) * color_lap;
                        }
                    }
                }
            }
        }

        float normal_len = sqrtf(surface_normal.x * surface_normal.x +
                                 surface_normal.y * surface_normal.y +
                                 surface_normal.z * surface_normal.z);

        if (normal_len > 0.01f) {
            float n_inv = 1.0f / normal_len;
            UpeVec3 surface_tension_force;
            surface_tension_force.x = -fluid->surface_tension * curvature * surface_normal.x * n_inv;
            surface_tension_force.y = -fluid->surface_tension * curvature * surface_normal.y * n_inv;
            surface_tension_force.z = -fluid->surface_tension * curvature * surface_normal.z * n_inv;

            pi->force.x += surface_tension_force.x;
            pi->force.y += surface_tension_force.y;
            pi->force.z += surface_tension_force.z;
        }
    }
}

void UpeFluidSph_Integrate(UpeFluidSph *fluid, float dt) {
    if (!fluid) return;
    
    for (uint32_t i = 0; i < fluid->count; ++i) {
        UpeSphParticle *p = &fluid->particles[i];

        if (p->mass <= 1e-8f) {
            continue;
        }
        
        // Velocity Verlet integration
        UpeVec3 accel;
        accel.x = p->force.x / p->mass;
        accel.y = p->force.y / p->mass;
        accel.z = p->force.z / p->mass;
        
        float max_accel = 500.0f;
        accel.x = ClampFloat(accel.x, -max_accel, max_accel);
        accel.y = ClampFloat(accel.y, -max_accel, max_accel);
        accel.z = ClampFloat(accel.z, -max_accel, max_accel);

        p->velocity.x += accel.x * dt;
        p->velocity.y += accel.y * dt;
        p->velocity.z += accel.z * dt;

        float max_velocity = 40.0f;
        p->velocity.x = ClampFloat(p->velocity.x, -max_velocity, max_velocity);
        p->velocity.y = ClampFloat(p->velocity.y, -max_velocity, max_velocity);
        p->velocity.z = ClampFloat(p->velocity.z, -max_velocity, max_velocity);

        if (!isfinite(p->velocity.x) || !isfinite(p->velocity.y) || !isfinite(p->velocity.z)) {
            p->velocity.x = 0.0f;
            p->velocity.y = 0.0f;
            p->velocity.z = 0.0f;
        }
        
        p->position.x += p->velocity.x * dt;
        p->position.y += p->velocity.y * dt;
        p->position.z += p->velocity.z * dt;

        if (!isfinite(p->position.x) || !isfinite(p->position.y) || !isfinite(p->position.z)) {
            p->position.x = 0.0f;
            p->position.y = 0.0f;
            p->position.z = 0.0f;
            p->velocity.x = 0.0f;
            p->velocity.y = 0.0f;
            p->velocity.z = 0.0f;
        }
    }
}

void UpeFluidSph_HandleBoundaries(UpeFluidSph *fluid) {
    if (!fluid) return;
    
    const float damping = 0.5f;  // Coefficient of restitution
    const float epsilon = 0.001f;
    
    for (uint32_t i = 0; i < fluid->count; ++i) {
        UpeSphParticle *p = &fluid->particles[i];
        
        // X bounds
        if (p->position.x < fluid->bounds_min.x + epsilon) {
            p->position.x = fluid->bounds_min.x + epsilon;
            p->velocity.x = -p->velocity.x * damping;
        }
        if (p->position.x > fluid->bounds_max.x - epsilon) {
            p->position.x = fluid->bounds_max.x - epsilon;
            p->velocity.x = -p->velocity.x * damping;
        }
        
        // Y bounds
        if (p->position.y < fluid->bounds_min.y + epsilon) {
            p->position.y = fluid->bounds_min.y + epsilon;
            p->velocity.y = -p->velocity.y * damping;
        }
        if (p->position.y > fluid->bounds_max.y - epsilon) {
            p->position.y = fluid->bounds_max.y - epsilon;
            p->velocity.y = -p->velocity.y * damping;
        }
        
        // Z bounds
        if (p->position.z < fluid->bounds_min.z + epsilon) {
            p->position.z = fluid->bounds_min.z + epsilon;
            p->velocity.z = -p->velocity.z * damping;
        }
        if (p->position.z > fluid->bounds_max.z - epsilon) {
            p->position.z = fluid->bounds_max.z - epsilon;
            p->velocity.z = -p->velocity.z * damping;
        }
    }
}

void UpeFluidSph_Step(UpeFluidSph *fluid, float dt) {
    if (!fluid || dt <= 0.0f) return;

    float max_sub_dt = 1.0f / 240.0f;
    uint32_t substeps = (uint32_t)ceilf(dt / max_sub_dt);
    if (substeps < 1) substeps = 1;
    if (substeps > 8) substeps = 8;
    float sub_dt = dt / (float)substeps;

    for (uint32_t step = 0; step < substeps; ++step) {
        UpeFluidSph_BuildSpatialHash(fluid);
        UpeFluidSph_ComputeDensityPressure(fluid);
        UpeFluidSph_ComputeForces(fluid);
        UpeFluidSph_ComputeSurfaceTension(fluid);
        UpeFluidSph_Integrate(fluid, sub_dt);
        UpeFluidSph_HandleBoundaries(fluid);
    }
}

uint32_t UpeFluidSph_DetectSurface(UpeFluidSph *fluid, float threshold) {
    if (!fluid) return 0;

    uint32_t surface_count = 0;
    float h = fluid->smoothing_radius;
    float h2 = h * h;

    for (uint32_t i = 0; i < fluid->count; ++i) {
        UpeSphParticle *pi = &fluid->particles[i];

        uint32_t neighbor_count = 0;
        for (uint32_t j = 0; j < fluid->count; ++j) {
            UpeSphParticle *pj = &fluid->particles[j];

            UpeVec3 r_vec;
            r_vec.x = pj->position.x - pi->position.x;
            r_vec.y = pj->position.y - pi->position.y;
            r_vec.z = pj->position.z - pi->position.z;

            float r2 = r_vec.x * r_vec.x + r_vec.y * r_vec.y + r_vec.z * r_vec.z;
            if (r2 < h2) {
                neighbor_count++;
            }
        }

        if ((float)neighbor_count < threshold * fluid->count / 1000.0f) {
            surface_count++;
        }
    }

    return surface_count;
}

uint32_t UpeFluidSph_GenerateSpray(UpeFluidSph *fluid, uint32_t count, float velocity_threshold) {
    if (!fluid) return 0;

    uint32_t spray_count = 0;
    for (uint32_t i = 0; i < fluid->count && spray_count < count; ++i) {
        UpeSphParticle *p = &fluid->particles[i];
        float v2 = p->velocity.x * p->velocity.x + p->velocity.y * p->velocity.y + p->velocity.z * p->velocity.z;

        if (sqrtf(v2) > velocity_threshold) {
            spray_count++;
        }
    }

    return spray_count;
}

void UpeFluidSph_Destroy(UpeFluidSph *fluid) {
    if (!fluid) return;
    
    if (fluid->particles) free(fluid->particles);
    if (fluid->grid_cells) free(fluid->grid_cells);
    if (fluid->grid_counts) free(fluid->grid_counts);
    if (fluid->particle_cells) free(fluid->particle_cells);
    
    memset(fluid, 0, sizeof(UpeFluidSph));
}
