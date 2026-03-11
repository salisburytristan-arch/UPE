#include "upe/upe_hair.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

void UpeHair_Init(UpeHairSystem *hair, uint32_t max_strands) {
    if (!hair) {
        return;
    }
    
    hair->strand_capacity = max_strands;
    hair->strand_count = 0;
    hair->strands = (UpeHairStrand*)malloc(max_strands * sizeof(UpeHairStrand));
    
    hair->guide_count = 0;
    hair->guide_indices = (uint32_t*)malloc(max_strands * sizeof(uint32_t));
    
    hair->gravity = 9.8f;
    hair->wind_velocity = {0.0f, 0.0f, 0.0f};
    hair->air_drag = 0.1f;
    hair->hair_friction = 0.5f;
    
    hair->capsule_count = 0;
    hair->capsule_starts = (UpeVec3*)malloc(32 * sizeof(UpeVec3));
    hair->capsule_ends = (UpeVec3*)malloc(32 * sizeof(UpeVec3));
    hair->capsule_radii = (float*)malloc(32 * sizeof(float));
    
    // Density grid for hair-hair interactions
    hair->grid_width = 16;
    hair->grid_height = 16;
    hair->grid_depth = 16;
    hair->grid_cell_size = 0.1f;
    hair->density_grid = (float*)malloc(hair->grid_width * hair->grid_height * 
                                        hair->grid_depth * sizeof(float));
    memset(hair->density_grid, 0, hair->grid_width * hair->grid_height * 
           hair->grid_depth * sizeof(float));
}

void UpeHair_Destroy(UpeHairSystem *hair) {
    if (!hair) {
        return;
    }
    
    for (uint32_t i = 0; i < hair->strand_count; ++i) {
        free(hair->strands[i].particles);
    }
    
    free(hair->strands);
    free(hair->guide_indices);
    free(hair->capsule_starts);
    free(hair->capsule_ends);
    free(hair->capsule_radii);
    free(hair->density_grid);
}

uint32_t UpeHair_AddStrand(UpeHairSystem *hair, UpeVec3 root, UpeVec3 direction, float length,
                          uint32_t particle_count, int is_guide) {
    if (!hair || particle_count > UPE_HAIR_MAX_PARTICLES_PER_STRAND || 
        hair->strand_count >= hair->strand_capacity) {
        return 0xFFFFFFFF;
    }
    
    uint32_t strand_idx = hair->strand_count++;
    UpeHairStrand *strand = &hair->strands[strand_idx];
    
    strand->particle_count = particle_count;
    strand->particles = (UpeHairParticle*)malloc(particle_count * sizeof(UpeHairParticle));
    strand->root_position = root;
    strand->root_bone_id = 0xFFFFFFFF;
    strand->stiffness = 1.0f;
    strand->damping = 0.9f;
    strand->length_compliance = 0.01f;
    strand->bending_compliance = 0.005f;
    strand->flags = is_guide ? 1 : 0;
    
    // Initialize particles along strand
    UpeVec3 dir_normalized = UpeVec3_Normalize(direction);
    float segment_length = length / (particle_count - 1);
    float particle_radius = 0.003f;
    float particle_mass = 0.001f / particle_count;
    
    for (uint32_t i = 0; i < particle_count; ++i) {
        strand->particles[i].position = UpeVec3_Add(root, 
            UpeVec3_Scale(dir_normalized, segment_length * i));
        strand->particles[i].velocity = {0.0f, 0.0f, 0.0f};
        strand->particles[i].mass = particle_mass;
        strand->particles[i].radius = particle_radius;
    }
    
    if (is_guide) {
        hair->guide_indices[hair->guide_count++] = strand_idx;
    }
    
    return strand_idx;
}

void UpeHair_AttachToBone(UpeHairSystem *hair, uint32_t strand_id, uint32_t bone_id) {
    if (!hair || strand_id >= hair->strand_count) {
        return;
    }
    hair->strands[strand_id].root_bone_id = bone_id;
}

