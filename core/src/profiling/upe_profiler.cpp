#include "upe/upe_profiler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
static uint64_t UpeProfiler_GetMicroseconds() {
    static LARGE_INTEGER frequency;
    static int initialized = 0;
    if (!initialized) {
        QueryPerformanceFrequency(&frequency);
        initialized = 1;
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (counter.QuadPart * 1000000ULL) / frequency.QuadPart;
}
#else
#include <sys/time.h>
static uint64_t UpeProfiler_GetMicroseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}
#endif

void UpeProfiler_Init(UpeProfiler *prof, uint32_t frame_history) {
    if (!prof) {
        return;
    }
    
    prof->frame_capacity = frame_history;
    prof->current_frame = 0;
    prof->enabled = 1;
    
    prof->frames = (UpeProfileFrame*)malloc(frame_history * sizeof(UpeProfileFrame));
    if (prof->frames) {
        memset(prof->frames, 0, frame_history * sizeof(UpeProfileFrame));
    }
    
    prof->active_zones = (UpeProfileZone*)malloc(UPE_PROF_MAX_ZONES * sizeof(UpeProfileZone));
    prof->active_zone_count = 0;
}

void UpeProfiler_Destroy(UpeProfiler *prof) {
    if (!prof) {
        return;
    }
    
    free(prof->frames);
    free(prof->active_zones);
    
    prof->frames = NULL;
    prof->active_zones = NULL;
}

void UpeProfiler_Enable(UpeProfiler *prof) {
    if (prof) {
        prof->enabled = 1;
    }
}

void UpeProfiler_Disable(UpeProfiler *prof) {
    if (prof) {
        prof->enabled = 0;
    }
}

void UpeProfiler_BeginFrame(UpeProfiler *prof) {
    if (!prof || !prof->enabled) {
        return;
    }
    
    // Advance to next frame
    prof->current_frame = (prof->current_frame + 1) % prof->frame_capacity;
    UpeProfileFrame *frame = &prof->frames[prof->current_frame];
    
    frame->zone_count = 0;
    frame->frame_start = UpeProfiler_GetMicroseconds();
    frame->frame_number++;
    
    prof->active_zone_count = 0;
}

void UpeProfiler_EndFrame(UpeProfiler *prof) {
    if (!prof || !prof->enabled) {
        return;
    }
    
    UpeProfileFrame *frame = &prof->frames[prof->current_frame];
    frame->frame_end = UpeProfiler_GetMicroseconds();
}

void UpeProfiler_BeginZone(UpeProfiler *prof, const char *name) {
    if (!prof || !prof->enabled || !name) {
        return;
    }
    
    UpeProfileFrame *frame = &prof->frames[prof->current_frame];
    if (frame->zone_count >= UPE_PROF_MAX_ZONES) {
        return;
    }
    
    uint32_t zone_idx = frame->zone_count++;
    UpeProfileZone *zone = &frame->zones[zone_idx];
    
    strncpy(zone->name, name, UPE_PROF_MAX_NAME_LENGTH - 1);
    zone->name[UPE_PROF_MAX_NAME_LENGTH - 1] = '\0';
    
    zone->start_time = UpeProfiler_GetMicroseconds();
    zone->total_time = 0;
    zone->call_count = 1;
    zone->depth = prof->active_zone_count;
    zone->parent_index = (prof->active_zone_count > 0) ? prof->active_zone_count - 1 : 0xFFFFFFFF;
    
    // Push onto active zone stack
    if (prof->active_zone_count < UPE_PROF_MAX_ZONES) {
        prof->active_zones[prof->active_zone_count] = *zone;
        prof->active_zone_count++;
    }
}

