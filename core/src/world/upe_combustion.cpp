#include "upe/upe_combustion.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

void UpeCombustionGrid_Init(UpeCombustionGrid *grid, uint32_t node_count) {
    if (!grid) {
        return;
    }
    grid->count = node_count;
    grid->ignition_temp = 500.0f;
    grid->heat_release = 1000.0f;
    grid->thermal_conductivity = 0.2f;
    grid->ambient_temp = 300.0f;
    grid->cooling_rate = 0.05f;
    grid->smoke_threshold = 400.0f;
    grid->flame_threshold = 600.0f;
    grid->max_smoke_particles = 1000;
    grid->smoke_particle_count = 0;
    
    grid->nodes = (UpeCombustionNode *)malloc(node_count * sizeof(UpeCombustionNode));
    grid->temperature_buffer = (float *)malloc(node_count * sizeof(float));
    grid->smoke_particles = (UpeSmokeParticle *)malloc(grid->max_smoke_particles * sizeof(UpeSmokeParticle));
    UpeRng_Seed(&grid->rng, 0xC0FFEEu);
    
    for (uint32_t i = 0; i < node_count; ++i) {
        grid->nodes[i].fuel = 1.0f;
        grid->nodes[i].oxidizer = 1.0f;
        grid->nodes[i].temperature = 300.0f;
        grid->nodes[i].reaction_rate = 0.0f;
        grid->nodes[i].burn_damage = 0.0f;
        grid->temperature_buffer[i] = 300.0f;
    }
}

void UpeCombustionGrid_AddHeat(UpeCombustionGrid *grid, uint32_t node_idx, float heat) {
    if (!grid || node_idx >= grid->count) return;
    grid->nodes[node_idx].temperature += heat;
    if (grid->nodes[node_idx].temperature < grid->ambient_temp) {
        grid->nodes[node_idx].temperature = grid->ambient_temp;
    }
}

void UpeCombustionGrid_SetIgnition(UpeCombustionGrid *grid, uint32_t node_idx, float fuel, float oxidizer) {
    if (!grid || node_idx >= grid->count) return;
    grid->nodes[node_idx].fuel = fuel;
    grid->nodes[node_idx].oxidizer = oxidizer;
    grid->nodes[node_idx].temperature = grid->ignition_temp + 100.0f;
}

void UpeCombustionGrid_ComputeReactions(UpeCombustionGrid *grid, float dt) {
    if (!grid) return;
    
    for (uint32_t i = 0; i < grid->count; ++i) {
        UpeCombustionNode *node = &grid->nodes[i];
        
        // Reaction: fuel + oxidizer -> products + heat
        if (node->temperature >= grid->ignition_temp && 
            node->fuel > 0.01f && 
            node->oxidizer > 0.01f) {
            
            // Reaction rate proportional to temperature, fuel, and oxidizer
            float temp_factor = (node->temperature - grid->ignition_temp) / 1000.0f;
            temp_factor = temp_factor > 1.0f ? 1.0f : temp_factor;
            
            float reaction = 0.15f * temp_factor * node->fuel * node->oxidizer * dt;
            
            node->fuel -= reaction;
            node->oxidizer -= reaction;
            node->temperature += reaction * grid->heat_release;
            node->reaction_rate = reaction / (dt > 0.0001f ? dt : 0.0001f);
            
            // Burn damage accumulation
            node->burn_damage += node->reaction_rate * dt * 0.5f;
        } else {
            node->reaction_rate = 0.0f;
        }
        
        // Clamp values
        if (node->fuel < 0.0f) node->fuel = 0.0f;
        if (node->oxidizer < 0.0f) node->oxidizer = 0.0f;
    }
}

void UpeCombustionGrid_DiffuseHeat(UpeCombustionGrid *grid, float dt) {
    if (!grid) return;
    
    // Copy current temperatures for diffusion calculation
    for (uint32_t i = 0; i < grid->count; ++i) {
        grid->temperature_buffer[i] = grid->nodes[i].temperature;
    }
    
    // Heat diffusion: simplified (would need proper grid topology in practice)
    // For now, basic cooling and neighbor diffusion
    for (uint32_t i = 0; i < grid->count; ++i) {
        float current_temp = grid->temperature_buffer[i];
        float new_temp = current_temp;
        
        // Cooling to ambient air
        float cooling = grid->cooling_rate * (current_temp - grid->ambient_temp) * dt;
        new_temp -= cooling;
        
        // Temperature floor at ambient
        if (new_temp < grid->ambient_temp) {
            new_temp = grid->ambient_temp;
        }
        
        grid->nodes[i].temperature = new_temp;
    }
}

