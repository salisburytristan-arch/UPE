#include "upe/upe_memory_diag.h"
#include "upe/upe_memory_policy.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <thread>
#include <mutex>
#include <functional>

/*==============================================================================
  DIAGNOSTICS STATE
==============================================================================*/

#define UPE_MAX_CALLSITES 512
#define UPE_MAX_FRAME_HISTORY 256

typedef struct {
    UpeMemoryCallsite callsites[UPE_MAX_CALLSITES];
    uint32_t callsite_count;
    
    UpeMemoryClassStats lifetime_stats[5];
    
    UpeMemoryFrameTelemetry frame_history[UPE_MAX_FRAME_HISTORY];
    uint32_t frame_history_head;
    uint32_t frame_history_count;
    
    UpeMemoryLOD current_lod;
} UpeGlobalDiastics;

static UpeGlobalDiastics g_diag = {};

#define UPE_RECENT_FREED_CAPACITY 1024
#define UPE_CONTENTION_SLOTS 64

static void *g_recent_freed[UPE_RECENT_FREED_CAPACITY] = {};
static uint32_t g_recent_freed_head = 0;
static uint32_t g_recent_freed_count = 0;

typedef struct {
    uint32_t thread_id;
    uint32_t allocation_count;
    uint32_t concurrent_allocations;
    uint64_t contention_bytes;
} UpeContentionSlot;

static UpeContentionSlot g_contention[UPE_CONTENTION_SLOTS] = {};
static uint32_t g_active_allocator_threads = 0;

#ifdef UPE_DEBUG_MEMORY
typedef struct {
    void *user_ptr;
    size_t user_size;
} UpeGuardTracking;

static UpeGuardTracking *g_guarded_allocs = NULL;
static uint32_t g_guarded_count = 0;
static uint32_t g_guarded_capacity = 0;
static std::mutex g_guard_mutex;

static void UpeMemoryDiag_GuardTrackAdd(void *user_ptr, size_t user_size) {
    std::lock_guard<std::mutex> lock(g_guard_mutex);
    if (g_guarded_count >= g_guarded_capacity) {
        uint32_t next_capacity = g_guarded_capacity ? g_guarded_capacity * 2u : 128u;
        UpeGuardTracking *next = (UpeGuardTracking*)realloc(g_guarded_allocs, next_capacity * sizeof(UpeGuardTracking));
        if (!next) {
            return;
        }
        g_guarded_allocs = next;
        g_guarded_capacity = next_capacity;
    }
    g_guarded_allocs[g_guarded_count].user_ptr = user_ptr;
    g_guarded_allocs[g_guarded_count].user_size = user_size;
    g_guarded_count++;
}

static void UpeMemoryDiag_GuardTrackRemove(void *user_ptr) {
    std::lock_guard<std::mutex> lock(g_guard_mutex);
    for (uint32_t i = 0; i < g_guarded_count; ++i) {
        if (g_guarded_allocs[i].user_ptr == user_ptr) {
            g_guarded_allocs[i] = g_guarded_allocs[g_guarded_count - 1];
            g_guarded_count--;
            return;
        }
    }
}
#endif

extern "C" void UpeMemoryDiag_RecordFreedPointer(void *ptr) {
    if (!ptr) {
        return;
    }
    g_recent_freed[g_recent_freed_head] = ptr;
    g_recent_freed_head = (g_recent_freed_head + 1u) % UPE_RECENT_FREED_CAPACITY;
    if (g_recent_freed_count < UPE_RECENT_FREED_CAPACITY) {
        g_recent_freed_count++;
    }
}

/*==============================================================================
  CALLSITE TRACKING
==============================================================================*/

[[maybe_unused]] static int _find_or_create_callsite(uint32_t hash, uint32_t *out_index) {
    /* Find existing */
    for (uint32_t i = 0; i < g_diag.callsite_count; i++) {
        if (g_diag.callsites[i].hash == hash) {
            *out_index = i;
            return 1;
        }
    }
    
    /* Create new */
    if (g_diag.callsite_count >= UPE_MAX_CALLSITES) {
        return 0;
    }
    
    *out_index = g_diag.callsite_count;
    memset(&g_diag.callsites[*out_index], 0, sizeof(UpeMemoryCallsite));
    g_diag.callsites[*out_index].hash = hash;
    g_diag.callsite_count++;
    return 1;
}

int UpeMemoryDiag_GetCallsiteStats(uint32_t callsite_hash, UpeMemoryCallsite *out_stats) {
    if (!out_stats) return 0;
    
    for (uint32_t i = 0; i < g_diag.callsite_count; i++) {
        if (g_diag.callsites[i].hash == callsite_hash) {
            *out_stats = g_diag.callsites[i];
            return 1;
        }
    }
    return 0;
}