void UpeProfiler_EndZone(UpeProfiler *prof) {
    if (!prof || !prof->enabled || prof->active_zone_count == 0) {
        return;
    }
    
    uint64_t end_time = UpeProfiler_GetMicroseconds();
    
    // Pop from active zone stack
    prof->active_zone_count--;
    UpeProfileZone *active_zone = &prof->active_zones[prof->active_zone_count];
    
    // Find matching zone in current frame
    UpeProfileFrame *frame = &prof->frames[prof->current_frame];
    for (uint32_t i = 0; i < frame->zone_count; ++i) {
        if (strcmp(frame->zones[i].name, active_zone->name) == 0 &&
            frame->zones[i].depth == active_zone->depth) {
            frame->zones[i].total_time = end_time - frame->zones[i].start_time;
            break;
        }
    }
}

uint64_t UpeProfiler_GetAverageFrameTime(const UpeProfiler *prof, uint32_t frame_count) {
    if (!prof || frame_count == 0 || frame_count > prof->frame_capacity) {
        return 0;
    }
    
    uint64_t total = 0;
    for (uint32_t i = 0; i < frame_count; ++i) {
        uint32_t frame_idx = (prof->current_frame + prof->frame_capacity - i) % prof->frame_capacity;
        const UpeProfileFrame *frame = &prof->frames[frame_idx];
        total += (frame->frame_end - frame->frame_start);
    }
    
    return total / frame_count;
}

uint64_t UpeProfiler_GetZoneTime(const UpeProfiler *prof, const char *zone_name) {
    if (!prof || !zone_name) {
        return 0;
    }
    
    const UpeProfileFrame *frame = &prof->frames[prof->current_frame];
    for (uint32_t i = 0; i < frame->zone_count; ++i) {
        if (strcmp(frame->zones[i].name, zone_name) == 0) {
            return frame->zones[i].total_time;
        }
    }
    
    return 0;
}

void UpeProfiler_PrintReport(const UpeProfiler *prof) {
    if (!prof) {
        return;
    }
    
    const UpeProfileFrame *frame = &prof->frames[prof->current_frame];
    uint64_t frame_time = frame->frame_end - frame->frame_start;
    
    printf("=== Profiler Report (Frame %u) ===\n", frame->frame_number);
    printf("Frame Time: %.2f ms\n", frame_time / 1000.0f);
    printf("\n%-40s %12s %12s %8s\n", "Zone", "Time (ms)", "% Frame", "Calls");
    printf("------------------------------------------------------------------------\n");
    
    for (uint32_t i = 0; i < frame->zone_count; ++i) {
        const UpeProfileZone *zone = &frame->zones[i];
        
        // Print indentation based on depth
        char indent[64] = {0};
        for (uint32_t d = 0; d < zone->depth && d < 10; ++d) {
            strcat(indent, "  ");
        }
        
        char name_with_indent[128];
        snprintf(name_with_indent, sizeof(name_with_indent), "%s%s", indent, zone->name);
        
        float time_ms = zone->total_time / 1000.0f;
        float percent = frame_time > 0 ? (zone->total_time * 100.0f / frame_time) : 0.0f;
        
        printf("%-40s %12.2f %11.1f%% %8u\n", name_with_indent, time_ms, percent, zone->call_count);
    }
    
    printf("========================================================================\n");
}

// ========================
// Chrome Tracing Export
// ========================

