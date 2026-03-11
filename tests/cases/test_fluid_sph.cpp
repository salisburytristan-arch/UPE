#include "test_framework.h"
#include "upe/upe_fluid_sph.h"
#include <math.h>

TEST(test_sph_fluid_initialization) {
    UpeFluidSph fluid;
    UpeFluidSph_Init(&fluid, 1000);
    
    REQUIRE(fluid.capacity == 1000);
    REQUIRE(fluid.count == 0);
    REQUIRE(fluid.particles != NULL);
    REQUIRE(fluid.rest_density == 1000.0f);
    REQUIRE(fluid.poly6_coeff > 0.0f);
    
    UpeFluidSph_Destroy(&fluid);
}

TEST(test_sph_add_and_remove_particles) {
    UpeFluidSph fluid;
    UpeFluidSph_Init(&fluid, 100);
    
    UpeVec3 pos = {0, 0, 0};
    UpeVec3 vel = {1, 0, 0};
    
    uint32_t idx = UpeFluidSph_AddParticle(&fluid, pos, vel);
    REQUIRE(idx == 0);
    REQUIRE(fluid.count == 1);
    REQUIRE(fluid.particles[0].mass == fluid.particle_mass);
    
    UpeFluidSph_RemoveParticle(&fluid, 0);
    REQUIRE(fluid.count == 0);
    
    UpeFluidSph_Destroy(&fluid);
}

TEST(test_sph_set_bounds_and_gravity) {
    UpeFluidSph fluid;
    UpeFluidSph_Init(&fluid, 100);
    
    UpeVec3 min = {-10, -10, -10};
    UpeVec3 max = {10, 10, 10};
    UpeVec3 gravity = {0, -10, 0};
    
    UpeFluidSph_SetBounds(&fluid, min, max);
    UpeFluidSph_SetGravity(&fluid, gravity);
    
    REQUIRE(fluid.bounds_min.x == -10);
    REQUIRE(fluid.bounds_max.x == 10);
    REQUIRE(fluid.gravity.y == -10);
    
    UpeFluidSph_Destroy(&fluid);
}

TEST(test_sph_density_computation) {
    UpeFluidSph fluid;
    UpeFluidSph_Init(&fluid, 100);
    fluid.smoothing_radius = 0.2f;
    
    // Add particles in close proximity
    UpeVec3 p0 = {0, 0, 0};
    UpeVec3 p1 = {0.1f, 0, 0};
    UpeVec3 vel = {0, 0, 0};
    
    UpeFluidSph_AddParticle(&fluid, p0, vel);
    UpeFluidSph_AddParticle(&fluid, p1, vel);
    
    UpeFluidSph_ComputeDensityPressure(&fluid);
    
    REQUIRE(fluid.particles[0].density > 0.0f);
    REQUIRE(fluid.particles[1].density > 0.0f);
    
    UpeFluidSph_Destroy(&fluid);
}

TEST(test_sph_simulation_step) {
    UpeFluidSph fluid;
    UpeFluidSph_Init(&fluid, 500);
    
    UpeVec3 bounds_min = {-2, -2, -2};
    UpeVec3 bounds_max = {2, 2, 2};
    UpeFluidSph_SetBounds(&fluid, bounds_min, bounds_max);
    
    // Create a small fluid block
    UpeVec3 vel = {0, 0, 0};
    for (int x = 0; x < 3; ++x) {
        for (int y = 0; y < 3; ++y) {
            for (int z = 0; z < 3; ++z) {
                UpeVec3 pos;
                pos.x = -0.1f + (float)x * 0.1f;
                pos.y = 0.5f + (float)y * 0.1f;
                pos.z = -0.1f + (float)z * 0.1f;
                UpeFluidSph_AddParticle(&fluid, pos, vel);
            }
        }
    }
    
    uint32_t initial_count = fluid.count;
    
    // Run several steps
    for (int i = 0; i < 10; ++i) {
        UpeFluidSph_Step(&fluid, 0.016f);
    }
    
    // Particle count should remain constant
    REQUIRE(fluid.count == initial_count);
    
    UpeFluidSph_Destroy(&fluid);
}
