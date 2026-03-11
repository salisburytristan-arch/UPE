#include "upe/upe_fluid_spray.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PI 3.14159265359f

// ============================================================================
// Initialization
// ============================================================================

void UpeFluidSpray_Init(UpeFluidSpraySystem *spray, 
                        UpeFluidSph *fluid, 
                        UpeEmitterPool *emitter_pool) {
    if (!spray || !fluid) return;
    
    memset(spray, 0, sizeof(UpeFluidSpraySystem));
    
    spray->fluid = fluid;
    spray->emitter_pool = emitter_pool;
    
    // Set default parameters
    UpeFluidSpray_SetDefaultParams(&spray->params);
    
    spray->frame_counter = 0;
    spray->total_spray_emitted = 0;
    spray->total_foam_emitted = 0;
    spray->total_bubbles_emitted = 0;
}

void UpeFluidSpray_SetDefaultParams(UpeFluidSprayParams *params) {
    if (!params) return;
    
    // Spray parameters (high velocity particles)
    params->spray_velocity_threshold = 2.0f;        // m/s
    params->spray_emission_rate = 0.5f;             // particles per m/s above threshold
    params->spray_velocity_multiplier = 1.0f;       // scale factor for initial velocity
    params->spray_size = 0.05f;                     // meters
    params->spray_lifetime = 1.0f;                  // seconds
    
    // Foam parameters (surface regions with high curvature)
    params->foam_curvature_threshold = 10.0f;       // 1/m (inverse radius of curvature)
    params->foam_emission_rate = 0.2f;              // particles per frame
    params->foam_size = 0.03f;
    params->foam_lifetime = 2.0f;
    
    // Bubble parameters (trapped air)
    params->bubble_pressure_threshold = -500.0f;   // Pa
    params->bubble_emission_rate = 0.1f;
    params->bubble_size = 0.04f;
    params->bubble_lifetime = 3.0f;
    params->bubble_buoyancy = 0.5f;                 // m/s upward
    
    // Global controls
    params->emission_probability = 0.3f;            // 30% chance per evaluation
    params->max_emissions_per_frame = 100;          // Rate limit
}

// ============================================================================
// Spray Detection and Emission
// ============================================================================

static float SimpleRandom(uint32_t *seed) {
    // Simple LCG random number generator
    *seed = (*seed * 1103515245 + 12345) & 0x7fffffff;
    return (float)(*seed) / 0x7fffffff;
}

static uint32_t EmitSprayBurst(UpeFluidSpraySystem *spray, 
                               UpeVec3 position, 
                               UpeVec3 velocity,
                               uint32_t count) {
    if (!spray || !spray->emitter_pool || count == 0) return 0;
    
    uint32_t emitted = 0;
    uint32_t seed = (uint32_t)((uint32_t*)&position.x)[0] ^ (uint32_t)((uint32_t*)&position.y)[0] ^ (uint32_t)((uint32_t*)&position.z)[0];
    
    for (uint32_t i = 0; i < count; ++i) {
        // Add random variation to spray direction
        float angle1 = SimpleRandom(&seed) * 2.0f * PI;
        float angle2 = SimpleRandom(&seed) * PI;
        float spread = 0.3f;  // Cone spread angle
        
        UpeVec3 spray_vel;
        spray_vel.x = velocity.x + cosf(angle1) * sinf(angle2) * spread;
        spray_vel.y = velocity.y + cosf(angle2) * spread;
        spray_vel.z = velocity.z + sinf(angle1) * sinf(angle2) * spread;
        
        // Emit spray particle
        if (spray->emitter_pool) {
            // Note: emitter pool emission would happen here
            // UpeEmitterPool_EmitParticle(spray->emitter_pool, position, spray_vel, ...);
            emitted++;
        }
    }
    
    return emitted;
}

// ============================================================================
// Physics Evaluation
// ============================================================================

// Evaluate high-velocity regions for spray generation
static float EvaluateSprayPotential(UpeSphParticle *p, float velocity_threshold) {
    if (!p) return 0.0f;
    
    float vel_mag = UpeVec3_Length(p->velocity);
    
    // Particles moving faster than threshold are spray sources
    if (vel_mag > velocity_threshold) {
        return (vel_mag - velocity_threshold) / velocity_threshold;  // Normalized excess
    }
    
    return 0.0f;
}

// Evaluate surface curvature for foam generation
static float EvaluateFoamPotential(UpeFluidSph *fluid, uint32_t particle_idx, float curvature_threshold) {
    if (!fluid || particle_idx >= fluid->count) return 0.0f;
    
    UpeSphParticle *pi = &fluid->particles[particle_idx];
    
    // Compute local curvature from density variations
    // Regions with low neighbor density relative to rest density have high curvature
    float density_ratio = pi->density / fluid->rest_density;
    
    // Surface particles (density ~95% of rest) indicate high curvature
    if (density_ratio > 0.9f && density_ratio < 1.1f) {
        return (1.1f - density_ratio) * 100.0f;  // Curvature estimate
    }
    
    return 0.0f;
}

