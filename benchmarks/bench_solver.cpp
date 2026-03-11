// Unified Physics Engine - Physics Solver Benchmarks

#include "upe/upe_solver.h"
#include "upe/upe_integrator.h"
#include "upe/upe_body.h"
#include "upe/upe_world_body_api.h"
#include "../core/src/world/upe_world.h"
#include "upe/upe_math.h"

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

// PGS Solver - 100 bodies stack (10 iterations)
BENCHMARK(PGS_Solver_100BodyStack, 20) {
    static UpeBodyState bodies[100];
    static UpeSolverConstraint constraints[99]; // 99 contact pairs
    
    // Initialize bodies in a stack
    for (int i = 0; i < 100; i++) {
        bodies[i].position.x = 0.0f;
        bodies[i].position.y = (float)i * 1.1f;
        bodies[i].position.z = 0.0f;
        bodies[i].velocity.x = 0.0f;
        bodies[i].velocity.y = 0.0f;
        bodies[i].velocity.z = 0.0f;
        bodies[i].rotation = UpeQuat_Identity();
        bodies[i].angular_velocity.x = 0.0f;
        bodies[i].angular_velocity.y = 0.0f;
        bodies[i].angular_velocity.z = 0.0f;
        bodies[i].inv_mass = (i == 0) ? 0.0f : 1.0f; // bottom is static
        bodies[i].inv_inertia.x = 1.0f;
        bodies[i].inv_inertia.y = 1.0f;
        bodies[i].inv_inertia.z = 1.0f;
    }
    
    // Create contacts between adjacent bodies
    for (int i = 0; i < 99; i++) {
        constraints[i].body_a = i;
        constraints[i].body_b = i + 1;
        constraints[i].normal.x = 0.0f;
        constraints[i].normal.y = 1.0f;
        constraints[i].normal.z = 0.0f;
        constraints[i].penetration = 0.1f;
        constraints[i].lambda = 0.0f;
    }
    
    // Run 10 PGS iterations
    for (int iter = 0; iter < 10; iter++) {
        UpeSolver_PgsIteration(bodies, 100, constraints, 99);
    }
}

// Euler Integration - 10k bodies
BENCHMARK(Integration_Euler_10kBodies, 50) {
    static UpeBody bodies[10000];
    
    // Initialize bodies
    for (int i = 0; i < 10000; i++) {
        UpeBody_Init(&bodies[i]);
        bodies[i].position.x = (float)(i % 100);
        bodies[i].position.y = (float)(i / 100);
        bodies[i].inv_mass = 1.0f;
        bodies[i].velocity.x = 1.0f;
        bodies[i].velocity.y = -1.0f;
    }
    
    // Apply gravity and integrate
    UpeVec3 gravity = {0.0f, -9.8f, 0.0f};
    float dt = 0.016f; // 60 FPS
    
    for (int i = 0; i < 10000; i++) {
        UpeBody_ApplyForce(&bodies[i], gravity);
        UpeIntegrator_Euler(&bodies[i], dt);
    }
}

// Semi-implicit Euler - 10k bodies with rotation
BENCHMARK(Integration_SemiImplicitEuler_10kBodies, 50) {
    static UpeBody bodies[10000];
    
    // Initialize bodies with rotation
    for (int i = 0; i < 10000; i++) {
        UpeBody_Init(&bodies[i]);
        bodies[i].position.x = (float)(i % 100);
        bodies[i].position.y = (float)(i / 100);
        bodies[i].inv_mass = 1.0f;
        bodies[i].velocity.x = 1.0f;
        bodies[i].velocity.y = -1.0f;
        bodies[i].angular_velocity.z = 0.5f;
    }
    
    // Apply forces and integrate
    UpeVec3 gravity = {0.0f, -9.8f, 0.0f};
    float dt = 0.016f;
    
    for (int i = 0; i < 10000; i++) {
        UpeBody_ApplyForce(&bodies[i], gravity);
        UpeIntegrator_SemiImplicitEuler(&bodies[i], dt);
    }
}

// Body creation benchmark using world API
BENCHMARK(BodyAPI_CreateDestroy_1000Bodies, 20) {
    UpeWorldDesc world_desc;
    world_desc.fixed_dt = 1.0f / 60.0f;
    UpeWorld *world = UpeWorld_Create(&world_desc);
    
    // Create 1000 bodies
    for (int i = 0; i < 1000; i++) {
        UpeBodyDesc desc;
        desc.type = UPE_BODY_DYNAMIC;
        desc.position.x = (float)(i % 32);
        desc.position.y = (float)(i / 32);
        desc.position.z = 0.0f;
        desc.rotation = UpeQuat_Identity();
        desc.mass = 1.0f;
        UpeWorld_CreateBody(world, &desc);
    }
    
    // Destroy half
    for (uint32_t i = 0; i < 500; i++) {
        UpeWorld_DestroyBody(world, i);
    }
    
    UpeWorld_Destroy(world);
}

// Mass properties calculation
BENCHMARK(MassProperties_1000Spheres, 100) {
    for (int i = 0; i < 1000; i++) {
        float radius = 0.5f + (float)(i % 10) * 0.1f;
        float mass = 1.0f;
        UpeVec3 inertia = UpeMassProperties_Sphere(radius, mass);
        // Use result to prevent optimization
        if (inertia.x < 0.0f) radius = 0.0f;
    }
}
