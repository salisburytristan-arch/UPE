#include "upe/upe_fluid_coupling.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

void UpeFluidCoupling_Init(UpeFluidCoupling *coupling, uint32_t max_bodies)
{
    if (!coupling) {
        return;
    }

    coupling->body_capacity = max_bodies > 0 ? max_bodies : 256;
    coupling->body_wetness = (float *)malloc(coupling->body_capacity * sizeof(float));
    if (coupling->body_wetness) {
        memset(coupling->body_wetness, 0, coupling->body_capacity * sizeof(float));
    }

    // Default parameters
    coupling->params.buoyancy_strength = 1.0f;
    coupling->params.drag_coefficient = 0.5f;
    coupling->params.water_density = 1000.0f;
    coupling->params.water_level = 0.0f;
    coupling->params.wetness_decay_rate = 1.0f;  // Dries in 1 second
    coupling->params.use_two_way_coupling = 0;
}

void UpeFluidCoupling_Destroy(UpeFluidCoupling *coupling)
{
    if (!coupling) {
        return;
    }
    if (coupling->body_wetness) {
        free(coupling->body_wetness);
        coupling->body_wetness = NULL;
    }
    coupling->body_capacity = 0;
}

void UpeFluidCoupling_SetParams(UpeFluidCoupling *coupling,
                               const UpeFluidCouplingParams *params)
{
    if (!coupling || !params) {
        return;
    }
    coupling->params = *params;
}

float UpeFluidCoupling_GetBodyWetness(const UpeFluidCoupling *coupling, uint32_t body_idx)
{
    if (!coupling || body_idx >= coupling->body_capacity || !coupling->body_wetness) {
        return 0.0f;
    }
    return coupling->body_wetness[body_idx];
}

int UpeFluidCoupling_IsBodySubmerged(const UpeFluidCoupling *coupling,
                                     const UpeBody *body)
{
    if (!coupling || !body) {
        return 0;
    }
    // Simple heuristic: body is submerged if center is below water level
    return body->position.y < coupling->params.water_level ? 1 : 0;
}

UpeVec3 UpeFluidCoupling_CalculateBuoyancyForce(const UpeFluidCoupling *coupling,
                                                const UpeBody *body,
                                                float submerged_volume)
{
    UpeVec3 result;
    if (!coupling || !body || body->inv_mass == 0.0f) {
        result.x = 0.0f; result.y = 0.0f; result.z = 0.0f;
        return result;
    }

    // Buoyancy = rho * g * V_submerged (upward)
    // F = water_density * 9.8 * submerged_volume * strength
    float buoyancy_magnitude = coupling->params.water_density * 9.8f * 
                              submerged_volume * coupling->params.buoyancy_strength;
    
    result.x = 0.0f;
    result.y = buoyancy_magnitude;
    result.z = 0.0f;
    return result;
}

UpeVec3 UpeFluidCoupling_CalculateDragForce(const UpeFluidCoupling *coupling,
                                            const UpeBody *body)
{
    UpeVec3 result;
    if (!coupling || !body) {
        result.x = 0.0f; result.y = 0.0f; result.z = 0.0f;
        return result;
    }

    // Drag force: F_d = 0.5 * rho * v^2 * C_d * A (opposite to velocity)
    // Simplified: F_d = drag_coeff * v (linear drag)
    
    float speed = sqrtf(body->velocity.x * body->velocity.x +
                       body->velocity.y * body->velocity.y +
                       body->velocity.z * body->velocity.z);
    
    if (speed < 0.001f) {
        result.x = 0.0f; result.y = 0.0f; result.z = 0.0f;
        return result;
    }

    float drag_magnitude = coupling->params.drag_coefficient * speed;
    
    result.x = -body->velocity.x / speed * drag_magnitude;
    result.y = -body->velocity.y / speed * drag_magnitude;
    result.z = -body->velocity.z / speed * drag_magnitude;
    return result;
}

