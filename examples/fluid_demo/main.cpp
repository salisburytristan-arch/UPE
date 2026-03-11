#include <stdio.h>
#include <math.h>
#include "upe/upe_fluid_sph.h"
#include "upe/upe_buoyancy.h"
#include "upe/upe_body.h"

int main() {
    printf("=== Unified Physics Engine - Fluids & Buoyancy Demo ===\n\n");
    
    // Initialize SPH fluid system
    UpeFluidSph fluid;
    UpeFluidSph_Init(&fluid, 1000);
    
    UpeVec3 bounds_min = {-5, 0, -5};
    UpeVec3 bounds_max = {5, 10, 5};
    UpeVec3 gravity = {0, -9.81f, 0};
    
    UpeFluidSph_SetBounds(&fluid, bounds_min, bounds_max);
    UpeFluidSph_SetGravity(&fluid, gravity);
    
    printf("SPH Fluid Configuration:\n");
    printf("  Rest Density: %.0f kg/m³\n", fluid.rest_density);
    printf("  Gas Constant: %.0f\n", fluid.gas_constant);
    printf("  Viscosity: %.4f\n", fluid.viscosity);
    printf("  Smoothing Radius: %.2f m\n", fluid.smoothing_radius);
    printf("  Particle Mass: %.3f kg\n", fluid.particle_mass);
    printf("  Domain: [%.1f, %.1f, %.1f] to [%.1f, %.1f, %.1f]\n\n",
           bounds_min.x, bounds_min.y, bounds_min.z,
           bounds_max.x, bounds_max.y, bounds_max.z);
    
    // Create fluid block (dam break scenario)
    printf("Creating fluid block (dam break scenario)...\n");
    UpeVec3 vel_zero = {0, 0, 0};
    uint32_t particle_count = 0;
    
    for (int x = 0; x < 10; ++x) {
        for (int y = 0; y < 10; ++y) {
            for (int z = 0; z < 5; ++z) {
                UpeVec3 pos;
                pos.x = -4.5f + (float)x * 0.15f;
                pos.y = 1.0f + (float)y * 0.15f;
                pos.z = -2.0f + (float)z * 0.15f;
                
                if (UpeFluidSph_AddParticle(&fluid, pos, vel_zero) != 0xFFFFFFFF) {
                    particle_count++;
                }
            }
        }
    }
    
    printf("Added %u fluid particles\n\n", particle_count);
    
    // Create floating body
    UpeBodyDesc body_desc = {};
    body_desc.type = UPE_BODY_DYNAMIC;
    body_desc.position.x = 0;
    body_desc.position.y = 2.0f;
    body_desc.position.z = 0;
    body_desc.rotation.w = 1;
    body_desc.mass = 10.0f;  // 10 kg - should float in water
    
    UpeBody body;
    UpeBody_Init(&body, 1, &body_desc);
    
    printf("Floating Body:\n");
    printf("  Mass: %.1f kg\n", body.mass);
    printf("  Initial Position: [%.2f, %.2f, %.2f]\n\n",
           body.position.x, body.position.y, body.position.z);
    
    // Setup buoyancy parameters
    UpeBuoyancyDesc buoyancy_desc = {};
    buoyancy_desc.fluid_density = 1000.0f;  // Water
    buoyancy_desc.fluid_surface_normal.x = 0;
    buoyancy_desc.fluid_surface_normal.y = 1;
    buoyancy_desc.fluid_surface_normal.z = 0;
    buoyancy_desc.fluid_surface_height = 3.0f;  // Water level at y=3
    buoyancy_desc.drag_coefficient = 0.8f;
    buoyancy_desc.angular_drag = 0.5f;
    buoyancy_desc.sample_points = 27;
    
    printf("Simulating...\n");
    printf("Time (s) | Particles | Body Y-Pos | Buoyancy Force | Drag Force\n");
    printf("---------|-----------|------------|----------------|------------\n");
    
    float time = 0.0f;
    const float dt = 0.016f;  // 60 FPS
    const float sim_duration = 3.0f;
    int step_count = 0;
    
    while (time < sim_duration) {
        // Step fluid simulation
        UpeFluidSph_Step(&fluid, dt);
        
        // Apply buoyancy to floating body
        UpeBuoyancyState buoyancy_state;
        UpeBuoyancy_ApplyUniform(&body, &buoyancy_desc, &buoyancy_state);
        
        // Simple integration for body
        body.velocity.x += (body.force.x / body.mass) * dt;
        body.velocity.y += (body.force.y / body.mass) * dt;
        body.velocity.z += (body.force.z / body.mass) * dt;
        
        body.position.x += body.velocity.x * dt;
        body.position.y += body.velocity.y * dt;
        body.position.z += body.velocity.z * dt;
        
        UpeBody_ClearForces(&body);
        
        // Print status every 0.5 seconds
        if (step_count % 30 == 0) {
            float buoy_mag = sqrtf(
                buoyancy_state.buoyancy_force.x * buoyancy_state.buoyancy_force.x +
                buoyancy_state.buoyancy_force.y * buoyancy_state.buoyancy_force.y +
                buoyancy_state.buoyancy_force.z * buoyancy_state.buoyancy_force.z
            );
            float drag_mag = sqrtf(
                buoyancy_state.drag_force.x * buoyancy_state.drag_force.x +
                buoyancy_state.drag_force.y * buoyancy_state.drag_force.y +
                buoyancy_state.drag_force.z * buoyancy_state.drag_force.z
            );
            
            printf("%7.2f  | %9u | %10.3f | %14.2f | %10.2f\n",
                   time, fluid.count, body.position.y, buoy_mag, drag_mag);
        }
        
        time += dt;
        step_count++;
    }
    
    printf("\n=== Simulation Statistics ===\n");
    printf("Total Steps: %d\n", step_count);
    printf("Fluid Particles: %u\n", fluid.count);
    printf("Final Body Position: [%.3f, %.3f, %.3f]\n",
           body.position.x, body.position.y, body.position.z);
    printf("Final Body Velocity: [%.3f, %.3f, %.3f] m/s\n",
           body.velocity.x, body.velocity.y, body.velocity.z);
    
    // Analyze fluid behavior
    float avg_density = 0.0f;
    float avg_pressure = 0.0f;
    for (uint32_t i = 0; i < fluid.count; ++i) {
        avg_density += fluid.particles[i].density;
        avg_pressure += fluid.particles[i].pressure;
    }
    if (fluid.count > 0) {
        avg_density /= (float)fluid.count;
        avg_pressure /= (float)fluid.count;
    }
    
    printf("\nFluid Statistics:\n");
    printf("  Average Density: %.2f kg/m³\n", avg_density);
    printf("  Average Pressure: %.2f Pa\n", avg_pressure);
    
    // Cleanup
    UpeFluidSph_Destroy(&fluid);
    
    printf("\n=== Demo Complete ===\n");
    return 0;
}
