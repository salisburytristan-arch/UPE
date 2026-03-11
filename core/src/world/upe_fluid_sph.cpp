#include "upe/upe_fluid_sph.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PI 3.14159265359f

// ============================================================================
// Kernel Functions and Coefficients
// ============================================================================

static float Poly6Kernel(float r, float h) {
    if (r >= h) return 0.0f;
    float r2 = r * r;
    float h2 = h * h;
    float factor = 315.0f / (64.0f * PI * h2 * h2 * h2);
    float u = (h2 - r2);
    return factor * u * u * u;
}

static float Poly6KernelGrad(float r, float h) {
    if (r >= h) return 0.0f;
    if (r == 0.0f) return 0.0f;
    float h2 = h * h;
    float factor = -945.0f / (32.0f * PI * h2 * h2 * h2);
    float u = (h2 - r * r);
    return factor * u * u / r;
}

static float SpikyKernelGrad(float r, float h) {
    if (r >= h || r == 0.0f) return 0.0f;
    float h_minus_r = h - r;
    float factor = -45.0f / (PI * h * h * h * h);
    return factor * h_minus_r * h_minus_r / r;
}

static float ViscocityKernelLaplacian(float r, float h) {
    if (r >= h) return 0.0f;
    float factor = 45.0f / (PI * h * h * h * h * h);
    return factor * (h - r);
}

// ============================================================================
// Initialization
// ============================================================================

void UpeFluidSph_Init(UpeFluidSph *fluid, uint32_t max_particles) {
    if (!fluid || max_particles == 0) return;
    
    memset(fluid, 0, sizeof(UpeFluidSph));
    
    fluid->capacity = (max_particles > UPE_SPH_MAX_PARTICLES) ? UPE_SPH_MAX_PARTICLES : max_particles;
    fluid->count = 0;
    
    // Allocate particle buffer
    fluid->particles = (UpeSphParticle*)malloc(fluid->capacity * sizeof(UpeSphParticle));
    memset(fluid->particles, 0, fluid->capacity * sizeof(UpeSphParticle));
    
    // Default parameters
    fluid->smoothing_radius = 0.1f;
    fluid->rest_density = 1000.0f;        // kg/m³ (water)
    fluid->gas_constant = 0.00667f;       // Stiffness constant
    fluid->viscosity = 0.0018f;           // Pa·s (water at 20°C)
    fluid->surface_tension = 0.073f;      // N/m (water surface tension)
    fluid->particle_mass = 0.01f;         // kg
    fluid->gravity = {0.0f, -9.81f, 0.0f};
    
    fluid->bounds_min = {-10.0f, -10.0f, -10.0f};
    fluid->bounds_max = {10.0f, 10.0f, 10.0f};
    
    // Setup spatial hash grid
    float grid_cell_size = fluid->smoothing_radius * 1.0f;  // One kernel radius per cell
    fluid->grid_cell_size = grid_cell_size;
    
    // Calculate grid dimensions
    UpeVec3 bounds_range = UpeVec3_Sub(fluid->bounds_max, fluid->bounds_min);
    fluid->grid_size[0] = (uint32_t)(bounds_range.x / grid_cell_size) + 2;
    fluid->grid_size[1] = (uint32_t)(bounds_range.y / grid_cell_size) + 2;
    fluid->grid_size[2] = (uint32_t)(bounds_range.z / grid_cell_size) + 2;
    
    uint32_t total_cells = fluid->grid_size[0] * fluid->grid_size[1] * fluid->grid_size[2];
    
    fluid->grid_cells = (uint32_t*)malloc(total_cells * sizeof(uint32_t));
    fluid->grid_counts = (uint32_t*)malloc(total_cells * sizeof(uint32_t));
    fluid->particle_cells = (uint32_t*)malloc(fluid->capacity * sizeof(uint32_t));
    
    memset(fluid->grid_cells, 0xFF, total_cells * sizeof(uint32_t));  // 0xFFFFFFFF = empty
    memset(fluid->grid_counts, 0, total_cells * sizeof(uint32_t));
    memset(fluid->particle_cells, 0xFF, fluid->capacity * sizeof(uint32_t));
    
    // Precompute kernel coefficients
    float h = fluid->smoothing_radius;
    float h2 = h * h;
    fluid->poly6_coeff = 315.0f / (64.0f * PI * h2 * h2 * h2);
    fluid->spiky_coeff = -45.0f / (PI * h2 * h2 * h2 * h);
    fluid->viscosity_coeff = 45.0f / (PI * h2 * h2 * h2 * h);
}

