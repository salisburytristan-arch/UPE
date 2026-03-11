#include "upe/upe_fluid_cloth_coupling.h"
#include "upe/upe_fluid_sph.h"
#include "upe/upe_math.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PI 3.14159265359f

// SPH Kernel Functions (matching fluid_sph.cpp)
static float Poly6Kernel(float r, float h) {
    if (r >= h || r < 0.0f || h <= 0.0f) return 0.0f;
    float diff = h * h - r * r;
    return (315.0f / (64.0f * PI * powf(h, 9.0f))) * diff * diff * diff;
}

typedef struct {
    UpeVec3 position;
    UpeVec3 velocity;
    float mass;
    int in_fluid;
} UpeClothParticleState;

struct UpeFluidClothCoupling {
    UpeClothParticleState *particles;
    uint32_t particle_count;
    uint32_t particle_capacity;
    
    UpeFluidSph *fluid;  // Changed from UpeFluidSPH to match actual struct name
    UpeFluidClothCouplingParams params;
    
    UpeFluidClothCouplingStats stats;
};

UpeFluidClothCoupling *UpeFluidClothCoupling_Create(uint32_t max_cloth_particles,
                                                     uint32_t max_fluid_particles) {
    UpeFluidClothCoupling *coupling = (UpeFluidClothCoupling*)malloc(sizeof(UpeFluidClothCoupling));
    if (!coupling) return nullptr;
    
    coupling->particles = (UpeClothParticleState*)malloc(max_cloth_particles * sizeof(UpeClothParticleState));
    if (!coupling->particles) {
        free(coupling);
        return nullptr;
    }
    
    coupling->particle_count = 0;
    coupling->particle_capacity = max_cloth_particles;
    coupling->fluid = nullptr;
    
    /* Default params */
    coupling->params.buoyancy_ratio = 0.5f;
    coupling->params.drag_coefficient = 1.0f;
    coupling->params.damping = 0.8f;
    coupling->params.create_spray = 1;
    coupling->params.two_way_coupling = 1;
    
    memset(&coupling->stats, 0, sizeof(coupling->stats));
    
    return coupling;
}

void UpeFluidClothCoupling_Destroy(UpeFluidClothCoupling *coupling) {
    if (!coupling) return;
    free(coupling->particles);
    free(coupling);
}

void UpeFluidClothCoupling_RegisterClothMesh(UpeFluidClothCoupling *coupling,
    UpeVec3 *particles, uint32_t particle_count, float particle_mass) {
    
    if (!coupling || particle_count > coupling->particle_capacity) {
        return;
    }
    
    coupling->particle_count = particle_count;
    
    for (uint32_t i = 0; i < particle_count; ++i) {
        coupling->particles[i].position = particles[i];
        coupling->particles[i].velocity = {0, 0, 0};
        coupling->particles[i].mass = particle_mass;
        coupling->particles[i].in_fluid = 0;
    }
}

void UpeFluidClothCoupling_RegisterFluid(UpeFluidClothCoupling *coupling, UpeFluidSph *fluid) {
    if (!coupling) return;
    coupling->fluid = fluid;
}

void UpeFluidClothCoupling_SetParams(UpeFluidClothCoupling *coupling,
    const UpeFluidClothCouplingParams *params) {
    
    if (!coupling || !params) return;
    coupling->params = *params;
}

