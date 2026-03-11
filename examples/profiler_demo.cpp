// Unified Physics Engine - Profiler Demo
// Demonstrates profiler usage with Chrome tracing export

#include "upe/upe_world.h"
#include "upe/upe_profiler.h"
#include "upe/upe_allocator.h"
#include <stdio.h>

int main() {
    printf("UPE Profiler Demo\n");
    printf("=================\n\n");
    
    // Enable memory profiling
    UpeAllocator_EnableMemoryProfiling();
    
    // Create world with profiler
    UpeWorldDesc world_desc;
    world_desc.fixed_dt = 1.0f / 60.0f;
    UpeWorld *world = UpeWorld_Create(&world_desc);
    
    if (!world || !world->profiler) {
        printf("ERROR: Failed to create world or profiler\n");
        return 1;
    }
    
    printf("Creating test scene with 100 boxes...\n");
    
    // Create ground
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
    
    // Create 10x10 grid of boxes
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            UpeBodyDesc desc;
            desc.type = UPE_BODY_DYNAMIC;
            desc.position.x = (float)(i - 5) * 1.2f;
            desc.position.y = 1.0f + (float)j * 1.2f;
            desc.position.z = 0.0f;
            desc.rotation = UpeQuat_Identity();
            desc.mass = 1.0f;
            
            uint32_t body_id = UpeWorld_CreateBody(world, &desc);
            
            UpeShapeDesc shape;
            shape.type = UPE_SHAPE_BOX;
            shape.box.extents = {0.5f, 0.5f, 0.5f};
            UpeWorld_AddShapeToBody(world, body_id, &shape);
        }
    }
    
    printf("\nRunning simulation for 180 frames (3 seconds)...\n");
    
    // Run simulation with profiling
    UpeProfiler_Enable(world->profiler);
    
    for (int frame = 0; frame < 180; frame++) {
        UpeWorld_Step(world, 1.0f / 60.0f);
        
        // Print progress every 60 frames
        if ((frame + 1) % 60 == 0) {
            printf("  Frame %d/%d\n", frame + 1, 180);
        }
    }
    
    printf("\nSimulation complete!\n\n");
    
    // Print profiler report
    printf("Profiler Statistics:\n");
    printf("--------------------\n");
    UpeProfiler_PrintReport(world->profiler);
    printf("\n");
    
    // Export Chrome trace
    printf("Exporting Chrome trace to 'profiler_trace.json'...\n");
    UpeProfiler_ExportChromeTrace(world->profiler, "profiler_trace.json", 180);
    printf("\nYou can view the trace by:\n");
    printf("  1. Open Chrome browser\n");
    printf("  2. Go to chrome://tracing\n");
    printf("  3. Click 'Load' and select profiler_trace.json\n");
    printf("\n");
    
    // Print memory statistics
    printf("Memory Statistics:\n");
    printf("------------------\n");
    UpeAllocator_PrintMemoryReport();
    printf("\n");
    
    // Calculate average frame time
    uint64_t avg_frame_time = UpeProfiler_GetAverageFrameTime(world->profiler, 180);
    float avg_frame_ms = avg_frame_time / 1000.0f;
    float avg_fps = (avg_frame_ms > 0.0f) ? (1000.0f / avg_frame_ms) : 0.0f;
    
    printf("Performance Summary:\n");
    printf("--------------------\n");
    printf("Average Frame Time: %.2f ms\n", avg_frame_ms);
    printf("Average FPS:        %.1f\n", avg_fps);
    printf("Body Count:         %u\n", world->body_count);
    printf("\n");
    
    // Get specific zone timings
    uint64_t broadphase_time = UpeProfiler_GetZoneTime(world->profiler, "Broadphase");
    uint64_t narrowphase_time = UpeProfiler_GetZoneTime(world->profiler, "Narrowphase");
    uint64_t solver_time = UpeProfiler_GetZoneTime(world->profiler, "Solver");
    uint64_t integration_time = UpeProfiler_GetZoneTime(world->profiler, "Integration");
    
    if (broadphase_time > 0 || narrowphase_time > 0 || solver_time > 0 || integration_time > 0) {
        printf("Phase Breakdown (last frame):\n");
        printf("-----------------------------\n");
        if (broadphase_time > 0)
            printf("  Broadphase:   %.2f ms\n", broadphase_time / 1000.0f);
        if (narrowphase_time > 0)
            printf("  Narrowphase:  %.2f ms\n", narrowphase_time / 1000.0f);
        if (solver_time > 0)
            printf("  Solver:       %.2f ms\n", solver_time / 1000.0f);
        if (integration_time > 0)
            printf("  Integration:  %.2f ms\n", integration_time / 1000.0f);
        printf("\n");
    }
    
    // Identify hotspot
    const char *hotspot = "Solver"; // Default assumption
    uint64_t max_time = solver_time;
    
    if (broadphase_time > max_time) {
        hotspot = "Broadphase";
        max_time = broadphase_time;
    }
    if (narrowphase_time > max_time) {
        hotspot = "Narrowphase";
        max_time = narrowphase_time;
    }
    if (integration_time > max_time) {
        hotspot = "Integration";
        max_time = integration_time;
    }
    
    if (max_time > 0) {
        printf("Hotspot Identified: %s (%.2f ms, %.1f%% of frame)\n",
            hotspot,
            max_time / 1000.0f,
            avg_frame_time > 0 ? (max_time * 100.0f / avg_frame_time) : 0.0f);
        printf("\n");
    }
    
    printf("Demo complete!\n");
    
    UpeWorld_Destroy(world);
    return 0;
}
