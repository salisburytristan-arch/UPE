// Unified Physics Engine - Collision Detection Benchmarks

#include "upe/upe_narrowphase.h"
#include "upe/upe_geometry.h"
#include "upe/upe_contact.h"

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

// Narrowphase - Sphere-Sphere contacts (1000 pairs)
BENCHMARK(Narrowphase_SphereSphere_1000Contacts, 50) {
    UpeSphere sphere_a, sphere_b;
    sphere_a.center.x = 0.0f;
    sphere_a.center.y = 0.0f;
    sphere_a.center.z = 0.0f;
    sphere_a.radius = 1.0f;
    
    sphere_b.center.x = 1.5f;
    sphere_b.center.y = 0.0f;
    sphere_b.center.z = 0.0f;
    sphere_b.radius = 1.0f;
    
    UpeContactManifold manifold;
    for (int i = 0; i < 1000; i++) {
        // Slight variation to prevent optimization
        sphere_b.center.x = 1.5f + (float)(i % 10) * 0.01f;
        UpeNarrowphase_SphereSphere(sphere_a, sphere_b, &manifold);
    }
}

// Capsule-Capsule contacts (500 pairs)
BENCHMARK(Narrowphase_CapsuleCapsule_500Contacts, 50) {
    UpeCapsule cap_a, cap_b;
    cap_a.start.x = 0.0f;
    cap_a.start.y = 0.0f;
    cap_a.start.z = 0.0f;
    cap_a.end.x = 0.0f;
    cap_a.end.y = 2.0f;
    cap_a.end.z = 0.0f;
    cap_a.radius = 0.5f;
    
    cap_b.start.x = 1.2f;
    cap_b.start.y = 0.0f;
    cap_b.start.z = 0.0f;
    cap_b.end.x = 1.2f;
    cap_b.end.y = 2.0f;
    cap_b.end.z = 0.0f;
    cap_b.radius = 0.5f;
    
    UpeContactManifold manifold;
    for (int i = 0; i < 500; i++) {
        cap_b.start.x = 1.2f + (float)(i % 10) * 0.01f;
        cap_b.end.x = cap_b.start.x;
        UpeNarrowphase_CapsuleCapsule(cap_a, cap_b, &manifold);
    }
}

// AABB overlap tests (tight loop)
BENCHMARK(AABB_Overlap_1M_Tests, 10) {
    UpeAabb3 a, b;
    a.min.x = 0.f; a.min.y = 0.f; a.min.z = 0.f;
    a.max.x = 1.f; a.max.y = 1.f; a.max.z = 1.f;
    b.min.x = 0.5f; b.min.y = 0.5f; b.min.z = 0.5f;
    b.max.x = 1.5f; b.max.y = 1.5f; b.max.z = 1.5f;
    
    UpeContactManifold manifold;
    int overlaps = 0;
    for (int i = 0; i < 100000; i++) {
        // Slight offset to prevent optimization
        b.min.x = 0.5f + (float)(i & 1) * 0.001f;
        if (UpeNarrowphase_AabbAabb(a, b, &manifold)) overlaps++;
    }
}