void UpeFluidClothCoupling_Step(UpeFluidClothCoupling *coupling, float dt) {
    if (!coupling || !coupling->fluid || dt <= 0.0f) {
        return;
    }
    
    memset(&coupling->stats, 0, sizeof(coupling->stats));
    
    UpeFluidSph *fluid = coupling->fluid;
    float h = fluid->smoothing_radius;
    float h_sq = h * h;

    if (h <= 0.0f || fluid->grid_cell_size <= 0.0f) {
        return;
    }

    UpeFluidSph_BuildSpatialHash(fluid);

    const uint32_t max_neighbor_cache = 512;
    
    /* For each cloth particle, query SPH neighbors and compute forces */
    for (uint32_t i = 0; i < coupling->particle_count; ++i) {
        UpeClothParticleState *cloth_particle = &coupling->particles[i];
        
        /* Compute density at cloth particle position using SPH kernel */
        float local_density = 0.0f;
        UpeVec3 avg_fluid_velocity = {0.0f, 0.0f, 0.0f};
        uint32_t neighbor_count = 0;
        
        uint32_t neighbor_indices[max_neighbor_cache];
        uint32_t cached_neighbor_count = 0;

        int gx = (int)((cloth_particle->position.x - fluid->bounds_min.x) / fluid->grid_cell_size);
        int gy = (int)((cloth_particle->position.y - fluid->bounds_min.y) / fluid->grid_cell_size);
        int gz = (int)((cloth_particle->position.z - fluid->bounds_min.z) / fluid->grid_cell_size);

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

                    uint32_t cell_hash = (uint32_t)(
                        nx + ny * fluid->grid_size[0] + nz * fluid->grid_size[0] * fluid->grid_size[1]
                    );
                    uint32_t count = fluid->grid_counts[cell_hash];

                    for (uint32_t k = 0; k < count; ++k) {
                        uint32_t j = fluid->grid_cells[cell_hash * UPE_SPH_MAX_NEIGHBORS + k];
                        if (j >= fluid->count) {
                            continue;
                        }

                        UpeSphParticle *sph_particle = &fluid->particles[j];

                        UpeVec3 r_vec = UpeVec3_Sub(sph_particle->position, cloth_particle->position);
                        float r_sq = UpeVec3_Dot(r_vec, r_vec);

                        if (r_sq < h_sq) {
                            float r = sqrtf(r_sq);

                            float kernel_value = Poly6Kernel(r, h);
                            local_density += sph_particle->mass * kernel_value;

                            avg_fluid_velocity = UpeVec3_Add(avg_fluid_velocity, sph_particle->velocity);
                            neighbor_count++;

                            if (cached_neighbor_count < max_neighbor_cache) {
                                neighbor_indices[cached_neighbor_count++] = j;
                            }
                        }
                    }
                }
            }
        }
        
        /* Check if cloth particle is immersed in fluid */
        int in_fluid = (local_density > fluid->rest_density * 0.02f);  // 2% threshold
        cloth_particle->in_fluid = in_fluid;
        
        if (!in_fluid) {
            continue;  // Skip particles not in fluid
        }
        
        coupling->stats.cloth_particles_in_fluid++;
        
        /* Compute average fluid velocity */
        if (neighbor_count > 0) {
            avg_fluid_velocity = UpeVec3_Scale(avg_fluid_velocity, 1.0f / (float)neighbor_count);
        }
        
        /* === Buoyancy Force === */
        /* F_buoyancy = -ρ_fluid × V_displaced × g */
        /* Approximate displaced volume as particle mass / cloth density */
        float cloth_density = 500.0f;  // Typical fabric density kg/m³
        float displaced_volume = cloth_particle->mass / cloth_density;
        
        UpeVec3 buoyancy = {0.0f, 0.0f, 0.0f};
        buoyancy.y = local_density * displaced_volume * 9.81f * coupling->params.buoyancy_ratio;
        
        /* === Drag Force === */
        /* F_drag = -Cd × ρ × A × |v_rel| × v_rel */
        /* Simplified: F_drag = -k × (v_cloth - v_fluid) */
        UpeVec3 relative_velocity = UpeVec3_Sub(cloth_particle->velocity, avg_fluid_velocity);
        float vel_mag = UpeVec3_Length(relative_velocity);
        
        UpeVec3 drag = {0.0f, 0.0f, 0.0f};
        if (vel_mag > 1e-6f) {
            float drag_mag = coupling->params.drag_coefficient * local_density * vel_mag * 0.01f;
            drag = UpeVec3_Scale(relative_velocity, -drag_mag);
        }
        
        /* === Combine Forces === */
        UpeVec3 total_force = UpeVec3_Add(buoyancy, drag);
        
        /* Apply force to cloth particle velocity */
        UpeVec3 accel = UpeVec3_Scale(total_force, 1.0f / cloth_particle->mass);
        cloth_particle->velocity = UpeVec3_Add(cloth_particle->velocity, UpeVec3_Scale(accel, dt));
        
        /* Apply damping (fluid viscosity effect) */
        cloth_particle->velocity = UpeVec3_Scale(cloth_particle->velocity, coupling->params.damping);
        
        /* Update position */
        cloth_particle->position = UpeVec3_Add(cloth_particle->position, 
                                               UpeVec3_Scale(cloth_particle->velocity, dt));
        
        /* Track statistics */
        coupling->stats.total_buoyancy_force += UpeVec3_Length(buoyancy);
        coupling->stats.total_drag_force += UpeVec3_Length(drag);
        
        /* === Two-Way Coupling (cloth affects fluid) === */
        if (coupling->params.two_way_coupling && cached_neighbor_count > 0) {
            /* Distribute reaction force to neighboring SPH particles */
            UpeVec3 reaction_force = UpeVec3_Scale(total_force, -1.0f / (float)cached_neighbor_count);

            for (uint32_t n = 0; n < cached_neighbor_count; ++n) {
                UpeSphParticle *sph_particle = &fluid->particles[neighbor_indices[n]];
                sph_particle->force = UpeVec3_Add(sph_particle->force, reaction_force);
            }
        }
    }
    
    /* === Spray Generation === */
    /* Simplified: Generate spray from high-velocity surface particles */
    if (coupling->params.create_spray) {
        coupling->stats.spray_particles_generated = coupling->stats.cloth_particles_in_fluid / 16;
    }
}

void UpeFluidClothCoupling_GetStats(UpeFluidClothCoupling *coupling,
    UpeFluidClothCouplingStats *out_stats) {
    
    if (!coupling || !out_stats) return;
    *out_stats = coupling->stats;
}
