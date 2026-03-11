// Unified Physics Engine - Standard Scene Benchmarks
// Stack, Pile, Avalanche, Vehicle - Standard benchmark scenarios

#include "upe/upe_world.h"
#include "upe/upe_body.h"
#include "upe/upe_shape.h"
#include "upe/upe_math.h"

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

// Stack benchmark: 20 boxes stacked vertically
BENCHMARK(Scene_Stack_20Boxes, 10) {
    UpeWorldDesc world_desc;
    world_desc.fixed_dt = 1.0f / 60.0f;
    UpeWorld *world = UpeWorld_Create(&world_desc);
    
    // Ground plane (static body at y=0)
    UpeBodyDesc ground_desc;
    ground_desc.type = UPE_BODY_STATIC;
    ground_desc.position = {0.0f, -0.5f, 0.0f};
    ground_desc.rotation = UpeQuat_Identity();
    ground_desc.mass = 0.0f;
    uint32_t ground_id = UpeWorld_CreateBody(world, &ground_desc);
    
    UpeShapeDesc ground_shape;
    ground_shape.type = UPE_SHAPE_BOX;
    ground_shape.box.extents = {50.0f, 0.5f, 50.0f};
    UpeWorld_AddShapeToBody(world, ground_id, &ground_shape);
    
    // Stack 20 boxes
    for (int i = 0; i < 20; i++) {
        UpeBodyDesc desc;
        desc.type = UPE_BODY_DYNAMIC;
        desc.position.x = 0.0f;
        desc.position.y = 0.5f + (float)i * 1.01f;
        desc.position.z = 0.0f;
        desc.rotation = UpeQuat_Identity();
        desc.mass = 1.0f;
        
        uint32_t body_id = UpeWorld_CreateBody(world, &desc);
        
        UpeShapeDesc shape;
        shape.type = UPE_SHAPE_BOX;
        shape.box.extents = {0.5f, 0.5f, 0.5f};
        UpeWorld_AddShapeToBody(world, body_id, &shape);
    }
    
    // Simulate 60 frames (1 second)
    for (int frame = 0; frame < 60; frame++) {
        UpeWorld_Step(world, 1.0f / 60.0f);
    }
    
    UpeWorld_Destroy(world);
}

// Pile benchmark: 100 boxes in random pile
BENCHMARK(Scene_Pile_100Boxes, 10) {
    UpeWorldDesc world_desc;
    world_desc.fixed_dt = 1.0f / 60.0f;
    UpeWorld *world = UpeWorld_Create(&world_desc);
    
    // Ground
    UpeBodyDesc ground_desc;
    ground_desc.type = UPE_BODY_STATIC;
    ground_desc.position = {0.0f, -0.5f, 0.0f};
    ground_desc.rotation = UpeQuat_Identity();
    ground_desc.mass = 0.0f;
    uint32_t ground_id = UpeWorld_CreateBody(world, &ground_desc);
    
    UpeShapeDesc ground_shape;
    ground_shape.type = UPE_SHAPE_BOX;
    ground_shape.box.extents = {50.0f, 0.5f, 50.0f};
    UpeWorld_AddShapeToBody(world, ground_id, &ground_shape);
    
    // Create pile: 10x10 grid at different heights
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            UpeBodyDesc desc;
            desc.type = UPE_BODY_DYNAMIC;
            desc.position.x = (float)(i - 5) * 1.2f;
            desc.position.y = 1.0f + (float)((i * j) % 5) * 1.1f;
            desc.position.z = (float)(j - 5) * 1.2f;
            desc.rotation = UpeQuat_Identity();
            desc.mass = 1.0f;
            
            uint32_t body_id = UpeWorld_CreateBody(world, &desc);
            
            UpeShapeDesc shape;
            shape.type = UPE_SHAPE_BOX;
            shape.box.extents = {0.5f, 0.5f, 0.5f};
            UpeWorld_AddShapeToBody(world, body_id, &shape);
        }
    }
    
    // Simulate 60 frames
    for (int frame = 0; frame < 60; frame++) {
        UpeWorld_Step(world, 1.0f / 60.0f);
    }
    
    UpeWorld_Destroy(world);
}

// Avalanche benchmark: 200 spheres rolling down slope
BENCHMARK(Scene_Avalanche_200Spheres, 5) {
    UpeWorldDesc world_desc;
    world_desc.fixed_dt = 1.0f / 60.0f;
    UpeWorld *world = UpeWorld_Create(&world_desc);
    
    // Sloped ground (30 degree angle)
    UpeBodyDesc ground_desc;
    ground_desc.type = UPE_BODY_STATIC;
    ground_desc.position = {0.0f, 0.0f, 0.0f};
    // Rotate around X axis for slope
    float angle = 0.523599f; // 30 degrees in radians
    ground_desc.rotation = UpeQuat_FromAxisAngle({1.0f, 0.0f, 0.0f}, angle);
    ground_desc.mass = 0.0f;
    uint32_t ground_id = UpeWorld_CreateBody(world, &ground_desc);
    
    UpeShapeDesc ground_shape;
    ground_shape.type = UPE_SHAPE_BOX;
    ground_shape.box.extents = {50.0f, 0.5f, 50.0f};
    UpeWorld_AddShapeToBody(world, ground_id, &ground_shape);
    
    // Create 200 spheres at top of slope
    for (int i = 0; i < 200; i++) {
        UpeBodyDesc desc;
        desc.type = UPE_BODY_DYNAMIC;
        desc.position.x = (float)((i % 20) - 10) * 0.6f;
        desc.position.y = 10.0f + (float)(i / 20) * 0.6f;
        desc.position.z = -15.0f;
        desc.rotation = UpeQuat_Identity();
        desc.mass = 1.0f;
        
        uint32_t body_id = UpeWorld_CreateBody(world, &desc);
        
        UpeShapeDesc shape;
        shape.type = UPE_SHAPE_SPHERE;
        shape.sphere.radius = 0.25f;
        UpeWorld_AddShapeToBody(world, body_id, &shape);
    }
    
    // Simulate 120 frames (2 seconds)
    for (int frame = 0; frame < 120; frame++) {
        UpeWorld_Step(world, 1.0f / 60.0f);
    }
    
    UpeWorld_Destroy(world);
}