void UpeCombustionGrid_EmitSmoke(UpeCombustionGrid *grid) {
    if (!grid) return;
    
    for (uint32_t i = 0; i < grid->count; ++i) {
        UpeCombustionNode *node = &grid->nodes[i];
        
        // Emit smoke particles from hot regions
        if (node->temperature > grid->smoke_threshold && grid->smoke_particle_count < grid->max_smoke_particles) {
            UpeSmokeParticle *smoke = &grid->smoke_particles[grid->smoke_particle_count++];
            
            // Smoke properties based on temperature
            float t_ratio = (node->temperature - grid->smoke_threshold) / (grid->flame_threshold - grid->smoke_threshold);
            if (t_ratio > 1.0f) t_ratio = 1.0f;
            
            smoke->position.x = 0.0f;
            smoke->position.y = 0.0f;
            smoke->position.z = 0.0f;
            
            // Buoyancy-driven upward velocity
            float vel_scale = 0.5f + t_ratio * 1.5f;
            float jitter_x = (UpeRng_NextF32(&grid->rng) * 2.0f - 1.0f) * 0.5f;
            float jitter_z = (UpeRng_NextF32(&grid->rng) * 2.0f - 1.0f) * 0.5f;
            smoke->velocity.x = jitter_x;
            smoke->velocity.y = vel_scale;
            smoke->velocity.z = jitter_z;
            
            smoke->density = 1.0f;
            smoke->temperature = node->temperature;
            smoke->lifetime = 2.0f + t_ratio;
            smoke->is_flame = node->temperature > grid->flame_threshold;
        }
    }
}

void UpeCombustionGrid_UpdateSmoke(UpeCombustionGrid *grid, float dt) {
    if (!grid) return;
    
    uint32_t write_idx = 0;
    
    for (uint32_t i = 0; i < grid->smoke_particle_count; ++i) {
        UpeSmokeParticle *smoke = &grid->smoke_particles[i];
        
        // Update lifetime
        smoke->lifetime -= dt;
        
        if (smoke->lifetime > 0.0f) {
            // Update position
            smoke->position.x += smoke->velocity.x * dt;
            smoke->position.y += smoke->velocity.y * dt;
            smoke->position.z += smoke->velocity.z * dt;
            
            // Apply air drag and buoyancy
            smoke->velocity.x *= 0.98f;
            smoke->velocity.y += 9.81f * 0.1f * dt;  // Slight gravity effect after buoyancy
            smoke->velocity.z *= 0.98f;
            
            // Cool down
            smoke->temperature -= 50.0f * dt;
            
            // Dissipate density
            smoke->density *= (1.0f - 0.1f * dt);
            
            // Keep particle if still visible
            if (smoke->density > 0.01f && smoke->lifetime > 0.0f) {
                grid->smoke_particles[write_idx++] = *smoke;
            }
        }
    }
    
    grid->smoke_particle_count = write_idx;
}

void UpeCombustionGrid_Step(UpeCombustionGrid *grid, float dt) {
    if (!grid) {
        return;
    }
    
    // 1. Compute combustion chemistry
    UpeCombustionGrid_ComputeReactions(grid, dt);
    
    // 2. Diffuse heat through space
    UpeCombustionGrid_DiffuseHeat(grid, dt);
    
    // 3. Emit smoke/fire particles
    UpeCombustionGrid_EmitSmoke(grid);
    
    // 4. Update existing smoke/fire particles
    UpeCombustionGrid_UpdateSmoke(grid, dt);
}

void UpeCombustionGrid_Destroy(UpeCombustionGrid *grid) {
    if (!grid) {
        return;
    }
    if (grid->nodes) free(grid->nodes);
    if (grid->temperature_buffer) free(grid->temperature_buffer);
    if (grid->smoke_particles) free(grid->smoke_particles);
}