void UpeFluidSph_Destroy(UpeFluidSph *fluid) {
    if (!fluid) return;
    
    free(fluid->particles);
    free(fluid->grid_cells);
    free(fluid->grid_counts);
    free(fluid->particle_cells);
    
    memset(fluid, 0, sizeof(UpeFluidSph));
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

// ============================================================================
// Particle Management
// ============================================================================

uint32_t UpeFluidSph_AddParticle(UpeFluidSph *fluid, UpeVec3 position, UpeVec3 velocity) {
    if (!fluid || fluid->count >= fluid->capacity) return 0xFFFFFFFF;
    
    uint32_t idx = fluid->count++;
    UpeSphParticle *p = &fluid->particles[idx];
    
    p->position = position;
    p->velocity = velocity;
    p->force = {0.0f, 0.0f, 0.0f};
    p->mass = fluid->particle_mass;
    p->density = fluid->rest_density;
    p->pressure = 0.0f;
    
    return idx;
}

void UpeFluidSph_RemoveParticle(UpeFluidSph *fluid, uint32_t index) {
    if (!fluid || index >= fluid->count) return;
    
    // Swap with last
    if (index < fluid->count - 1) {
        fluid->particles[index] = fluid->particles[fluid->count - 1];
    }
    fluid->count--;
}

// ============================================================================
// Spatial Hashing
// ============================================================================

static uint32_t GetGridCell(UpeFluidSph *fluid, UpeVec3 pos) {
    // Convert position to grid indices
    int gx = (int)((pos.x - fluid->bounds_min.x) / fluid->grid_cell_size);
    int gy = (int)((pos.y - fluid->bounds_min.y) / fluid->grid_cell_size);
    int gz = (int)((pos.z - fluid->bounds_min.z) / fluid->grid_cell_size);
    
    // Clamp to grid bounds
    gx = (gx < 0) ? 0 : (gx >= (int)fluid->grid_size[0]) ? (int)fluid->grid_size[0] - 1 : gx;
    gy = (gy < 0) ? 0 : (gy >= (int)fluid->grid_size[1]) ? (int)fluid->grid_size[1] - 1 : gy;
    gz = (gz < 0) ? 0 : (gz >= (int)fluid->grid_size[2]) ? (int)fluid->grid_size[2] - 1 : gz;
    
    return gz * fluid->grid_size[0] * fluid->grid_size[1] + gy * fluid->grid_size[0] + gx;
}

void UpeFluidSph_BuildSpatialHash(UpeFluidSph *fluid) {
    if (!fluid) return;
    
    uint32_t total_cells = fluid->grid_size[0] * fluid->grid_size[1] * fluid->grid_size[2];
    
    // Clear grid
    memset(fluid->grid_counts, 0, total_cells * sizeof(uint32_t));
    
    // Count particles per cell
    for (uint32_t i = 0; i < fluid->count; ++i) {
        uint32_t cell = GetGridCell(fluid, fluid->particles[i].position);
        fluid->particle_cells[i] = cell;
        fluid->grid_counts[cell]++;
    }
    
    // Build cell head pointers
    uint32_t offset = 0;
    for (uint32_t i = 0; i < total_cells; ++i) {
        fluid->grid_cells[i] = offset;
        offset += fluid->grid_counts[i];
        fluid->grid_counts[i] = 0;  // Reset to use as write pointer
    }
}

// ============================================================================
// Physics: Density and Pressure
// ============================================================================

void UpeFluidSph_ComputeDensityPressure(UpeFluidSph *fluid) {
    if (!fluid) return;
    
    for (uint32_t i = 0; i < fluid->count; ++i) {
        UpeSphParticle *pi = &fluid->particles[i];
        pi->density = 0.0f;
        
        // Self-contribution (always present)
        pi->density += fluid->particle_mass * Poly6Kernel(0.0f, fluid->smoothing_radius);
        
        // Find neighbors and accumulate density
        uint32_t cell_i = GetGridCell(fluid, pi->position);
        int gx = (int)((pi->position.x - fluid->bounds_min.x) / fluid->grid_cell_size);
        int gy = (int)((pi->position.y - fluid->bounds_min.y) / fluid->grid_cell_size);
        int gz = (int)((pi->position.z - fluid->bounds_min.z) / fluid->grid_cell_size);
        
        // Check all 27 neighboring cells (3x3x3)
        for (int dz = -1; dz <= 1; ++dz) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int nx = gx + dx;
                    int ny = gy + dy;
                    int nz = gz + dz;
                    
                    // Clamp to valid grid range
                    if (nx >= 0 && nx < (int)fluid->grid_size[0] &&
                        ny >= 0 && ny < (int)fluid->grid_size[1] &&
                        nz >= 0 && nz < (int)fluid->grid_size[2]) {
                        
                        uint32_t cell = nz * fluid->grid_size[0] * fluid->grid_size[1] + 
                                       ny * fluid->grid_size[0] + nx;
                        
                        // Check all particles in this cell
                        for (uint32_t j = 0; j < fluid->count; ++j) {
                            if (fluid->particle_cells[j] == cell && i != j) {
                                UpeSphParticle *pj = &fluid->particles[j];
                                UpeVec3 r_vec = UpeVec3_Sub(pj->position, pi->position);
                                float r = UpeVec3_Length(r_vec);
                                
                                if (r < fluid->smoothing_radius) {
                                    pi->density += fluid->particle_mass * Poly6Kernel(r, fluid->smoothing_radius);
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // Ensure minimum density
        if (pi->density < fluid->rest_density * 0.01f) {
            pi->density = fluid->rest_density * 0.01f;
        }
        
        // Equation of state (ideal gas law)
        // P = k * (rho - rho_0)
        pi->pressure = fluid->gas_constant * (pi->density - fluid->rest_density);
        if (pi->pressure < 0.0f) pi->pressure = 0.0f;
    }
}

// ============================================================================
// Physics: Forces
// ============================================================================

void UpeFluidSph_ComputeForces(UpeFluidSph *fluid) {
    if (!fluid) return;
    
    // Zero forces
    for (uint32_t i = 0; i < fluid->count; ++i) {
        fluid->particles[i].force = UpeVec3_Scale({0.0f, 0.0f, 0.0f}, 0.0f);
    }
    
    // Calculate forces for each particle
    for (uint32_t i = 0; i < fluid->count; ++i) {
        UpeSphParticle *pi = &fluid->particles[i];
        UpeVec3 force = {0.0f, 0.0f, 0.0f};
        
        // Gravity
        force = UpeVec3_Add(force, UpeVec3_Scale(fluid->gravity, pi->mass));
        
        // Pressure and viscosity forces from neighbors
        int gx = (int)((pi->position.x - fluid->bounds_min.x) / fluid->grid_cell_size);
        int gy = (int)((pi->position.y - fluid->bounds_min.y) / fluid->grid_cell_size);
        int gz = (int)((pi->position.z - fluid->bounds_min.z) / fluid->grid_cell_size);
        
        for (int dz = -1; dz <= 1; ++dz) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int nx = gx + dx;
                    int ny = gy + dy;
                    int nz = gz + dz;
                    
                    if (nx >= 0 && nx < (int)fluid->grid_size[0] &&
                        ny >= 0 && ny < (int)fluid->grid_size[1] &&
                        nz >= 0 && nz < (int)fluid->grid_size[2]) {
                        
                        uint32_t cell = nz * fluid->grid_size[0] * fluid->grid_size[1] + 
                                       ny * fluid->grid_size[0] + nx;
                        
                        for (uint32_t j = 0; j < fluid->count; ++j) {
                            if (fluid->particle_cells[j] == cell && i != j) {
                                UpeSphParticle *pj = &fluid->particles[j];
                                UpeVec3 r_vec = UpeVec3_Sub(pj->position, pi->position);
                                float r = UpeVec3_Length(r_vec);
                                
                                if (r < fluid->smoothing_radius && r > 1e-6f) {
                                    UpeVec3 r_normalized = UpeVec3_Scale(r_vec, 1.0f / r);
                                    
                                    // Pressure force: F_p = -m_j * (p_i/rho_i^2 + p_j/rho_j^2) * grad W
                                    float pressure_factor = pi->pressure / (pi->density * pi->density) +
                                                           pj->pressure / (pj->density * pj->density);
                                    float grad_w = SpikyKernelGrad(r, fluid->smoothing_radius);
                                    UpeVec3 pressure_force = UpeVec3_Scale(
                                        UpeVec3_Scale(r_normalized, grad_w),
                                        -pj->mass * pressure_factor
                                    );
                                    force = UpeVec3_Add(force, pressure_force);
                                    
                                    // Viscosity force: F_v = mu * m_j * (v_j - v_i)/rho_j * laplacian W
                                    UpeVec3 v_diff = UpeVec3_Sub(pj->velocity, pi->velocity);
                                    float laplacian_w = ViscocityKernelLaplacian(r, fluid->smoothing_radius);
                                    UpeVec3 viscosity_force = UpeVec3_Scale(
                                        v_diff,
                                        fluid->viscosity * pj->mass * laplacian_w / pj->density
                                    );
                                    force = UpeVec3_Add(force, viscosity_force);
                                }
                            }
                        }
                    }
                }
            }
        }
        
        pi->force = force;
    }
}

// ============================================================================
// Physics: Surface Tension
// ============================================================================

void UpeFluidSph_ComputeSurfaceTension(UpeFluidSph *fluid) {
    if (!fluid) return;
    
    // Surface tension via color field gradient method
    // Requires computing color field and its Laplacian for each particle
    
    for (uint32_t i = 0; i < fluid->count; ++i) {
        UpeSphParticle *pi = &fluid->particles[i];
        
        // Compute color field gradient (normal to surface)
        UpeVec3 normal = {0.0f, 0.0f, 0.0f};
        float laplacian_color = 0.0f;
        
        int gx = (int)((pi->position.x - fluid->bounds_min.x) / fluid->grid_cell_size);
        int gy = (int)((pi->position.y - fluid->bounds_min.y) / fluid->grid_cell_size);
        int gz = (int)((pi->position.z - fluid->bounds_min.z) / fluid->grid_cell_size);
        
        for (int dz = -1; dz <= 1; ++dz) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int nx = gx + dx;
                    int ny = gy + dy;
                    int nz = gz + dz;
                    
                    if (nx >= 0 && nx < (int)fluid->grid_size[0] &&
                        ny >= 0 && ny < (int)fluid->grid_size[1] &&
                        nz >= 0 && nz < (int)fluid->grid_size[2]) {
                        
                        uint32_t cell = nz * fluid->grid_size[0] * fluid->grid_size[1] + 
                                       ny * fluid->grid_size[0] + nx;
                        
                        for (uint32_t j = 0; j < fluid->count; ++j) {
                            if (fluid->particle_cells[j] == cell && i != j) {
                                UpeSphParticle *pj = &fluid->particles[j];
                                UpeVec3 r_vec = UpeVec3_Sub(pj->position, pi->position);
                                float r = UpeVec3_Length(r_vec);
                                
                                if (r < fluid->smoothing_radius && r > 1e-6f) {
                                    float k = Poly6Kernel(r, fluid->smoothing_radius);
                                    UpeVec3 grad_k = UpeVec3_Scale(
                                        UpeVec3_Scale(r_vec, 1.0f / r),
                                        Poly6KernelGrad(r, fluid->smoothing_radius)
                                    );
                                    
                                    // Accumulate color field gradient
                                    normal = UpeVec3_Add(normal, UpeVec3_Scale(grad_k, pj->mass / pj->density));
                                    
                                    // Accumulate color field Laplacian
                                    laplacian_color += (pj->mass / pj->density) * k;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // Surface tension acts perpendicular to the surface
        float normal_len = UpeVec3_Length(normal);
        if (normal_len > 1e-6f) {
            UpeVec3 normal_normalized = UpeVec3_Scale(normal, 1.0f / normal_len);
            float curvature = laplacian_color / normal_len;
            
            // Surface tension force: F_st = -gamma * curvature * normal
            // (negative because it minimizes surface area)
            UpeVec3 st_force = UpeVec3_Scale(
                normal_normalized,
                -fluid->surface_tension * curvature * pi->mass
            );
            
            pi->force = UpeVec3_Add(pi->force, st_force);
        }
    }
}

// ============================================================================
// Physics: Boundary Handling (No-Slip Boundary Conditions)
// ============================================================================

void UpeFluidSph_HandleBoundaries(UpeFluidSph *fluid) {
    if (!fluid) return;
    
    float boundary_damping = 0.9f;  // Energy loss at boundaries
    float boundary_buffer = fluid->smoothing_radius * 0.5f;
    
    for (uint32_t i = 0; i < fluid->count; ++i) {
        UpeSphParticle *p = &fluid->particles[i];
        
        // X boundaries
        if (p->position.x < fluid->bounds_min.x + boundary_buffer) {
            p->position.x = fluid->bounds_min.x + boundary_buffer;
            p->velocity.x = -p->velocity.x * boundary_damping;  // No-slip reflection
        }
        if (p->position.x > fluid->bounds_max.x - boundary_buffer) {
            p->position.x = fluid->bounds_max.x - boundary_buffer;
            p->velocity.x = -p->velocity.x * boundary_damping;
        }
        
        // Y boundaries
        if (p->position.y < fluid->bounds_min.y + boundary_buffer) {
            p->position.y = fluid->bounds_min.y + boundary_buffer;
            p->velocity.y = -p->velocity.y * boundary_damping;
        }
        if (p->position.y > fluid->bounds_max.y - boundary_buffer) {
            p->position.y = fluid->bounds_max.y - boundary_buffer;
            p->velocity.y = -p->velocity.y * boundary_damping;
        }
        
        // Z boundaries
        if (p->position.z < fluid->bounds_min.z + boundary_buffer) {
            p->position.z = fluid->bounds_min.z + boundary_buffer;
            p->velocity.z = -p->velocity.z * boundary_damping;
        }
        if (p->position.z > fluid->bounds_max.z - boundary_buffer) {
            p->position.z = fluid->bounds_max.z - boundary_buffer;
            p->velocity.z = -p->velocity.z * boundary_damping;
        }
    }
}

// ============================================================================
// Physics: Integration
// ============================================================================

void UpeFluidSph_Integrate(UpeFluidSph *fluid, float dt) {
    if (!fluid || dt <= 0.0f) return;
    
    for (uint32_t i = 0; i < fluid->count; ++i) {
        UpeSphParticle *p = &fluid->particles[i];
        
        if (p->mass <= 0.0f) continue;
        
        // Velocity update: v = v + (F/m) * dt
        UpeVec3 acceleration = UpeVec3_Scale(p->force, 1.0f / p->mass);
        p->velocity = UpeVec3_Add(p->velocity, UpeVec3_Scale(acceleration, dt));
        
        // Apply velocity damping (energy dissipation)
        float damping = 0.999f;  // Very slight damping
        p->velocity = UpeVec3_Scale(p->velocity, damping);
        
        // Position update: x = x + v * dt
        p->position = UpeVec3_Add(p->position, UpeVec3_Scale(p->velocity, dt));
    }
}

// ============================================================================
// Main Simulation Step
// ============================================================================

void UpeFluidSph_Step(UpeFluidSph *fluid, float dt) {
    if (!fluid || fluid->count == 0 || dt <= 0.0f) return;
    
    // 1. Update spatial hash structure
    UpeFluidSph_BuildSpatialHash(fluid);
    
    // 2. Compute particle neighbors and update density/pressure
    UpeFluidSph_ComputeDensityPressure(fluid);
    
    // 3. Apply all forces (pressure, viscosity, gravity)
    UpeFluidSph_ComputeForces(fluid);
    
    // 4. Apply surface tension effects
    UpeFluidSph_ComputeSurfaceTension(fluid);
    
    // 5. Integrate accelerations and velocities
    UpeFluidSph_Integrate(fluid, dt);
    
    // 6. Handle boundary collisions with no-slip conditions
    UpeFluidSph_HandleBoundaries(fluid);
}

// ============================================================================
// Utilities: Surface Detection and Spray Generation
// ============================================================================

uint32_t UpeFluidSph_DetectSurface(UpeFluidSph *fluid, float threshold) {
    if (!fluid) return 0;
    
    uint32_t surface_count = 0;
    
    // Particles with low neighbor count are near surface
    for (uint32_t i = 0; i < fluid->count; ++i) {
        UpeSphParticle *pi = &fluid->particles[i];
        
        uint32_t neighbor_count = 0;
        int gx = (int)((pi->position.x - fluid->bounds_min.x) / fluid->grid_cell_size);
        int gy = (int)((pi->position.y - fluid->bounds_min.y) / fluid->grid_cell_size);
        int gz = (int)((pi->position.z - fluid->bounds_min.z) / fluid->grid_cell_size);
        
        for (int dz = -1; dz <= 1; ++dz) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int nx = gx + dx;
                    int ny = gy + dy;
                    int nz = gz + dz;
                    
                    if (nx >= 0 && nx < (int)fluid->grid_size[0] &&
                        ny >= 0 && ny < (int)fluid->grid_size[1] &&
                        nz >= 0 && nz < (int)fluid->grid_size[2]) {
                        
                        uint32_t cell = nz * fluid->grid_size[0] * fluid->grid_size[1] + 
                                       ny * fluid->grid_size[0] + nx;
                        
                        for (uint32_t j = 0; j < fluid->count; ++j) {
                            if (fluid->particle_cells[j] == cell && i != j) {
                                UpeSphParticle *pj = &fluid->particles[j];
                                float dist = UpeVec3_Length(UpeVec3_Sub(pj->position, pi->position));
                                if (dist < fluid->smoothing_radius) {
                                    neighbor_count++;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // If density is below threshold, it's a surface particle
        if (pi->density < threshold * fluid->rest_density) {
            surface_count++;
        }
    }
    
    return surface_count;
}

uint32_t UpeFluidSph_GenerateSpray(UpeFluidSph *fluid, uint32_t count, float velocity_threshold) {
    if (!fluid) return 0;
    
    uint32_t generated = 0;
    
    for (uint32_t i = 0; i < fluid->count && generated < count; ++i) {
        UpeSphParticle *p = &fluid->particles[i];
        float vel_mag = UpeVec3_Length(p->velocity);
        
        // Only generate spray from fast-moving particles
        if (vel_mag > velocity_threshold) {
            // This marks the particle for spray generation
            // (Actual spray particle creation handled by splash system)
            generated++;
        }
    }
    
    return generated;
}
