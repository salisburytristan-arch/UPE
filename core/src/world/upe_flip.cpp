#include "upe/upe_flip.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============================================================================
// FLIP SOLVER IMPLEMENTATION
// 
// Fluid-Implicit-Particle (FLIP) hybrid Eulerian-Lagrangian approach
// - Particles store mass and carry momentum
// - Grid stores velocity field for pressure solve
// - Better energy conservation than pure SPH
// ============================================================================

#define UPE_FLIP_PARTICLE_BOUNDARY_STICKY        (1u << 0)
#define UPE_FLIP_PARTICLE_BOUNDARY_KILL_OUTSIDE  (1u << 1)
#define UPE_FLIP_PARTICLE_BOUNDARY_HIGH_BOUNCE   (1u << 2)

static void UpeFlip_ApplyParticleBoundary(UpeFlipSystem *flip, UpeFlipParticle *part)
{
    if (!flip || !part) {
        return;
    }

    float h = flip->grid.cell_size;
    float domain_min_x = flip->grid.origin.x + h;
    float domain_min_y = flip->grid.origin.y + h;
    float domain_min_z = flip->grid.origin.z + h;
    float domain_max_x = flip->grid.origin.x + h * ((float)flip->grid.width - 2.0f);
    float domain_max_y = flip->grid.origin.y + h * ((float)flip->grid.height - 2.0f);
    float domain_max_z = flip->grid.origin.z + h * ((float)flip->grid.depth - 2.0f);

    int outside = (part->position.x < domain_min_x || part->position.x > domain_max_x ||
                   part->position.y < domain_min_y || part->position.y > domain_max_y ||
                   part->position.z < domain_min_z || part->position.z > domain_max_z) ? 1 : 0;

    if (outside && (part->flags & UPE_FLIP_PARTICLE_BOUNDARY_KILL_OUTSIDE)) {
        part->flags |= (1u << 31);
        part->velocity = {0.0f, 0.0f, 0.0f};
        return;
    }

    float restitution = (part->flags & UPE_FLIP_PARTICLE_BOUNDARY_HIGH_BOUNCE) ? 0.5f : 0.2f;
    float tangential_damping = (part->flags & UPE_FLIP_PARTICLE_BOUNDARY_STICKY) ? 0.2f : 0.95f;

    if (part->position.x < domain_min_x) {
        part->position.x = domain_min_x;
        if (part->velocity.x < 0.0f) part->velocity.x = -part->velocity.x * restitution;
        part->velocity.y *= tangential_damping;
        part->velocity.z *= tangential_damping;
    } else if (part->position.x > domain_max_x) {
        part->position.x = domain_max_x;
        if (part->velocity.x > 0.0f) part->velocity.x = -part->velocity.x * restitution;
        part->velocity.y *= tangential_damping;
        part->velocity.z *= tangential_damping;
    }

    if (part->position.y < domain_min_y) {
        part->position.y = domain_min_y;
        if (part->velocity.y < 0.0f) part->velocity.y = -part->velocity.y * restitution;
        part->velocity.x *= tangential_damping;
        part->velocity.z *= tangential_damping;
    } else if (part->position.y > domain_max_y) {
        part->position.y = domain_max_y;
        if (part->velocity.y > 0.0f) part->velocity.y = -part->velocity.y * restitution;
        part->velocity.x *= tangential_damping;
        part->velocity.z *= tangential_damping;
    }

    if (part->position.z < domain_min_z) {
        part->position.z = domain_min_z;
        if (part->velocity.z < 0.0f) part->velocity.z = -part->velocity.z * restitution;
        part->velocity.x *= tangential_damping;
        part->velocity.y *= tangential_damping;
    } else if (part->position.z > domain_max_z) {
        part->position.z = domain_max_z;
        if (part->velocity.z > 0.0f) part->velocity.z = -part->velocity.z * restitution;
        part->velocity.x *= tangential_damping;
        part->velocity.y *= tangential_damping;
    }
}

