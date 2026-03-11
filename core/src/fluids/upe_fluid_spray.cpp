#include "upe/upe_fluid_spray.h"
#include "upe/upe_math.h"
#include <stdlib.h>
#include <math.h>

// Simple hash-based random
static float Hash1D_Spray(float seed) {
    float x = sinf(seed * 12.9898f) * 43758.5453f;
    return x - floorf(x);
}

void UpeFluidSpray_SetDefaultParams(UpeFluidSprayParams *params) {
    if (!params) return;
    
    params->spray_velocity_threshold = 2.0f;
    params->spray_emission_rate = 0.5f;
    params->spray_size = 0.05f;
    params->spray_lifetime = 1.0f;
    
    params->foam_curvature_threshold = 10.0f;
    params->foam_emission_rate = 0.2f;
    params->foam_size = 0.03f;
    params->foam_lifetime = 2.0f;
    
    params->bubble_pressure_threshold = -500.0f;
    params->bubble_emission_rate = 0.1f;
    params->bubble_size = 0.04f;
    params->bubble_lifetime = 3.0f;
    params->bubble_buoyancy = 0.5f;
    
    params->emission_probability = 0.3f;
    params->max_emissions_per_frame = 100;
}

void UpeFluidSpray_Init(UpeFluidSpraySystem *spray, 
                        UpeFluidSph *fluid, 
                        UpeEmitterPool *emitter_pool) {
    if (!spray || !fluid || !emitter_pool) return;
    
    spray->fluid = fluid;
    spray->emitter_pool = emitter_pool;
    spray->frame_counter = 0;
    spray->total_spray_emitted = 0;
    spray->total_foam_emitted = 0;
    spray->total_bubbles_emitted = 0;
    
    UpeFluidSpray_SetDefaultParams(&spray->params);
}

