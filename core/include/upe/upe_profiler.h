#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
  Profiling & Performance Analysis
  
  - Frame timing
  - Subsystem breakdown
  - Memory tracking
  - GPU/CPU synchronization points
*/

#define UPE_PROF_MAX_ZONES 256
#define UPE_PROF_MAX_NAME_LENGTH 64

typedef struct UpeProfileZone {
    char name[UPE_PROF_MAX_NAME_LENGTH];
    uint64_t start_time;         // Microseconds
    uint64_t total_time;
    uint32_t call_count;
    uint32_t depth;              // Nesting level
    uint32_t parent_index;       // Index of parent zone (-1 for root)
} UpeProfileZone;

typedef struct UpeProfileFrame {
    UpeProfileZone zones[UPE_PROF_MAX_ZONES];
    uint32_t zone_count;
    
    uint64_t frame_start;
    uint64_t frame_end;
    uint32_t frame_number;
    
} UpeProfileFrame;

typedef struct UpeProfiler {
    UpeProfileFrame *frames;
    uint32_t frame_capacity;
    uint32_t current_frame;
    
    UpeProfileZone *active_zones; // Stack of active zones
    uint32_t active_zone_count;
    
    int enabled;
    
} UpeProfiler;

void UpeProfiler_Init(UpeProfiler *prof, uint32_t frame_history);
void UpeProfiler_Destroy(UpeProfiler *prof);

void UpeProfiler_Enable(UpeProfiler *prof);
void UpeProfiler_Disable(UpeProfiler *prof);

// Frame boundaries
void UpeProfiler_BeginFrame(UpeProfiler *prof);
void UpeProfiler_EndFrame(UpeProfiler *prof);

// Zone tracking
void UpeProfiler_BeginZone(UpeProfiler *prof, const char *name);
void UpeProfiler_EndZone(UpeProfiler *prof);

// Query
uint64_t UpeProfiler_GetAverageFrameTime(const UpeProfiler *prof, uint32_t frame_count);
uint64_t UpeProfiler_GetZoneTime(const UpeProfiler *prof, const char *zone_name);
void UpeProfiler_PrintReport(const UpeProfiler *prof);

// Chrome Tracing
void UpeProfiler_ExportChromeTrace(const UpeProfiler *prof, const char *filename, uint32_t frame_count);

// ========================
// Memory Tracking
// ========================

typedef struct UpeProfilerMemoryStats {
    uint64_t total_allocated;
    uint64_t total_freed;
    uint64_t current_usage;
    uint32_t allocation_count;
    uint32_t free_count;
    
    // Per-category tracking
    uint64_t category_usage[16];
    const char *category_names[16];
    uint32_t category_count;
    
} UpeProfilerMemoryStats;

void UpeMemory_Init(UpeProfilerMemoryStats *stats);
void UpeMemory_TrackAlloc(UpeProfilerMemoryStats *stats, uint32_t category, uint64_t size);
void UpeMemory_TrackFree(UpeProfilerMemoryStats *stats, uint32_t category, uint64_t size);
void UpeMemory_PrintReport(const UpeProfilerMemoryStats *stats);

// Helper macros
#define UPE_PROFILE_ZONE(prof, name) UpeProfiler_BeginZone(prof, name)
#define UPE_PROFILE_ZONE_END(prof) UpeProfiler_EndZone(prof)

#ifdef __cplusplus
}
#endif