void UpeProfiler_ExportChromeTrace(const UpeProfiler *prof, const char *filename, uint32_t frame_count) {
    if (!prof || !filename || frame_count == 0) {
        return;
    }
    
    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("Failed to open %s for writing\n", filename);
        return;
    }
    
    // Clamp frame count to available frames
    if (frame_count > prof->frame_capacity) {
        frame_count = prof->frame_capacity;
    }
    
    // Write JSON header
    fprintf(file, "{\n");
    fprintf(file, "  \"traceEvents\": [\n");
    
    int first_event = 1;
    
    // Export frames in chronological order
    for (uint32_t f = 0; f < frame_count; ++f) {
        uint32_t frame_idx = (prof->current_frame + prof->frame_capacity - frame_count + f + 1) % prof->frame_capacity;
        const UpeProfileFrame *frame = &prof->frames[frame_idx];
        
        if (frame->zone_count == 0) {
            continue;
        }
        
        // Write frame marker
        if (!first_event) {
            fprintf(file, ",\n");
        }
        first_event = 0;
        
        fprintf(file, "    {\"name\": \"Frame\", \"cat\": \"frame\", \"ph\": \"X\", \"ts\": %llu, \"dur\": %llu, \"pid\": 0, \"tid\": 0, \"args\": {\"frame\": %u}}",
            (unsigned long long)frame->frame_start,
            (unsigned long long)(frame->frame_end - frame->frame_start),
            frame->frame_number);
        
        // Write all zones in this frame
        for (uint32_t i = 0; i < frame->zone_count; ++i) {
            const UpeProfileZone *zone = &frame->zones[i];
            
            fprintf(file, ",\n");
            fprintf(file, "    {\"name\": \"%s\", \"cat\": \"physics\", \"ph\": \"X\", \"ts\": %llu, \"dur\": %llu, \"pid\": 0, \"tid\": 0, \"args\": {\"depth\": %u}}",
                zone->name,
                (unsigned long long)zone->start_time,
                (unsigned long long)zone->total_time,
                zone->depth);
        }
    }
    
    // Write JSON footer
    fprintf(file, "\n  ],\n");
    fprintf(file, "  \"displayTimeUnit\": \"ms\",\n");
    fprintf(file, "  \"otherData\": {\n");
    fprintf(file, "    \"version\": \"UPE Profiler 1.0\"\n");
    fprintf(file, "  }\n");
    fprintf(file, "}\n");
    
    fclose(file);
    printf("Chrome trace exported to %s (%u frames)\n", filename, frame_count);
}

// ========================
// Memory Tracking
// ========================

void UpeMemory_Init(UpeProfilerMemoryStats *stats) {
    if (!stats) {
        return;
    }
    
    memset(stats, 0, sizeof(UpeProfilerMemoryStats));
}

void UpeMemory_TrackAlloc(UpeProfilerMemoryStats *stats, uint32_t category, uint64_t size) {
    if (!stats || category >= 16) {
        return;
    }
    
    stats->total_allocated += size;
    stats->current_usage += size;
    stats->allocation_count++;
    stats->category_usage[category] += size;
}

void UpeMemory_TrackFree(UpeProfilerMemoryStats *stats, uint32_t category, uint64_t size) {
    if (!stats || category >= 16) {
        return;
    }
    
    stats->total_freed += size;
    stats->current_usage -= size;
    stats->free_count++;
    stats->category_usage[category] -= size;
}

void UpeMemory_PrintReport(const UpeProfilerMemoryStats *stats) {
    if (!stats) {
        return;
    }
    
    printf("=== Memory Report ===\n");
    printf("Total Allocated:  %llu bytes\n", (unsigned long long)stats->total_allocated);
    printf("Total Freed:      %llu bytes\n", (unsigned long long)stats->total_freed);
    printf("Current Usage:    %llu bytes (%.2f MB)\n",
        (unsigned long long)stats->current_usage,
        stats->current_usage / (1024.0f * 1024.0f));
    printf("Allocations:      %u\n", stats->allocation_count);
    printf("Frees:            %u\n", stats->free_count);
    printf("\n");
    
    if (stats->category_count > 0) {
        printf("Category Breakdown:\n");
        for (uint32_t i = 0; i < stats->category_count; ++i) {
            const char *name = stats->category_names[i] ? stats->category_names[i] : "Unknown";
            printf("  %-20s: %llu bytes (%.2f MB)\n",
                name,
                (unsigned long long)stats->category_usage[i],
                stats->category_usage[i] / (1024.0f * 1024.0f));
        }
    }
    
    printf("=====================\n");
}