void UpeFlip_Init(UpeFlipSystem *flip, uint32_t grid_width, uint32_t grid_height, uint32_t grid_depth,
                  float cell_size, uint32_t max_particles) {
    if (!flip || grid_width == 0 || grid_height == 0 || grid_depth == 0) {
        return;
    }
    
    uint32_t total_cells = grid_width * grid_height * grid_depth;
    
    // Grid setup
    flip->grid.width = grid_width;
    flip->grid.height = grid_height;
    flip->grid.depth = grid_depth;
    flip->grid.cell_size = cell_size;
    flip->grid.origin = {0.0f, 0.0f, 0.0f};
    
    // Allocate MAC grid (staggered velocities)
    flip->grid.velocity_x = (float*)calloc(total_cells, sizeof(float));
    flip->grid.velocity_y = (float*)calloc(total_cells, sizeof(float));
    flip->grid.velocity_z = (float*)calloc(total_cells, sizeof(float));
    flip->grid.pressure = (float*)calloc(total_cells, sizeof(float));
    flip->grid.cell_type = (uint8_t*)calloc(total_cells, sizeof(uint8_t));
    
    // Temporary buffers for transfers
    flip->grid_velocity_old_x = (float*)calloc(total_cells, sizeof(float));
    flip->grid_velocity_old_y = (float*)calloc(total_cells, sizeof(float));
    flip->grid_velocity_old_z = (float*)calloc(total_cells, sizeof(float));
    flip->grid_weights = (float*)calloc(total_cells, sizeof(float));
    
    // Particle storage
    flip->particle_capacity = max_particles;
    flip->particle_count = 0;
    flip->particles = (UpeFlipParticle*)calloc(max_particles, sizeof(UpeFlipParticle));
    
    // Parameters
    flip->flip_ratio = 0.95f;    // 95% FLIP (energetic), 5% PIC (smooth)
    flip->gravity = -9.81f;
    flip->density = 1000.0f;     // kg/m³ (water)
}

void UpeFlip_Destroy(UpeFlipSystem *flip) {
    if (!flip) {
        return;
    }
    
    free(flip->grid.velocity_x);
    free(flip->grid.velocity_y);
    free(flip->grid.velocity_z);
    free(flip->grid.pressure);
    free(flip->grid.cell_type);
    free(flip->grid_velocity_old_x);
    free(flip->grid_velocity_old_y);
    free(flip->grid_velocity_old_z);
    free(flip->grid_weights);
    free(flip->particles);
    
    memset(flip, 0, sizeof(UpeFlipSystem));
}

// Add particle to system
uint32_t UpeFlip_AddParticle(UpeFlipSystem *flip, UpeVec3 position, UpeVec3 velocity) {
    if (!flip || flip->particle_count >= flip->particle_capacity) {
        return UINT32_MAX;
    }
    
    uint32_t idx = flip->particle_count++;
    flip->particles[idx].position = position;
    flip->particles[idx].velocity = velocity;
    flip->particles[idx].mass = 1.0f;  // Default mass
    flip->particles[idx].flags = 0;
    
    return idx;
}

void UpeFlip_RemoveParticle(UpeFlipSystem *flip, uint32_t particle_id) {
    if (!flip || particle_id >= flip->particle_count) {
        return;
    }
    
    // Swap with last particle
    flip->particles[particle_id] = flip->particles[flip->particle_count - 1];
    flip->particle_count--;
}

// Create box of fluid particles
void UpeFlip_CreateFluidBox(UpeFlipSystem *flip, UpeVec3 min_corner, UpeVec3 max_corner) {
    if (!flip) {
        return;
    }
    
    float spacing = flip->grid.cell_size * 0.25f;  // Particles at 1/4 cell spacing
    
    for (float x = min_corner.x; x < max_corner.x; x += spacing) {
        for (float y = min_corner.y; y < max_corner.y; y += spacing) {
            for (float z = min_corner.z; z < max_corner.z; z += spacing) {
                UpeVec3 pos = {x, y, z};
                UpeVec3 vel = {0.0f, 0.0f, 0.0f};
                UpeFlip_AddParticle(flip, pos, vel);
            }
        }
    }
}