void UpeHair_IntegrateGuideStrands(UpeHairSystem *hair, float dt) {
    if (!hair || dt <= 0.0f) {
        return;
    }
    
    for (uint32_t g = 0; g < hair->guide_count; ++g) {
        uint32_t strand_idx = hair->guide_indices[g];
        UpeHairStrand *strand = &hair->strands[strand_idx];
        
        for (uint32_t i = 0; i < strand->particle_count; ++i) {
            if (i == 0) {
                // Root particle is pinned
                continue;
            }
            
            UpeHairParticle *p = &strand->particles[i];
            
            // Apply forces
            UpeVec3 total_force = {0.0f, -hair->gravity * p->mass, 0.0f};
            
            // Wind force
            total_force = UpeVec3_Add(total_force, 
                UpeVec3_Scale(hair->wind_velocity, hair->air_drag * p->mass));
            
            // Damping
            total_force = UpeVec3_Add(total_force, 
                UpeVec3_Scale(p->velocity, -hair->air_drag * p->mass));
            
            // Acceleration
            UpeVec3 accel = UpeVec3_Scale(total_force, 1.0f / p->mass);
            
            // Velocity Verlet integration
            UpeVec3 old_pos = p->position;
            p->position = UpeVec3_Add(p->position,
                UpeVec3_Add(UpeVec3_Scale(p->velocity, dt),
                           UpeVec3_Scale(accel, 0.5f * dt * dt)));
            
            p->velocity = UpeVec3_Scale(
                UpeVec3_Sub(p->position, old_pos),
                1.0f / dt);
        }
    }
}

static void UpeHair_SolveDistanceConstraint(UpeHairParticle *p0, UpeHairParticle *p1, 
                                            float rest_distance, float compliance) {
    UpeVec3 delta = UpeVec3_Sub(p1->position, p0->position);
    float dist = UpeVec3_Length(delta);
    
    if (dist < 1.0e-6f) {
        return;
    }
    
    float constraint_error = dist - rest_distance;
    
    // XPBD impulse
    float k = (1.0f / p0->mass) + (1.0f / p1->mass);
    k += compliance;
    
    if (k < 1.0e-8f) {
        return;
    }
    
    float impulse = -constraint_error / k;
    UpeVec3 correction = UpeVec3_Scale(delta, impulse / dist);
    
    p0->position = UpeVec3_Sub(p0->position, 
        UpeVec3_Scale(correction, 1.0f / p0->mass));
    p1->position = UpeVec3_Add(p1->position, 
        UpeVec3_Scale(correction, 1.0f / p1->mass));
}

void UpeHair_SolveConstraints(UpeHairSystem *hair, uint32_t iterations) {
    if (!hair) {
        return;
    }
    
    for (uint32_t iter = 0; iter < iterations; ++iter) {
        // Distance constraints within each strand
        for (uint32_t s = 0; s < hair->strand_count; ++s) {
            UpeHairStrand *strand = &hair->strands[s];
            
            // Length constraints (distance between adjacent particles)
            for (uint32_t i = 0; i < strand->particle_count - 1; ++i) {
                float expected_length = 0.0f;
                if (i == 0) {
                    expected_length = UpeVec3_Length(
                        UpeVec3_Sub(strand->root_position, strand->particles[i].position));
                } else {
                    float orig_dist = UpeVec3_Length(
                        UpeVec3_Sub(strand->particles[i + 1].position, 
                                   strand->particles[i].position));
                    expected_length = orig_dist;
                }
                
                UpeHair_SolveDistanceConstraint(&strand->particles[i], 
                                               &strand->particles[i + 1],
                                               expected_length, 
                                               strand->length_compliance);
            }
            
            // Bending constraints (distance between skip-one particles)
            for (uint32_t i = 0; i < strand->particle_count - 2; ++i) {
                float expected_bend_dist = 2.0f * UpeVec3_Length(
                    UpeVec3_Sub(strand->particles[i + 1].position, 
                               strand->particles[i].position));
                
                UpeHair_SolveDistanceConstraint(&strand->particles[i],
                                               &strand->particles[i + 2],
                                               expected_bend_dist,
                                               strand->bending_compliance);
            }
        }
    }
}