void UpeFluidCoupling_ApplyForcesToBody(UpeFluidCoupling *coupling,
                                       UpeFluidSph *fluid,
                                       UpeBody *body,
                                       uint32_t body_idx,
                                       float dt)
{
    if (!coupling || !fluid || !body || body_idx >= coupling->body_capacity) {
        return;
    }

    if (body->inv_mass == 0.0f) {
        return;  // Static body
    }

    // Check if body is near water
    float distance_to_water = body->position.y - coupling->params.water_level;
    
    // If body is significantly above water, skip
    if (distance_to_water > 2.0f) {
        // Decay wetness
        if (coupling->body_wetness) {
            coupling->body_wetness[body_idx] *= (1.0f - coupling->params.wetness_decay_rate * dt);
            if (coupling->body_wetness[body_idx] < 0.01f) {
                coupling->body_wetness[body_idx] = 0.0f;
            }
        }
        return;
    }

    // Body is partially or fully submerged
    float submerged_fraction = 1.0f - (distance_to_water / 2.0f);
    if (submerged_fraction < 0.0f) submerged_fraction = 0.0f;
    if (submerged_fraction > 1.0f) submerged_fraction = 1.0f;

    // Simplified body volume (assume unit sphere of radius 0.5)
    float body_volume = (4.0f / 3.0f) * 3.14159f * 0.5f * 0.5f * 0.5f;  // 0.524
    float submerged_volume = body_volume * submerged_fraction;

    // Update wetness
    if (coupling->body_wetness) {
        coupling->body_wetness[body_idx] = fmaxf(coupling->body_wetness[body_idx] - coupling->params.wetness_decay_rate * dt, submerged_fraction * 0.5f);
    }

    // Apply buoyancy (only if any part is submerged)
    if (submerged_fraction > 0.001f) {
        UpeVec3 buoyancy = UpeFluidCoupling_CalculateBuoyancyForce(coupling, body, submerged_volume);
        
        // Apply force as velocity change (simplified)
        body->velocity.y += buoyancy.y * body->inv_mass * dt;
    }

    // Apply drag (proportional to submersion)
    if (submerged_fraction > 0.001f) {
        UpeVec3 drag = UpeFluidCoupling_CalculateDragForce(coupling, body);
        
        // Scale drag by submersion
        drag.x *= submerged_fraction;
        drag.y *= submerged_fraction;
        drag.z *= submerged_fraction;
        
        // Apply as acceleration
        body->velocity.x += drag.x * body->inv_mass * dt;
        body->velocity.y += drag.y * body->inv_mass * dt;
        body->velocity.z += drag.z * body->inv_mass * dt;
    }
}

void UpeFluidCoupling_Update(UpeFluidCoupling *coupling,
                            UpeFluidSph *fluid,
                            UpeBody *bodies,
                            uint32_t body_count,
                            float dt)
{
    if (!coupling || !fluid || !bodies) {
        return;
    }

    // Apply fluid forces to each body
    for (uint32_t i = 0; i < body_count && i < coupling->body_capacity; ++i) {
        UpeFluidCoupling_ApplyForcesToBody(coupling, fluid, &bodies[i], i, dt);
    }

    // Two-way coupling: apply body forces to fluid particles
    if (coupling->params.use_two_way_coupling) {
        // For each body, find nearby fluid particles and apply reaction forces
        for (uint32_t b = 0; b < body_count && b < coupling->body_capacity; ++b) {
            UpeBody* body = &bodies[b];
            if (body->inv_mass == 0.0f) continue;
            
            float body_radius = 0.5f; // Simplified
            float interaction_radius = body_radius + fluid->smoothing_radius;
            float ir2 = interaction_radius * interaction_radius;
            
            for (uint32_t p = 0; p < fluid->count; ++p) {
                UpeSphParticle* particle = &fluid->particles[p];
                
                // Check distance
                UpeVec3 diff;
                diff.x = particle->position.x - body->position.x;
                diff.y = particle->position.y - body->position.y;
                diff.z = particle->position.z - body->position.z;
                
                float dist2 = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                
                if (dist2 < ir2 && dist2 > 1e-6f) {
                    float dist = sqrtf(dist2);
                    
                    // Apply repulsion force to particle (body pushes fluid away)
                    float strength = (interaction_radius - dist) / interaction_radius;
                    UpeVec3 push_dir;
                    push_dir.x = diff.x / dist;
                    push_dir.y = diff.y / dist;
                    push_dir.z = diff.z / dist;
                    
                    // Transfer momentum from body to particle
                    float impulse_magnitude = body->mass * strength * 0.5f;
                    particle->velocity.x += push_dir.x * impulse_magnitude * dt;
                    particle->velocity.y += push_dir.y * impulse_magnitude * dt;
                    particle->velocity.z += push_dir.z * impulse_magnitude * dt;
                    
                    // Apply reaction force to body
                    body->velocity.x -= push_dir.x * impulse_magnitude * body->inv_mass * dt;
                    body->velocity.y -= push_dir.y * impulse_magnitude * body->inv_mass * dt;
                    body->velocity.z -= push_dir.z * impulse_magnitude * body->inv_mass * dt;
                }
            }
        }
    }
}