// Trilinear interpolation for velocity
static UpeVec3 InterpolateVelocity(const UpeFlipSystem *flip, UpeVec3 pos) {
    float h = flip->grid.cell_size;
    uint32_t width = flip->grid.width;
    uint32_t height = flip->grid.height;
    uint32_t depth = flip->grid.depth;
    
    // Grid-space position
    float gx = (pos.x - flip->grid.origin.x) / h;
    float gy = (pos.y - flip->grid.origin.y) / h;
    float gz = (pos.z - flip->grid.origin.z) / h;
    
    int32_t i = (int32_t)gx;
    int32_t j = (int32_t)gy;
    int32_t k = (int32_t)gz;
    
    if (i < 0 || i >= (int32_t)width - 1 || j < 0 || j >= (int32_t)height - 1 || k < 0 || k >= (int32_t)depth - 1) {
        return {0.0f, 0.0f, 0.0f};
    }
    
    float fx = gx - i;
    float fy = gy - j;
    float fz = gz - k;
    
    // Trilinear interpolation for velocity
    UpeVec3 vel = {0.0f, 0.0f, 0.0f};
    
    for (int di = 0; di <= 1; ++di) {
        for (int dj = 0; dj <= 1; ++dj) {
            for (int dk = 0; dk <= 1; ++dk) {
                float w = (di ? fx : 1.0f - fx) * (dj ? fy : 1.0f - fy) * (dk ? fz : 1.0f - fz);
                uint32_t idx = ((i + di) * height + (j + dj)) * depth + (k + dk);
                if (idx < width * height * depth) {
                    vel.x += w * flip->grid.velocity_x[idx];
                    vel.y += w * flip->grid.velocity_y[idx];
                    vel.z += w * flip->grid.velocity_z[idx];
                }
            }
        }
    }
    
    return vel;
}

// Particle to Grid transfer
void UpeFlip_TransferToGrid(UpeFlipSystem *flip) {
    if (!flip) {
        return;
    }
    
    uint32_t total = flip->grid.width * flip->grid.height * flip->grid.depth;
    memset(flip->grid.velocity_x, 0, total * sizeof(float));
    memset(flip->grid.velocity_y, 0, total * sizeof(float));
    memset(flip->grid.velocity_z, 0, total * sizeof(float));
    memset(flip->grid_weights, 0, total * sizeof(float));
    
    // Save old velocities for FLIP
    memcpy(flip->grid_velocity_old_x, flip->grid.velocity_x, total * sizeof(float));
    memcpy(flip->grid_velocity_old_y, flip->grid.velocity_y, total * sizeof(float));
    memcpy(flip->grid_velocity_old_z, flip->grid.velocity_z, total * sizeof(float));
    
    float h = flip->grid.cell_size;
    uint32_t width = flip->grid.width;
    uint32_t height = flip->grid.height;
    uint32_t depth = flip->grid.depth;
    
    // Transfer particle momentum to grid using quadratic B-spline kernel
    for (uint32_t p = 0; p < flip->particle_count; ++p) {
        const UpeFlipParticle *part = &flip->particles[p];
        
        // Grid position
        float gx = (part->position.x - flip->grid.origin.x) / h;
        float gy = (part->position.y - flip->grid.origin.y) / h;
        float gz = (part->position.z - flip->grid.origin.z) / h;
        
        int32_t i0 = (int32_t)gx;
        int32_t j0 = (int32_t)gy;
        int32_t k0 = (int32_t)gz;
        
        float fx = gx - i0;
        float fy = gy - j0;
        float fz = gz - k0;
        
        // Quadratic kernel weights
        float wfx[3] = {0.5f * (1.5f - fx) * (1.5f - fx), 0.75f - (fx - 1.0f) * (fx - 1.0f), 0.5f * (fx - 0.5f) * (fx - 0.5f)};
        float wfy[3] = {0.5f * (1.5f - fy) * (1.5f - fy), 0.75f - (fy - 1.0f) * (fy - 1.0f), 0.5f * (fy - 0.5f) * (fy - 0.5f)};
        float wfz[3] = {0.5f * (1.5f - fz) * (1.5f - fz), 0.75f - (fz - 1.0f) * (fz - 1.0f), 0.5f * (fz - 0.5f) * (fz - 0.5f)};
        
        float mass = part->mass;
        
        for (int di = 0; di < 3; ++di) {
            int32_t ni = i0 - 1 + di;
            if (ni < 0 || ni >= (int32_t)width) continue;
            
            for (int dj = 0; dj < 3; ++dj) {
                int32_t nj = j0 - 1 + dj;
                if (nj < 0 || nj >= (int32_t)height) continue;
                
                for (int dk = 0; dk < 3; ++dk) {
                    int32_t nk = k0 - 1 + dk;
                    if (nk < 0 || nk >= (int32_t)depth) continue;
                    
                    uint32_t idx = (ni * height + nj) * depth + nk;
                    float w = wfx[di] * wfy[dj] * wfz[dk];
                    float weighted_mass = w * mass;
                    
                    flip->grid.velocity_x[idx] += w * part->velocity.x * mass;
                    flip->grid.velocity_y[idx] += w * part->velocity.y * mass;
                    flip->grid.velocity_z[idx] += w * part->velocity.z * mass;
                    flip->grid_weights[idx] += weighted_mass;
                }
            }
        }
    }
    
    // Normalize velocities by weights
    for (uint32_t idx = 0; idx < total; ++idx) {
        if (flip->grid_weights[idx] > 1e-6f) {
            flip->grid.velocity_x[idx] /= flip->grid_weights[idx];
            flip->grid.velocity_y[idx] /= flip->grid_weights[idx];
            flip->grid.velocity_z[idx] /= flip->grid_weights[idx];
        }
    }
}