void UpeFluidSpray_Update(UpeFluidSpraySystem *spray, float dt) {
    if (!spray || !spray->fluid || !spray->emitter_pool) return;
    
    UpeFluidSph *fluid = spray->fluid;
    UpeFluidSprayParams *params = &spray->params;
    
    uint32_t emissions_this_frame = 0;
    spray->frame_counter++;
    
    // Iterate SPH particles and check emission criteria
    for (uint32_t i = 0; i < fluid->count && emissions_this_frame < params->max_emissions_per_frame; i++) {
        UpeSphParticle *p = &fluid->particles[i];
        
        // Compute velocity magnitude
        float vx = p->velocity.x;
        float vy = p->velocity.y;
        float vz = p->velocity.z;
        float speed = sqrtf(vx * vx + vy * vy + vz * vz);
        
        // Check if we should emit (probabilistic)
        float rand_seed = (float)(spray->frame_counter * 1000 + i);
        float rand_val = Hash1D_Spray(rand_seed);
        
        if (rand_val > params->emission_probability) {
            continue;
        }
        
        // Spray emission (high velocity)
        if (speed > params->spray_velocity_threshold) {
            float excess_velocity = speed - params->spray_velocity_threshold;
            float spray_chance = excess_velocity * params->spray_emission_rate * dt;
            
            if (Hash1D_Spray(rand_seed + 100.0f) < spray_chance) {
                // Create spray particle emitter request
                UpeEmitterDesc desc;
                desc.shape = UPE_EMITTER_POINT;
                desc.mode = UPE_EMITTER_BURST;
                desc.position = p->position;
                desc.rotation.x = 0.0f;
                desc.rotation.y = 0.0f;
                desc.rotation.z = 0.0f;
                desc.rotation.w = 1.0f;
                desc.particles_per_frame = 1;
                desc.emission_duration = 0.0f;
                
                // Inherit velocity from fluid particle with some spread
                desc.particle_template.velocity.x = vx * 1.2f;
                desc.particle_template.velocity.y = vy * 1.2f + 0.5f; // slight upward
                desc.particle_template.velocity.z = vz * 1.2f;
                desc.particle_template.lifetime = params->spray_lifetime;
                desc.particle_template.mass = 0.01f;
                desc.particle_template.size = params->spray_size;
                desc.particle_template.drag = 0.5f; // High drag for spray
                
                desc.shape_scale = 0.1f;
                desc.velocity_spread = 0.3f;
                desc.gravity.x = 0.0f;
                desc.gravity.y = -9.81f;
                desc.gravity.z = 0.0f;
                desc.wind_velocity.x = 0.0f;
                desc.wind_velocity.y = 0.0f;
                desc.wind_velocity.z = 0.0f;
                desc.damping = 0.0f;
                
                uint32_t emitter_id = UpeEmitterPool_AddEmitter(spray->emitter_pool, &desc);
                if (emitter_id != (uint32_t)-1) {
                    UpeEmitterPool_EmitBurst(spray->emitter_pool, emitter_id, 1);
                    spray->total_spray_emitted++;
                    emissions_this_frame++;
                }
            }
        }
        
        // Foam emission (high curvature / surface)
        // Use density as proxy for surface (low density = near surface)
        float density_ratio = p->density / fluid->rest_density;
        float is_surface = (density_ratio < 0.8f) ? 1.0f : 0.0f;
        
        if (is_surface > 0.5f && speed > 0.5f) {
            float foam_chance = params->foam_emission_rate * dt;
            
            if (Hash1D_Spray(rand_seed + 200.0f) < foam_chance) {
                UpeEmitterDesc desc;
                desc.shape = UPE_EMITTER_POINT;
                desc.mode = UPE_EMITTER_BURST;
                desc.position = p->position;
                desc.rotation.x = 0.0f;
                desc.rotation.y = 0.0f;
                desc.rotation.z = 0.0f;
                desc.rotation.w = 1.0f;
                desc.particles_per_frame = 1;
                desc.emission_duration = 0.0f;
                
                // Foam drifts slowly
                desc.particle_template.velocity.x = vx * 0.3f;
                desc.particle_template.velocity.y = 0.1f; // Float
                desc.particle_template.velocity.z = vz * 0.3f;
                desc.particle_template.lifetime = params->foam_lifetime;
                desc.particle_template.mass = 0.005f;
                desc.particle_template.size = params->foam_size;
                desc.particle_template.drag = 0.8f; // Very high drag
                
                desc.shape_scale = 0.05f;
                desc.velocity_spread = 0.2f;
                desc.gravity.x = 0.0f;
                desc.gravity.y = -1.0f; // Reduced gravity for foam
                desc.gravity.z = 0.0f;
                desc.wind_velocity.x = 0.0f;
                desc.wind_velocity.y = 0.0f;
                desc.wind_velocity.z = 0.0f;
                desc.damping = 0.0f;
                
                uint32_t emitter_id = UpeEmitterPool_AddEmitter(spray->emitter_pool, &desc);
                if (emitter_id != (uint32_t)-1) {
                    UpeEmitterPool_EmitBurst(spray->emitter_pool, emitter_id, 1);
                    spray->total_foam_emitted++;
                    emissions_this_frame++;
                }
            }
        }
        
        // Bubble emission (negative pressure / cavitation)
        if (p->pressure < params->bubble_pressure_threshold) {
            float bubble_chance = params->bubble_emission_rate * dt;
            
            if (Hash1D_Spray(rand_seed + 300.0f) < bubble_chance) {
                UpeEmitterDesc desc;
                desc.shape = UPE_EMITTER_POINT;
                desc.mode = UPE_EMITTER_BURST;
                desc.position = p->position;
                desc.rotation.x = 0.0f;
                desc.rotation.y = 0.0f;
                desc.rotation.z = 0.0f;
                desc.rotation.w = 1.0f;
                desc.particles_per_frame = 1;
                desc.emission_duration = 0.0f;
                
                // Bubbles rise
                desc.particle_template.velocity.x = 0.0f;
                desc.particle_template.velocity.y = params->bubble_buoyancy;
                desc.particle_template.velocity.z = 0.0f;
                desc.particle_template.lifetime = params->bubble_lifetime;
                desc.particle_template.mass = 0.001f;
                desc.particle_template.size = params->bubble_size;
                desc.particle_template.drag = 0.3f;
                
                desc.shape_scale = 0.02f;
                desc.velocity_spread = 0.1f;
                desc.gravity.x = 0.0f;
                desc.gravity.y = -0.1f; // Very weak gravity (buoyant)
                desc.gravity.z = 0.0f;
                desc.wind_velocity.x = 0.0f;
                desc.wind_velocity.y = 0.0f;
                desc.wind_velocity.z = 0.0f;
                desc.damping = 0.0f;
                
                uint32_t emitter_id = UpeEmitterPool_AddEmitter(spray->emitter_pool, &desc);
                if (emitter_id != (uint32_t)-1) {
                    UpeEmitterPool_EmitBurst(spray->emitter_pool, emitter_id, 1);
                    spray->total_bubbles_emitted++;
                    emissions_this_frame++;
                }
            }
        }
    }
}

uint32_t UpeFluidSpray_GetTotalEmitted(const UpeFluidSpraySystem *spray) {
    if (!spray) return 0;
    return spray->total_spray_emitted + spray->total_foam_emitted + spray->total_bubbles_emitted;
}

void UpeFluidSpray_GetEmissionStats(const UpeFluidSpraySystem *spray,
                                    uint32_t *spray_count,
                                    uint32_t *foam_count,
                                    uint32_t *bubble_count) {
    if (!spray) return;
    
    if (spray_count) *spray_count = spray->total_spray_emitted;
    if (foam_count) *foam_count = spray->total_foam_emitted;
    if (bubble_count) *bubble_count = spray->total_bubbles_emitted;
}