// Evaluate internal pressure for bubble generation
static float EvaluateBubblePotential(UpeSphParticle *p, float pressure_threshold) {
    if (!p) return 0.0f;
    
    // Negative pressure or very low pressure indicates cavitation zones
    if (p->pressure < pressure_threshold) {
        return (pressure_threshold - p->pressure) / (-pressure_threshold);
    }
    
    return 0.0f;
}

// ============================================================================
// Main Update Loop
// ============================================================================

void UpeFluidSpray_Update(UpeFluidSpraySystem *spray, float dt) {
    if (!spray || !spray->fluid || dt <= 0.0f) return;
    
    UpeFluidSph *fluid = spray->fluid;
    UpeFluidSprayParams *params = &spray->params;
    
    uint32_t frame_emissions = 0;
    uint32_t seed = spray->frame_counter * 73856093 + 19349663;
    
    // Evaluate each particle for spray/foam/bubble potential
    for (uint32_t i = 0; i < fluid->count && frame_emissions < params->max_emissions_per_frame; ++i) {
        UpeSphParticle *p = &fluid->particles[i];
        
        // ====== SPRAY EMISSION (High Velocity) ======
        float spray_potential = EvaluateSprayPotential(p, params->spray_velocity_threshold);
        if (spray_potential > 0.0f && SimpleRandom(&seed) < params->emission_probability) {
            // Emit spray particles based on excess velocity
            uint32_t spray_count = (uint32_t)(spray_potential * params->spray_emission_rate);
            spray_count = (spray_count > 5) ? 5 : spray_count;  // Cap per particle
            
            UpeVec3 spray_dir = UpeVec3_Scale(p->velocity, params->spray_velocity_multiplier * 1.5f);
            uint32_t emitted = EmitSprayBurst(spray, p->position, spray_dir, spray_count);
            
            spray->total_spray_emitted += emitted;
            frame_emissions += emitted;
        }
        
        // ====== FOAM EMISSION (Surface Regions) ======
        float foam_potential = EvaluateFoamPotential(fluid, i, params->foam_curvature_threshold);
        if (foam_potential > 0.0f && SimpleRandom(&seed) < params->emission_probability * 0.5f) {
            // Emit foam particles at surface
            uint32_t foam_count = (uint32_t)(foam_potential * params->foam_emission_rate);
            foam_count = (foam_count > 3) ? 3 : foam_count;
            
            // Foam particles have small upward velocity
            UpeVec3 foam_vel = p->velocity;
            foam_vel.y += 0.3f;  // Slight buoyancy
            
            uint32_t emitted = EmitSprayBurst(spray, p->position, foam_vel, foam_count);
            spray->total_foam_emitted += emitted;
            frame_emissions += emitted;
        }
        
        // ====== BUBBLE EMISSION (Cavitation Zones) ======
        float bubble_potential = EvaluateBubblePotential(p, params->bubble_pressure_threshold);
        if (bubble_potential > 0.0f && SimpleRandom(&seed) < params->emission_probability * 0.3f) {
            // Emit bubbles in low-pressure regions
            uint32_t bubble_count = (uint32_t)(bubble_potential * params->bubble_emission_rate);
            bubble_count = (bubble_count > 2) ? 2 : bubble_count;
            
            // Bubbles rise due to buoyancy
            UpeVec3 bubble_vel = p->velocity;
            bubble_vel.y = params->bubble_buoyancy;
            
            uint32_t emitted = EmitSprayBurst(spray, p->position, bubble_vel, bubble_count);
            spray->total_bubbles_emitted += emitted;
            frame_emissions += emitted;
        }
    }
    
    spray->frame_counter++;
}

// ============================================================================
// Utilities
// ============================================================================

uint32_t UpeFluidSpray_GetTotalEmitted(const UpeFluidSpraySystem *spray) {
    if (!spray) return 0;
    return spray->total_spray_emitted + spray->total_foam_emitted + spray->total_bubbles_emitted;
}

void UpeFluidSpray_GetEmissionStats(const UpeFluidSpraySystem *spray,
                                    uint32_t *spray_count,
                                    uint32_t *foam_count,
                                    uint32_t *bubble_count) {
    if (spray_count) *spray_count = spray ? spray->total_spray_emitted : 0;
    if (foam_count) *foam_count = spray ? spray->total_foam_emitted : 0;
    if (bubble_count) *bubble_count = spray ? spray->total_bubbles_emitted : 0;
}