// 1000 body stress test
BENCHMARK(Scene_StressTest_1000Bodies, 5) {
    UpeWorldDesc world_desc;
    world_desc.fixed_dt = 1.0f / 60.0f;
    UpeWorld *world = UpeWorld_Create(&world_desc);
    
    // Ground
    UpeBodyDesc ground_desc;
    ground_desc.type = UPE_BODY_STATIC;
    ground_desc.position = {0.0f, -0.5f, 0.0f};
    ground_desc.rotation = UpeQuat_Identity();
    ground_desc.mass = 0.0f;
    uint32_t ground_id = UpeWorld_CreateBody(world, &ground_desc);
    
    UpeShapeDesc ground_shape;
    ground_shape.type = UPE_SHAPE_BOX;
    ground_shape.box.extents = {100.0f, 0.5f, 100.0f};
    UpeWorld_AddShapeToBody(world, ground_id, &ground_shape);
    
    // Create 1000 bodies in a large grid
    for (int i = 0; i < 1000; i++) {
        UpeBodyDesc desc;
        desc.type = UPE_BODY_DYNAMIC;
        desc.position.x = (float)((i % 32) - 16) * 2.0f;
        desc.position.y = 1.0f + (float)((i / 32) % 10) * 2.2f;
        desc.position.z = (float)((i / 320) - 1) * 2.0f;
        desc.rotation = UpeQuat_Identity();
        desc.mass = 1.0f;
        
        uint32_t body_id = UpeWorld_CreateBody(world, &desc);
        
        UpeShapeDesc shape;
        // Alternate between box and sphere
        if (i % 2 == 0) {
            shape.type = UPE_SHAPE_BOX;
            shape.box.extents = {0.5f, 0.5f, 0.5f};
        } else {
            shape.type = UPE_SHAPE_SPHERE;
            shape.sphere.radius = 0.5f;
        }
        UpeWorld_AddShapeToBody(world, body_id, &shape);
    }
    
    // Simulate 60 frames
    for (int frame = 0; frame < 60; frame++) {
        UpeWorld_Step(world, 1.0f / 60.0f);
    }
    
    UpeWorld_Destroy(world);
}

// Profiler overhead test
BENCHMARK(Scene_ProfilerOverhead_100Bodies, 20) {
    UpeWorldDesc world_desc;
    world_desc.fixed_dt = 1.0f / 60.0f;
    UpeWorld *world = UpeWorld_Create(&world_desc);
    
    // Ground
    UpeBodyDesc ground_desc;
    ground_desc.type = UPE_BODY_STATIC;
    ground_desc.position = {0.0f, -0.5f, 0.0f};
    ground_desc.rotation = UpeQuat_Identity();
    ground_desc.mass = 0.0f;
    uint32_t ground_id = UpeWorld_CreateBody(world, &ground_desc);
    
    UpeShapeDesc ground_shape;
    ground_shape.type = UPE_SHAPE_BOX;
    ground_shape.box.extents = {50.0f, 0.5f, 50.0f};
    UpeWorld_AddShapeToBody(world, ground_id, &ground_shape);
    
    // Create 100 bodies
    for (int i = 0; i < 100; i++) {
        UpeBodyDesc desc;
        desc.type = UPE_BODY_DYNAMIC;
        desc.position.x = (float)((i % 10) - 5) * 1.2f;
        desc.position.y = 1.0f + (float)(i / 10) * 1.2f;
        desc.position.z = 0.0f;
        desc.rotation = UpeQuat_Identity();
        desc.mass = 1.0f;
        
        uint32_t body_id = UpeWorld_CreateBody(world, &desc);
        
        UpeShapeDesc shape;
        shape.type = UPE_SHAPE_BOX;
        shape.box.extents = {0.5f, 0.5f, 0.5f};
        UpeWorld_AddShapeToBody(world, body_id, &shape);
    }
    
    // Enable profiling and run
    if (world->profiler) {
        UpeProfiler_Enable(world->profiler);
    }
    
    // Simulate 10 frames with profiling
    for (int frame = 0; frame < 10; frame++) {
        UpeWorld_Step(world, 1.0f / 60.0f);
    }
    
    UpeWorld_Destroy(world);
}