void UpeMemoryDiag_GetTopCallsites(UpeMemoryCallsite *out_callsites, uint32_t max_count,
                                    uint32_t *out_count) {
    if (!out_callsites || !out_count) return;
    
    /* Sort callsites by current bytes (bubble sort for simplicity) */
    uint32_t count = g_diag.callsite_count < max_count ? g_diag.callsite_count : max_count;
    
    memcpy(out_callsites, g_diag.callsites, count * sizeof(UpeMemoryCallsite));
    
    /* Simple bubble sort */
    for (uint32_t i = 0; i < count; i++) {
        for (uint32_t j = i + 1; j < count; j++) {
            if (out_callsites[j].total_bytes_allocated > out_callsites[i].total_bytes_allocated) {
                UpeMemoryCallsite tmp = out_callsites[i];
                out_callsites[i] = out_callsites[j];
                out_callsites[j] = tmp;
            }
        }
    }
    
    *out_count = count;
}

/*==============================================================================
  LIFETIME CLASS TRACKING
==============================================================================*/

void UpeMemoryDiag_GetLifetimeStats(UpeMemoryLifetime lifetime, UpeMemoryClassStats *out_stats) {
    if (!out_stats || lifetime >= 5) return;
    *out_stats = g_diag.lifetime_stats[lifetime];
}

/*==============================================================================
  FRAME TELEMETRY
==============================================================================*/

void UpeMemoryDiag_RecordFrameMetrics(double frame_time_ms, uint32_t contact_count,
                                      uint32_t manifold_count, uint32_t constraint_count,
                                      uint32_t particle_count, uint32_t pair_count) {
    UpeMemoryFrameTelemetry *slot = &g_diag.frame_history[g_diag.frame_history_head];
    
    slot->frame_number++;
    slot->frame_time_ms = frame_time_ms;
    slot->peak_contact_count = contact_count;
    slot->peak_manifold_count = manifold_count;
    slot->peak_constraint_count = constraint_count;
    slot->peak_particle_count = particle_count;
    slot->peak_broadphase_pairs = pair_count;
    
    g_diag.frame_history_head = (g_diag.frame_history_head + 1) % UPE_MAX_FRAME_HISTORY;
    if (g_diag.frame_history_count < UPE_MAX_FRAME_HISTORY) {
        g_diag.frame_history_count++;
    }
}

void UpeMemoryDiag_GetFrameTelemetry(uint32_t frame_offset_back, UpeMemoryFrameTelemetry *out_telemetry) {
    if (!out_telemetry || frame_offset_back >= g_diag.frame_history_count) {
        memset(out_telemetry, 0, sizeof(UpeMemoryFrameTelemetry));
        return;
    }
    
    uint32_t idx = (g_diag.frame_history_head - 1 - frame_offset_back + UPE_MAX_FRAME_HISTORY) % UPE_MAX_FRAME_HISTORY;
    *out_telemetry = g_diag.frame_history[idx];
}

void UpeMemoryDiag_GetRollingStats(uint32_t frame_window, UpeMemoryRollingStats *out_stats) {
    if (!out_stats) return;
    
    memset(out_stats, 0, sizeof(UpeMemoryRollingStats));
    
    uint32_t count = frame_window < g_diag.frame_history_count ? frame_window : g_diag.frame_history_count;
    if (count == 0) return;
    
    uint64_t total_allocations = 0;
    uint64_t total_bytes = 0;
    double total_time = 0.0;
    
    for (uint32_t i = 0; i < count; i++) {
        UpeMemoryFrameTelemetry telemetry;
        UpeMemoryDiag_GetFrameTelemetry(i, &telemetry);
        
        total_allocations += telemetry.allocations_this_frame;
        total_bytes += telemetry.bytes_allocated_this_frame;
        total_time += telemetry.frame_time_ms;
    }
    
    out_stats->avg_allocations_per_frame = (double)total_allocations / count;
    out_stats->avg_bytes_per_frame = (double)total_bytes / count;
    out_stats->avg_frame_time_ms = total_time / count;
    out_stats->sample_count = count;
}

/*==============================================================================
  LEAK DETECTION
==============================================================================*/

void UpeMemoryDiag_DetectLeaks(UpeMemoryLeakReport *out_report) {
    if (!out_report) return;
    
    memset(out_report, 0, sizeof(UpeMemoryLeakReport));
    
    UpeMemoryStats stats;
    UpeMemory_GetStats(&stats);
    
    /* Any bytes still allocated = potential leak */
    out_report->leaked_bytes = stats.total_allocated;
    out_report->leaked_allocations = stats.allocation_count;
}