void UpeHair_ResolveCollisions(UpeHairSystem *hair) {
    if (!hair) {
        return;
    }
    
    // Capsule collision for each guide strand particle
    for (uint32_t g = 0; g < hair->guide_count; ++g) {
        uint32_t strand_idx = hair->guide_indices[g];
        UpeHairStrand *strand = &hair->strands[strand_idx];
        
        for (uint32_t i = 0; i < strand->particle_count; ++i) {
            UpeHairParticle *p = &strand->particles[i];
            
            for (uint32_t c = 0; c < hair->capsule_count; ++c) {
                UpeVec3 start = hair->capsule_starts[c];
                UpeVec3 end = hair->capsule_ends[c];
                float radius = hair->capsule_radii[c];
                
                // Closest point on capsule to particle
                UpeVec3 capsule_dir = UpeVec3_Sub(end, start);
                float capsule_len = UpeVec3_Length(capsule_dir);
                if (capsule_len < 1.0e-6f) {
                    continue;
                }
                
                capsule_dir = UpeVec3_Scale(capsule_dir, 1.0f / capsule_len);
                
                UpeVec3 to_particle = UpeVec3_Sub(p->position, start);
                float t = UpeVec3_Dot(to_particle, capsule_dir);
                t = (t < 0.0f) ? 0.0f : (t > capsule_len) ? capsule_len : t;
                
                UpeVec3 closest = UpeVec3_Add(start, UpeVec3_Scale(capsule_dir, t));
                UpeVec3 to_closest = UpeVec3_Sub(closest, p->position);
                float dist = UpeVec3_Length(to_closest);
                
                if (dist < radius + p->radius) {
                    float overlap = radius + p->radius - dist;
                    if (dist > 1.0e-6f) {
                        UpeVec3 normal = UpeVec3_Scale(to_closest, 1.0f / dist);
                        p->position = UpeVec3_Sub(p->position, 
                            UpeVec3_Scale(normal, overlap));
                        
                        // Friction
                        UpeVec3 tangent_vel = UpeVec3_Sub(p->velocity, 
                            UpeVec3_Scale(normal, UpeVec3_Dot(p->velocity, normal)));
                        p->velocity = UpeVec3_Sub(p->velocity,
                            UpeVec3_Scale(tangent_vel, hair->hair_friction));
                    }
                }
            }
        }
    }
}

void UpeHair_InterpolateStrands(UpeHairSystem *hair) {
    if (!hair || hair->guide_count == 0) {
        return;
    }
    
    // Simple: interpolate between nearest guide strands
    // For now, just copy guide strand positions to non-guide strands
    for (uint32_t s = 0; s < hair->strand_count; ++s) {
        if (hair->strands[s].flags & 1) {
            continue; // Skip guide strands
        }
        
        // Find nearest guide strand (simplified)
        if (hair->guide_count > 0) {
            UpeHairStrand *guide = &hair->strands[hair->guide_indices[0]];
            UpeHairStrand *strand = &hair->strands[s];
            
            if (guide->particle_count == strand->particle_count) {
                for (uint32_t i = 0; i < strand->particle_count; ++i) {
                    strand->particles[i].position = guide->particles[i].position;
                    strand->particles[i].velocity = guide->particles[i].velocity;
                }
            }
        }
    }
}

void UpeHair_ApplyWind(UpeHairSystem *hair, UpeVec3 wind, float dt) {
    if (!hair || dt <= 0.0f) {
        return;
    }
    hair->wind_velocity = wind;
}

void UpeHair_ApplyDensityForces(UpeHairSystem *hair) {
    if (!hair || hair->grid_width == 0) {
        return;
    }
    
    // Clear density grid
    memset(hair->density_grid, 0, 
           hair->grid_width * hair->grid_height * hair->grid_depth * sizeof(float));
    
    // Accumulate density from particles
    for (uint32_t s = 0; s < hair->strand_count; ++s) {
        for (uint32_t i = 0; i < hair->strands[s].particle_count; ++i) {
            UpeVec3 p = hair->strands[s].particles[i].position;
            
            // Grid index
            int gx = (int)(p.x / hair->grid_cell_size);
            int gy = (int)(p.y / hair->grid_cell_size);
            int gz = (int)(p.z / hair->grid_cell_size);
            
            if (gx >= 0 && gx < (int)hair->grid_width &&
                gy >= 0 && gy < (int)hair->grid_height &&
                gz >= 0 && gz < (int)hair->grid_depth) {
                uint32_t idx = gx + gy * hair->grid_width + 
                              gz * hair->grid_width * hair->grid_height;
                hair->density_grid[idx] += 1.0f;
            }
        }
    }
}

void UpeHair_AddCollisionCapsule(UpeHairSystem *hair, UpeVec3 start, UpeVec3 end, float radius) {
    if (!hair || hair->capsule_count >= 32) {
        return;
    }
    
    hair->capsule_starts[hair->capsule_count] = start;
    hair->capsule_ends[hair->capsule_count] = end;
    hair->capsule_radii[hair->capsule_count] = radius;
    hair->capsule_count++;
}

void UpeHair_Step(UpeHairSystem *hair, float dt) {
    if (!hair || dt <= 0.0f) {
        return;
    }
    
    // Integrate guide strands
    UpeHair_IntegrateGuideStrands(hair, dt);
    
    // Solve constraints (multiple iterations)
    UpeHair_SolveConstraints(hair, 3);
    
    // Resolve collisions
    UpeHair_ResolveCollisions(hair);
    
    // Interpolate non-guide strands
    UpeHair_InterpolateStrands(hair);
    
    // Compute density for hair-hair interactions
    UpeHair_ApplyDensityForces(hair);
}
