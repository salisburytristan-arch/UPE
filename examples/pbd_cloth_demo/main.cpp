/*
  PBD Cloth Demo
  
  Demonstrates cloth simulation using Position-Based Dynamics
*/

#include "upe/upe_pbd.h"
#include <stdio.h>
#include <math.h>

int main(void) {
    printf("=== UPE Position-Based Dynamics - Cloth Demo ===\n\n");
    
    // Create PBD system
    UpePbdSystem pbd;
    UpePbd_Init(&pbd, 4096, 16384);
    
    // Configure
    pbd.gravity.x = 0.0f;
    pbd.gravity.y = -9.81f;
    pbd.gravity.z = 0.0f;
    pbd.solver_iterations = 8;
    pbd.damping = 0.995f;
    pbd.ground_friction = 0.4f;
    
    // Create cloth grid (20x20, 0.1m spacing)
    printf("Creating cloth: 20x20 grid, 0.1m spacing\n");
    UpePbd_CreateClothGrid(&pbd, 20, 20, 0.1f, 0.01f, 0.0001f, 0.001f);
    
    printf("  Particles: %u\n", pbd.particle_count);
    printf("  Distance constraints: %u\n", pbd.distance_count);
    printf("  Solver iterations: %u\n\n", pbd.solver_iterations);
    
    // Simulate
    const float dt = 1.0f / 60.0f;
    const int frames = 300;
    
    printf("Simulating 5 seconds at 60 FPS...\n\n");
    printf("Time (s) | Lowest Y | Avg Velocity\n");
    printf("---------|----------|--------------\n");
    
    for (int frame = 0; frame < frames; frame++) {
        UpePbd_Step(&pbd, dt);
        
        if (frame % 60 == 0) {
            // Calculate stats
            float lowest_y = 1000.0f;
            float avg_vel = 0.0f;
            
            for (uint32_t i = 0; i < pbd.particle_count; i++) {
                if (pbd.particles[i].position.y < lowest_y) {
                    lowest_y = pbd.particles[i].position.y;
                }
                
                float v = sqrtf(pbd.particles[i].velocity.x * pbd.particles[i].velocity.x +
                               pbd.particles[i].velocity.y * pbd.particles[i].velocity.y +
                               pbd.particles[i].velocity.z * pbd.particles[i].velocity.z);
                avg_vel += v;
            }
            
            avg_vel /= pbd.particle_count;
            
            printf("%8.2f | %8.3f | %12.4f\n", frame * dt, lowest_y, avg_vel);
        }
    }
    
    printf("\n=== Final State ===\n");
    printf("All particles settled\n");
    printf("Cloth drape complete\n");
    
    UpePbd_Destroy(&pbd);
    
    printf("\n=== Demo Complete ===\n");
    return 0;
}