// Classify cells (fluid, solid, air)
void UpeFlip_ClassifyCells(UpeFlipSystem *flip) {
    if (!flip) {
        return;
    }
    
    uint32_t total = flip->grid.width * flip->grid.height * flip->grid.depth;
    memset(flip->grid.cell_type, 0, total);  // All air initially
    
    // Mark cells with particles as fluid
    float h = flip->grid.cell_size;
    uint32_t width = flip->grid.width;
    uint32_t height = flip->grid.height;
    uint32_t depth = flip->grid.depth;
    
    for (uint32_t p = 0; p < flip->particle_count; ++p) {
        const UpeFlipParticle *part = &flip->particles[p];
        
        float gx = (part->position.x - flip->grid.origin.x) / h;
        float gy = (part->position.y - flip->grid.origin.y) / h;
        float gz = (part->position.z - flip->grid.origin.z) / h;
        
        int32_t i = (int32_t)(gx + 0.5f);  // Round to nearest cell
        int32_t j = (int32_t)(gy + 0.5f);
        int32_t k = (int32_t)(gz + 0.5f);
        
        if (i >= 0 && i < (int32_t)width && j >= 0 && j < (int32_t)height && k >= 0 && k < (int32_t)depth) {
            uint32_t idx = (i * height + j) * depth + k;
            flip->grid.cell_type[idx] = 1;  // FLUID
        }
    }
}

// Add gravity
void UpeFlip_AddGravity(UpeFlipSystem *flip, float dt) {
    if (!flip || dt <= 0.0f) {
        return;
    }
    
    uint32_t total = flip->grid.width * flip->grid.height * flip->grid.depth;
    
    for (uint32_t i = 0; i < total; ++i) {
        if (flip->grid.cell_type[i] == 1) {  // Fluid cell
            flip->grid.velocity_y[i] += flip->gravity * dt;
        }
    }
}

// Simple Conjugate Gradient solver for pressure (max 20 iterations)
#define CG_MAX_ITERATIONS 20
#define CG_TOLERANCE 1e-4f

