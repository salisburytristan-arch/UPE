// Unified Physics Engine - Fluid & Particle Benchmarks

#include "upe/upe_fluid_sph.h"
#include "upe/upe_particles.h"
#include "upe/upe_cloth.h"
#include "upe/upe_waves.h"

// Forward declare benchmark macro (MSVC compatible)
extern "C" void RegisterBenchmark(const char*, void(*)(void));
struct Benchmark { 
    const char* name;
    void (*func)(void);
    double avg_ms;
    double min_ms;
    double max_ms;
    int iterations;
};
extern Benchmark g_benchmarks[];
extern int g_benchmark_count;

#define BENCHMARK(name, iters) \
    static void Bench_##name(void); \
    struct Register_##name { Register_##name() { \
        RegisterBenchmark(#name, Bench_##name); \
        g_benchmarks[g_benchmark_count-1].iterations = iters; \
    } }; \
    static Register_##name g_register_##name; \
    static void Bench_##name(void)

// SPH Fluid - 10k particles, 1 step
BENCHMARK(SPH_10kParticles_DensityCompute, 5) {
    UpeSph sph;
    UpeSph_Init(&sph, 10000);
    
    // Initialize particles in a cube
    for (int i = 0; i < 10000; i++) {
        sph.positions[i].x = (float)(i % 22) * 0.3f;
        sph.positions[i].y = (float)((i / 22) % 22) * 0.3f;
        sph.positions[i].z = (float)(i / 484) * 0.3f;
        sph.velocities[i].x = 0.0f;
        sph.velocities[i].y = 0.0f;
        sph.velocities[i].z = 0.0f;
    }
    
    // Compute density (most expensive SPH operation)
    UpeSph_ComputeDensity(&sph);
    
    UpeSph_Destroy(&sph);
}

// SPH Fluid - Full step with forces
BENCHMARK(SPH_5kParticles_FullStep, 10) {
    UpeSph sph;
    UpeSph_Init(&sph, 5000);
    
    // Initialize particles
    for (int i = 0; i < 5000; i++) {
        sph.positions[i].x = (float)(i % 18) * 0.3f;
        sph.positions[i].y = (float)((i / 18) % 18) * 0.3f;
        sph.positions[i].z = (float)(i / 324) * 0.3f;
        sph.velocities[i].x = 0.0f;
        sph.velocities[i].y = 0.0f;
        sph.velocities[i].z = 0.0f;
    }
    
    // Run one full SPH step
    float dt = 0.001f;
    UpeSph_Step(&sph, dt);
    
    UpeSph_Destroy(&sph);
}

// Particle System - 50k particles update
BENCHMARK(Particles_50kUpdate, 20) {
    UpeParticleEmitter emitter;
    UpeParticleEmitter_Init(&emitter, 50000);
    
    // Spawn all particles
    for (int i = 0; i < 50000; i++) {
        UpeVec3 pos = {(float)(i % 100), (float)(i / 100), 0.0f};
        UpeVec3 vel = {0.0f, 1.0f, 0.0f};
        UpeParticleEmitter_Spawn(&emitter, pos, vel);
    }
    
    // Update particles
    float dt = 0.016f;
    UpeVec3 force = {0.0f, -9.8f, 0.0f};
    UpeParticleEmitter_Update(&emitter, dt, force);
    
    UpeParticleEmitter_Destroy(&emitter);
}

// Cloth Simulation - 50x50 grid, 10 PBD iterations
BENCHMARK(Cloth_50x50Grid_10Iterations, 10) {
    UpeCloth cloth;
    UpeCloth_Init(&cloth, 50, 50);
    
    // Pin top row
    for (int x = 0; x < 50; x++) {
        int idx = x; // top row
        cloth.pinned[idx] = 1;
    }
    
    // Run 10 PBD iterations
    float dt = 0.016f;
    UpeVec3 gravity = {0.0f, -9.8f, 0.0f};
    for (int iter = 0; iter < 10; iter++) {
        UpeCloth_Step(&cloth, dt, gravity);
    }
    
    UpeCloth_Destroy(&cloth);
}

// Wave Propagation - 128x128 grid, 100 steps
BENCHMARK(Waves_128x128Grid_100Steps, 5) {
    UpeWaves waves;
    UpeWaves_Init(&waves, 128, 128);
    
    // Disturb center
    UpeWaves_Disturb(&waves, 64, 64, 5.0f);
    
    // Propagate 100 steps
    float dt = 0.001f;
    for (int step = 0; step < 100; step++) {
        UpeWaves_Step(&waves, dt);
    }
    
    UpeWaves_Destroy(&waves);
}

// Thermodynamics - 64x64x64 grid heat diffusion
BENCHMARK(HeatDiffusion_64x64x64_100Steps, 3) {
    UpeThermoDynamics thermo;
    UpeThermoDynamics_Init(&thermo, 64, 64, 64);
    
    // Set hot spot in center
    int center = 32 + 32*64 + 32*64*64;
    thermo.temperature[center] = 1000.0f;
    
    // Run 100 diffusion steps
    float dt = 0.01f;
    for (int step = 0; step < 100; step++) {
        UpeThermoDynamics_Step(&thermo, dt);
    }
    
    UpeThermoDynamics_Destroy(&thermo);
}