void* UpeMemoryDiag_GetRecentlyFreed(uint32_t offset_back) {
    if (offset_back >= g_recent_freed_count) {
        return NULL;
    }
    uint32_t newest = (g_recent_freed_head + UPE_RECENT_FREED_CAPACITY - 1u) % UPE_RECENT_FREED_CAPACITY;
    uint32_t idx = (newest + UPE_RECENT_FREED_CAPACITY - (offset_back % UPE_RECENT_FREED_CAPACITY)) % UPE_RECENT_FREED_CAPACITY;
    return g_recent_freed[idx];
}

int UpeMemoryDiag_ValidateAllAllocations(void) {
    if (!UpeMemory_ValidateCanaries()) {
        return 0;
    }
#ifdef UPE_DEBUG_MEMORY
    if (!UpeMemoryDiag_ValidateGuards()) {
        return 0;
    }
#endif
    return UpeMemory_CheckBudgets();
}

double UpeMemoryDiag_GetFragmentationRatio(void) {
    UpeMemoryStats stats;
    UpeMemory_GetStats(&stats);

    if (stats.peak_allocated == 0) {
        return 0.0;
    }

    double live_ratio = (double)stats.total_allocated / (double)stats.peak_allocated;
    double frag = 1.0 - live_ratio;
    if (frag < 0.0) frag = 0.0;
    if (frag > 1.0) frag = 1.0;
    return frag;
}

/*==============================================================================
  HANDLE MANAGEMENT
==============================================================================*/

int UpeHandle_IsValid(UpeHandle handle) {
    /* Generation 0 is invalid (always), rest are valid if within pool */
    return handle.generation != 0;
}

uint8_t UpeHandle_GetNextGeneration(uint32_t index) {
    /* Simple increment; wrap at 255 back to 1 (preserve 0 as invalid) */
    uint8_t next = (uint8_t)((index + 1u) & 0xFFu);
    if (next == 0) {
        next = 1;
    }
    return next;
}

/*==============================================================================
  THREAD CONTENTION
==============================================================================*/

void UpeMemoryDiag_RecordThreadContention(uint32_t thread_id, int allocating) {
    if (thread_id == 0u) {
        thread_id = (uint32_t)std::hash<std::thread::id>{}(std::this_thread::get_id());
    }

    uint32_t slot = thread_id % UPE_CONTENTION_SLOTS;
    if (g_contention[slot].thread_id != thread_id) {
        g_contention[slot].thread_id = thread_id;
        g_contention[slot].allocation_count = 0;
        g_contention[slot].concurrent_allocations = 0;
        g_contention[slot].contention_bytes = 0;
    }

    if (allocating) {
        g_contention[slot].allocation_count++;
        if (g_active_allocator_threads > 0u) {
            g_contention[slot].concurrent_allocations += g_active_allocator_threads;
            g_contention[slot].contention_bytes += 64u;
        }
        g_active_allocator_threads++;
    } else {
        if (g_active_allocator_threads > 0u) {
            g_active_allocator_threads--;
        }
    }
}

/*==============================================================================
  LOD MANAGEMENT
====================================================================================*/

UpeMemoryLOD UpeMemoryDiag_GetCurrentLOD(void) {
    UpeMemoryPressure pressure = UpeMemory_GetPressure();
    
    switch (pressure) {
        case UPE_MEMORY_OK:
            return UPE_LOD_FULL;
        case UPE_MEMORY_LOW:
            return UPE_LOD_REDUCED;
        case UPE_MEMORY_CRITICAL:
            return UPE_LOD_LOW;
        default:
            return UPE_LOD_CRITICAL;
    }
}

int UpeMemoryDiag_CheckSubsystemBudget(uint32_t tag, uint64_t requested_bytes) {
    UpeMemoryStats stats;
    UpeMemory_GetStats(&stats);
    
    if (stats.bytes_by_tag[tag] + requested_bytes > 1000000000) {  /* 1GB default */
        return 0;  /* Budget exceeded */
    }
    return 1;
}

/*==============================================================================
  DEBUG GUARDS (for debug builds)
==============================================================================*/

#ifdef UPE_DEBUG_MEMORY