void UpeFlip_SolvePressure(UpeFlipSystem *flip, float dt) {
    if (!flip || dt <= 0.0f) {
        return;
    }
    
    uint32_t width = flip->grid.width;
    uint32_t height = flip->grid.height;
    uint32_t depth = flip->grid.depth;
    uint32_t total = width * height * depth;
    
    float h = flip->grid.cell_size;
    float scale = dt / (flip->density * h * h);
    
    // Initialize pressure to zero
    memset(flip->grid.pressure, 0, total * sizeof(float));
    
    // Allocate temporary arrays for CG solver
    float *divergence = (float*)calloc(total, sizeof(float));
    float *pressure_new = (float*)calloc(total, sizeof(float));
    
    // Compute initial divergence (v divides h by cell size)
    for (uint32_t i = 0; i < width; ++i) {
        for (uint32_t j = 0; j < height; ++j) {
            for (uint32_t k = 0; k < depth; ++k) {
                if (flip->grid.cell_type[((i * height + j) * depth + k)] != 1) {
                    continue;  // Skip non-fluid cells
                }
                
                uint32_t idx = (i * height + j) * depth + k;
                
                // Divergence = -(∇·v) = -(du/dx + dv/dy + dw/dz)
                float div = 0.0f;
                
                if (i > 0) {
                    div -= (flip->grid.velocity_x[(i * height + j) * depth + k] - flip->grid.velocity_x[((i - 1) * height + j) * depth + k]) / h;
                } else {
                    div -= flip->grid.velocity_x[idx] / h;
                }
                
                if (j > 0) {
                    div -= (flip->grid.velocity_y[((i) * height + j) * depth + k] - flip->grid.velocity_y[((i) * height + (j - 1)) * depth + k]) / h;
                } else {
                    div -= flip->grid.velocity_y[idx] / h;
                }
                
                if (k > 0) {
                    div -= (flip->grid.velocity_z[((i) * height + j) * depth + k] - flip->grid.velocity_z[((i) * height + j) * depth + (k - 1)]) / h;
                } else {
                    div -= flip->grid.velocity_z[idx] / h;
                }
                
                divergence[idx] = div;
            }
        }
    }
    
    // Simple Jacobi iteration for pressure (instead of full CG for speed)
    for (int iter = 0; iter < CG_MAX_ITERATIONS; ++iter) {
        memcpy(pressure_new, flip->grid.pressure, total * sizeof(float));
        
        for (uint32_t i = 0; i < width; ++i) {
            for (uint32_t j = 0; j < height; ++j) {
                for (uint32_t k = 0; k < depth; ++k) {
                    if (flip->grid.cell_type[((i * height + j) * depth + k)] != 1) {
                        continue;
                    }
                    
                    uint32_t idx = (i * height + j) * depth + k;
                    float sum = 0.0f;
                    int count = 0;
                    
                    // Average pressure from neighbors
                    if (i > 0 && flip->grid.cell_type[(((i - 1) * height + j) * depth + k)] == 1) {
                        sum += flip->grid.pressure[(((i - 1) * height + j) * depth + k)];
                        count++;
                    }
                    if (i < width - 1 && flip->grid.cell_type[(((i + 1) * height + j) * depth + k)] == 1) {
                        sum += flip->grid.pressure[(((i + 1) * height + j) * depth + k)];
                        count++;
                    }
                    if (j > 0 && flip->grid.cell_type[((i * height + (j - 1)) * depth + k)] == 1) {
                        sum += flip->grid.pressure[((i * height + (j - 1)) * depth + k)];
                        count++;
                    }
                    if (j < height - 1 && flip->grid.cell_type[((i * height + (j + 1)) * depth + k)] == 1) {
                        sum += flip->grid.pressure[((i * height + (j + 1)) * depth + k)];
                        count++;
                    }
                    if (k > 0 && flip->grid.cell_type[((i * height + j) * depth + (k - 1))] == 1) {
                        sum += flip->grid.pressure[((i * height + j) * depth + (k - 1))];
                        count++;
                    }
                    if (k < depth - 1 && flip->grid.cell_type[((i * height + j) * depth + (k + 1))] == 1) {
                        sum += flip->grid.pressure[((i * height + j) * depth + (k + 1))];
                        count++;
                    }
                    
                    if (count > 0) {
                        pressure_new[idx] = (sum / count - divergence[idx] * scale) / (1.0f + scale * count);
                    }
                }
            }
        }
        
        memcpy(flip->grid.pressure, pressure_new, total * sizeof(float));
    }
    
    // Apply pressure gradient to velocity: v_new = v - ∇p/ρ
    for (uint32_t i = 0; i < width; ++i) {
        for (uint32_t j = 0; j < height; ++j) {
            for (uint32_t k = 0; k < depth; ++k) {
                if (flip->grid.cell_type[((i * height + j) * depth + k)] != 1) {
                    continue;
                }
                
                uint32_t idx = (i * height + j) * depth + k;
                
                // X velocity update
                if (i > 0) {
                    float p_grad = (flip->grid.pressure[idx] - flip->grid.pressure[(((i - 1) * height + j) * depth + k)]) / h;
                    flip->grid.velocity_x[idx] -= p_grad / flip->density * dt;
                }
                
                // Y velocity update
                if (j > 0) {
                    float p_grad = (flip->grid.pressure[idx] - flip->grid.pressure[((i * height + (j - 1)) * depth + k)]) / h;
                    flip->grid.velocity_y[idx] -= p_grad / flip->density * dt;
                }
                
                // Z velocity update
                if (k > 0) {
                    float p_grad = (flip->grid.pressure[idx] - flip->grid.pressure[((i * height + j) * depth + (k - 1))]) / h;
                    flip->grid.velocity_z[idx] -= p_grad / flip->density * dt;
                }
            }
        }
    }
    
    free(divergence);
    free(pressure_new);
}

