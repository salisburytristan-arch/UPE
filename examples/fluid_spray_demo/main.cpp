/*
  Fluid Spray/Foam Demo
  
  Demonstrates:
  - SPH fluid simulation
  - Automatic spray/foam/bubble emission from fluid
  - Secondary particle rendering
  - Velocity-based spray criteria
*/

#include "upe/upe_fluid_sph.h"
#include "upe/upe_fluid_spray.h"
#include "upe/upe_particles.h"
#include "upe/upe_emitter.h"
#include <stdio.h>
#include <math.h>

int main(void) {
    printf("=== UPE Fluid Spray/Foam Demo ===\n\n");
    
    // Create SPH fluid
    UpeFluidSph fluid;
    UpeFluidSph_Init(&fluid, 500);
    
    UpeVec3 bounds_min = {-3.0f, 0.0f, -3.0f};
    UpeVec3 bounds_max = {3.0f, 5.0f, 3.0f};
    UpeFluidSph_SetBounds(&fluid, bounds_min, bounds_max);
    
    UpeVec3 gravity = {0.0f, -9.81f, 0.0f};
    UpeFluidSph_SetGravity(&fluid, gravity);
    
    printf("SPH Fluid Configuration:\n");
    printf("  Particles: 500\n");
    printf("  Domain: [%.1f, %.1f, %.1f] to [%.1f, %.1f, %.1f]\n",
           bounds_min.x, bounds_min.y, bounds_min.z,
           bounds_max.x, bounds_max.y, bounds_max.z);
    
    // Add fluid particles (falling column)
    uint32_t added = 0;
    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 5; y++) {
            for (int z = 0; z < 10; z++) {
                UpeVec3 pos;
                pos.x = -1.0f + x * 0.2f;
                pos.y = 2.0f + y * 0.2f;
                pos.z = -1.0f + z * 0.2f;
                
                UpeVec3 vel;
                vel.x = 0.0f;
                vel.y = -2.0f; // Initial downward velocity
                vel.z = 0.0f;
                
                if (UpeFluidSph_AddParticle(&fluid, pos, vel) != 0xFFFFFFFF) {
                    added++;
                }
            }
        }
    }
    
    printf("  Added: %u particles\n\n", added);
    
    // Create particle system for secondary particles
    UpeParticleSystem secondary_particles;
    UpeParticleSystem_Init(&secondary_particles, 2048);
    
    // Create emitter pool
    UpeEmitterPool *emitter_pool = UpeEmitterPool_Create(100, 512);
    if (!emitter_pool) {
        printf("Failed to create emitter pool\n");
        return 1;
    }
    
    // Create spray system
    UpeFluidSpraySystem spray_system;
    UpeFluidSpray_Init(&spray_system, &fluid, emitter_pool);
    
    // Customize spray parameters for more visible effects
    spray_system.params.spray_velocity_threshold = 1.5f;
    spray_system.params.spray_emission_rate = 0.8f;
    spray_system.params.spray_lifetime = 1.5f;
    spray_system.params.foam_emission_rate = 0.4f;
    spray_system.params.foam_lifetime = 2.5f;
    spray_system.params.bubble_emission_rate = 0.3f;
    spray_system.params.emission_probability = 0.5f;
    spray_system.params.max_emissions_per_frame = 50;
    
    printf("Spray System Configured:\n");
    printf("  Spray threshold: %.2f m/s\n", spray_system.params.spray_velocity_threshold);
    printf("  Emission probability: %.0f%%\n", spray_system.params.emission_probability * 100.0f);
    printf("  Max emissions/frame: %u\n\n", spray_system.params.max_emissions_per_frame);
    
    printf("Simulating 3 seconds at 60 FPS...\n\n");
    printf("Time (s) | Fluid | Spray | Foam | Bubbles | Secondary\n");
    printf("---------|-------|-------|------|---------|----------\n");
    
    const float dt = 1.0f / 60.0f;
    const int total_frames = 180;
    
    for (int frame = 0; frame < total_frames; frame++) {
        float time = frame * dt;
        
        // Step SPH fluid
        UpeFluidSph_Step(&fluid, dt);
        
        // Update spray system (analyzes SPH and emits secondary particles)
        UpeFluidSpray_Update(&spray_system, dt);
        
        // Update emitter pool
        UpeEmitterPool_Update(emitter_pool, dt);
        
        // Get emission requests
        UpeParticleSpawnRequest requests[512];
        uint32_t request_count = UpeEmitterPool_GetEmissionQueue(emitter_pool, requests, 512);
        
        // Emit secondary particles
        if (request_count > 0) {
            UpeParticleSystem_EmitFromRequests(&secondary_particles, requests, request_count);
        }
        
        // Step secondary particles
        UpeParticleSystem_Step(&secondary_particles, dt, gravity);
        
        // Output every 30 frames (0.5 seconds)
        if (frame % 30 == 0) {
            uint32_t spray_count, foam_count, bubble_count;
            UpeFluidSpray_GetEmissionStats(&spray_system, &spray_count, &foam_count, &bubble_count);
            
            printf("%8.2f | %5u | %5u | %4u | %7u | %8u\n",
                   time,
                   fluid.count,
                   spray_count,
                   foam_count,
                   bubble_count,
                   secondary_particles.count);
        }
    }
    
    printf("\n=== Final Statistics ===\n");
    uint32_t spray_count, foam_count, bubble_count;
    UpeFluidSpray_GetEmissionStats(&spray_system, &spray_count, &foam_count, &bubble_count);
    
    printf("Fluid Particles: %u\n", fluid.count);
    printf("Secondary Particles: %u\n", secondary_particles.count);
    printf("\nEmission Breakdown:\n");
    printf("  Spray:   %u (high velocity)\n", spray_count);
    printf("  Foam:    %u (surface turbulence)\n", foam_count);
    printf("  Bubbles: %u (cavitation)\n", bubble_count);
    printf("  Total:   %u\n", UpeFluidSpray_GetTotalEmitted(&spray_system));
    
    // Cleanup
    UpeEmitterPool_Destroy(emitter_pool);
    UpeParticleSystem_Destroy(&secondary_particles);
    UpeFluidSph_Destroy(&fluid);
    
    printf("\n=== Demo Complete ===\n");
    return 0;
}