void* UpeMemoryDiag_AllocGuarded(size_t size) {
    /* Allocate extra for guards */
    void *raw = malloc(size + 2 * UPE_GUARD_SIZE);
    if (!raw) return NULL;
    
    uint8_t *bytes = (uint8_t*)raw;
    memset(bytes, UPE_GUARD_BYTE_PRE, UPE_GUARD_SIZE);
    memset(bytes + size + UPE_GUARD_SIZE, UPE_GUARD_BYTE_POST, UPE_GUARD_SIZE);
    
    void *user_ptr = bytes + UPE_GUARD_SIZE;
    UpeMemoryDiag_GuardTrackAdd(user_ptr, size);
    return user_ptr;
}

void UpeMemoryDiag_FreeGuarded(void *ptr) {
    if (!ptr) return;
    
    UpeMemoryDiag_GuardTrackRemove(ptr);
    uint8_t *bytes = (uint8_t*)ptr;
    free(bytes - UPE_GUARD_SIZE);
}

int UpeMemoryDiag_ValidateGuards(void) {
    std::lock_guard<std::mutex> lock(g_guard_mutex);
    for (uint32_t i = 0; i < g_guarded_count; ++i) {
        uint8_t *user = (uint8_t*)g_guarded_allocs[i].user_ptr;
        size_t size = g_guarded_allocs[i].user_size;
        uint8_t *pre = user - UPE_GUARD_SIZE;
        uint8_t *post = user + size;

        for (uint32_t j = 0; j < UPE_GUARD_SIZE; ++j) {
            if (pre[j] != UPE_GUARD_BYTE_PRE) {
                return 0;
            }
            if (post[j] != UPE_GUARD_BYTE_POST) {
                return 0;
            }
        }
    }
    return 1;
}

#endif /* UPE_DEBUG_MEMORY */

/*==============================================================================
  REPORTING
====================================================================================*/

void UpeMemoryDiag_DumpReportJSON(const char *filename) {
    if (!filename) return;
    
    FILE *file = fopen(filename, "w");
    if (!file) return;
    
    fprintf(file, "{\n");
    fprintf(file, "  \"memory_report\": {\n");
    
    UpeMemoryStats stats;
    UpeMemory_GetStats(&stats);
    
    fprintf(file, "    \"total_allocated\": %" PRIu64 ",\n", stats.total_allocated);
    fprintf(file, "    \"peak_allocated\": %" PRIu64 ",\n", stats.peak_allocated);
    fprintf(file, "    \"allocation_count\": %" PRIu64 ",\n", stats.allocation_count);
    fprintf(file, "    \"fragmentation\": %.2f\n", UpeMemoryDiag_GetFragmentationRatio());
    
    fprintf(file, "  }\n");
    fprintf(file, "}\n");
    
    fclose(file);
}

void UpeMemoryDiag_DumpReportCSV(const char *filename) {
    if (!filename) return;
    
    FILE *file = fopen(filename, "w");
    if (!file) return;
    
    fprintf(file, "Frame,Allocations,Bytes,PeakBytes,Time(ms)\n");
    
    for (uint32_t i = 0; i < g_diag.frame_history_count; i++) {
        UpeMemoryFrameTelemetry telemetry;
        UpeMemoryDiag_GetFrameTelemetry(i, &telemetry);
        
        fprintf(file, "%u,%u,%" PRIu64 ",%" PRIu64 ",%.2f\n",
                telemetry.frame_number,
                telemetry.allocations_this_frame,
            telemetry.bytes_allocated_this_frame,
            telemetry.peak_bytes_active,
                telemetry.frame_time_ms);
    }
    
    fclose(file);
}

void UpeMemoryDiag_PrintSummary(void) {
    UpeMemoryStats stats;
    UpeMemory_GetStats(&stats);
    
    printf("\n=== MEMORY DIAGNOSTICS ===\n");
    printf("Current: %.1f MB / Peak: %.1f MB\n",
           stats.total_allocated / 1024.0 / 1024.0,
           stats.peak_allocated / 1024.0 / 1024.0);
    printf("Allocations: %" PRIu64 " live\n", stats.allocation_count);
    printf("Fragmentation: %.1f%%\n", UpeMemoryDiag_GetFragmentationRatio() * 100.0);
    printf("LOD Mode: %d\n", UpeMemoryDiag_GetCurrentLOD());
    
    printf("\nTop allocators by bytes:\n");
    UpeMemoryCallsite top_callsites[10];
    uint32_t count;
    UpeMemoryDiag_GetTopCallsites(top_callsites, 10, &count);
    
    for (uint32_t i = 0; i < count; i++) {
        printf("  [%u] %" PRIu64 " bytes in %u allocations\n",
               i, top_callsites[i].total_bytes_allocated, top_callsites[i].allocation_count);
    }
    printf("========================\n\n");
}