// Grid to Particles transfer with FLIP/PIC blending
void UpeFlip_TransferToParticles(UpeFlipSystem *flip) {
    if (!flip) {
        return;
    }
    
    float h = flip->grid.cell_size;
    uint32_t width = flip->grid.width;
    uint32_t height = flip->grid.height;
    uint32_t depth = flip->grid.depth;
    
    for (uint32_t p = 0; p < flip->particle_count; ++p) {
        UpeFlipParticle *part = &flip->particles[p];
        
        // Get new and old velocities at particle position
        UpeVec3 v_new = InterpolateVelocity(flip, part->position);
        
        // Compute change in velocity
        float gx = (part->position.x - flip->grid.origin.x) / h;
        float gy = (part->position.y - flip->grid.origin.y) / h;
        float gz = (part->position.z - flip->grid.origin.z) / h;
        
        int32_t i = (int32_t)gx;
        int32_t j = (int32_t)gy;
        int32_t k = (int32_t)gz;
        
        if (i < 0 || i >= (int32_t)width - 1 || j < 0 || j >= (int32_t)height - 1 || k < 0 || k >= (int32_t)depth - 1) {
            part->velocity = v_new;
            continue;
        }
        
        float fx = gx - i;
        float fy = gy - j;
        float fz = gz - k;
        
        // Interpolate old velocity
        UpeVec3 v_old = {0.0f, 0.0f, 0.0f};
        for (int di = 0; di <= 1; ++di) {
            for (int dj = 0; dj <= 1; ++dj) {
                for (int dk = 0; dk <= 1; ++dk) {
                    float w = (di ? fx : 1.0f - fx) * (dj ? fy : 1.0f - fy) * (dk ? fz : 1.0f - fz);
                    uint32_t idx = ((i + di) * height + (j + dj)) * depth + (k + dk);
                    v_old.x += w * flip->grid_velocity_old_x[idx];
                    v_old.y += w * flip->grid_velocity_old_y[idx];
                    v_old.z += w * flip->grid_velocity_old_z[idx];
                }
            }
        }
        
        // FLIP: Use velocity change
        UpeVec3 v_change = {v_new.x - v_old.x, v_new.y - v_old.y, v_new.z - v_old.z};
        
        // Blend FLIP (energetic) and PIC (smooth)
        float flip_ratio = flip->flip_ratio;
        part->velocity.x = v_old.x + flip_ratio * v_change.x + (1.0f - flip_ratio) * v_new.x;
        part->velocity.y = v_old.y + flip_ratio * v_change.y + (1.0f - flip_ratio) * v_new.y;
        part->velocity.z = v_old.z + flip_ratio * v_change.z + (1.0f - flip_ratio) * v_new.z;
    }
}

// Advect particles
void UpeFlip_AdvectParticles(UpeFlipSystem *flip, float dt) {
    if (!flip || dt <= 0.0f) {
        return;
    }
    
    for (uint32_t p = 0; p < flip->particle_count; ++p) {
        UpeFlipParticle *part = &flip->particles[p];
        
        // RK2 (2nd order Runge-Kutta) integration
        UpeVec3 v1 = InterpolateVelocity(flip, part->position);
        UpeVec3 pos_mid = {part->position.x + 0.5f * dt * v1.x, part->position.y + 0.5f * dt * v1.y, part->position.z + 0.5f * dt * v1.z};
        UpeVec3 v2 = InterpolateVelocity(flip, pos_mid);
        
        part->position.x += dt * v2.x;
        part->position.y += dt * v2.y;
        part->position.z += dt * v2.z;

        UpeFlip_ApplyParticleBoundary(flip, part);
    }

    uint32_t write = 0;
    for (uint32_t read = 0; read < flip->particle_count; ++read) {
        if ((flip->particles[read].flags & (1u << 31)) != 0u) {
            continue;
        }
        if (write != read) {
            flip->particles[write] = flip->particles[read];
        }
        ++write;
    }
    flip->particle_count = write;
}

// Main simulation step
void UpeFlip_Step(UpeFlipSystem *flip, float dt) {
    if (!flip || dt <= 0.0f) {
        return;
    }
    
    // Stage 1: Particle to Grid transfer
    UpeFlip_TransferToGrid(flip);
    
    // Stage 2: Classify cells (fluid/air/solid)
    UpeFlip_ClassifyCells(flip);
    
    // Stage 3: Add forces (gravity)
    UpeFlip_AddGravity(flip, dt);
    
    // Stage 4: Pressure projection (makes flow incompressible)
    UpeFlip_SolvePressure(flip, dt);
    
    // Stage 5: Grid to Particles transfer with FLIP/PIC blending
    UpeFlip_TransferToParticles(flip);
    
    // Stage 6: Advect particles
    UpeFlip_AdvectParticles(flip, dt);
}
