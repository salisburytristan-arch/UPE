#pragma once

#include <stdint.h>
#include <stddef.h>
#include "upe/upe_handle.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
  MEMORY DIAGNOSTICS & INSTRUMENTATION
  
  Tracks allocations with full context: callsite, thread, lifetime, tags.
  Detects common errors: double-free, use-after-free, fragmentation, leaks.
  Provides frame-by-frame telemetry for memory optimization.
==============================================================================*/

/*==================== ALLOCATION CALLSITE TRACKING ====================*/

typedef struct UpeMemoryCallsite {
    uint32_t hash;                    /* file:line or return address hash */
    const char *filename;
    uint32_t line_number;
    uint64_t total_bytes_allocated;
    uint64_t total_bytes_freed;
    uint32_t allocation_count;        /* Current live allocations from this site */
    uint32_t allocation_peak;         /* Peak concurrent allocations */
} UpeMemoryCallsite;

/* Get stats for a specific callsite */
int UpeMemoryDiag_GetCallsiteStats(uint32_t callsite_hash, UpeMemoryCallsite *out_stats);

/* Get all callsites with allocations (sorted by current bytes) */
void UpeMemoryDiag_GetTopCallsites(UpeMemoryCallsite *out_callsites, uint32_t max_count,
                                    uint32_t *out_count);

/*==================== ALLOCATION CLASS TRACKING ====================*/

typedef enum {
    UPE_LIFETIME_STATIC = 0,          /* Engine startup → shutdown */
    UPE_LIFETIME_WORLD = 1,           /* Level load → level unload */
    UPE_LIFETIME_SESSION = 2,         /* Play session */
    UPE_LIFETIME_FRAME = 3,           /* Single frame (reset each tick) */
    UPE_LIFETIME_TRANSIENT = 4        /* Brief scratch (multiple resets per frame) */
} UpeMemoryLifetime;

typedef struct UpeMemoryClassStats {
    UpeMemoryLifetime lifetime;
    uint64_t total_allocated;
    uint64_t current_bytes;
    uint32_t allocation_count;
    uint32_t deallocation_count;
} UpeMemoryClassStats;

void UpeMemoryDiag_GetLifetimeStats(UpeMemoryLifetime lifetime, UpeMemoryClassStats *out_stats);

/*==================== PER-FRAME TELEMETRY ====================*/

typedef struct UpeMemoryFrameTelemetry {
    uint32_t frame_number;
    uint32_t allocations_this_frame;
    uint32_t deallocations_this_frame;
    uint64_t bytes_allocated_this_frame;
    uint64_t bytes_freed_this_frame;
    uint64_t peak_bytes_active;       /* Peak during frame */
    uint32_t peak_contact_count;
    uint32_t peak_manifold_count;
    uint32_t peak_constraint_count;
    uint32_t peak_particle_count;
    uint32_t peak_broadphase_pairs;
    double frame_time_ms;
} UpeMemoryFrameTelemetry;

/* Record per-frame metrics */
void UpeMemoryDiag_RecordFrameMetrics(double frame_time_ms, uint32_t contact_count,
                                      uint32_t manifold_count, uint32_t constraint_count,
                                      uint32_t particle_count, uint32_t pair_count);

/* Get historical frame data */
void UpeMemoryDiag_GetFrameTelemetry(uint32_t frame_offset_back, UpeMemoryFrameTelemetry *out_telemetry);

/* Get rolling average metrics (last N frames) */
typedef struct {
    double avg_allocations_per_frame;
    double avg_bytes_per_frame;
    double avg_frame_time_ms;
    uint32_t sample_count;
} UpeMemoryRollingStats;

void UpeMemoryDiag_GetRollingStats(uint32_t frame_window, UpeMemoryRollingStats *out_stats);

/*==================== DETECTOR FUNCTIONS ====================*/

/* Scan for potential memory leaks at world unload */
typedef struct {
    uint64_t leaked_bytes;
    uint32_t leaked_allocations;
    uint32_t expected_freed_count;
    uint32_t actually_freed_count;
} UpeMemoryLeakReport;

void UpeMemoryDiag_DetectLeaks(UpeMemoryLeakReport *out_report);

/* Find most recently freed pointer (for use-after-free investigation) */
void* UpeMemoryDiag_GetRecentlyFreed(uint32_t offset_back);

/* Validate all tracked allocations for corruption markers */
int UpeMemoryDiag_ValidateAllAllocations(void);

/* Get fragmentation ratio for a specific allocator */
double UpeMemoryDiag_GetFragmentationRatio(void);

/*==================== GENERATION-BASED HANDLE TRACKING ====================*/

/* Test if a handle is still valid */
int UpeHandle_IsValid(UpeHandle handle);

/* Get generation for next reuse of index */
uint8_t UpeHandle_GetNextGeneration(uint32_t index);

/*==================== CONTENTION TRACKING ====================*/

typedef struct UpeMemoryContention {
    uint32_t thread_id;
    uint32_t allocation_count;
    uint32_t concurrent_allocations;  /* Allocations happening in parallel with this */
    uint64_t contention_bytes;
} UpeMemoryContention;

/* Multi-threaded allocation coordination */
void UpeMemoryDiag_RecordThreadContention(uint32_t thread_id, int allocating);

/*==================== BUDGET ENFORCEMENT ====================*/

/* Graceful degradation when under memory pressure */
typedef enum {
    UPE_LOD_FULL = 0,                 /* Full detail */
    UPE_LOD_REDUCED = 1,              /* Drop non-critical caches */
    UPE_LOD_LOW = 2,                  /* Minimal allocations */
    UPE_LOD_CRITICAL = 3              /* Survive-only mode */
} UpeMemoryLOD;

UpeMemoryLOD UpeMemoryDiag_GetCurrentLOD(void);

/* Per-subsystem budget check */
int UpeMemoryDiag_CheckSubsystemBudget(uint32_t tag, uint64_t requested_bytes);

/*==================== DEBUG GUARDS ====================*/

/* In debug builds, write guardwords around allocations to detect stomp */
#ifdef UPE_DEBUG_MEMORY

#define UPE_GUARD_BYTE_PRE 0xAA
#define UPE_GUARD_BYTE_POST 0x55
#define UPE_GUARD_SIZE 16

typedef struct UpeGuardedAllocation {
    uint8_t pre_guard[UPE_GUARD_SIZE];
    void *user_ptr;
    size_t user_size;
    uint8_t post_guard[UPE_GUARD_SIZE];
} UpeGuardedAllocation;

void* UpeMemoryDiag_AllocGuarded(size_t size);
void UpeMemoryDiag_FreeGuarded(void *ptr);
int UpeMemoryDiag_ValidateGuards(void);

#else

#define UpeMemoryDiag_AllocGuarded(s) malloc(s)
#define UpeMemoryDiag_FreeGuarded(p) free(p)
#define UpeMemoryDiag_ValidateGuards() 1

#endif /* UPE_DEBUG_MEMORY */

/*==================== MEMORY REPORT GENERATION ====================*/

/* Emit detailed JSON or CSV report for analysis */
void UpeMemoryDiag_DumpReportJSON(const char *filename);
void UpeMemoryDiag_DumpReportCSV(const char *filename);

/* Print human-readable memory summary */
void UpeMemoryDiag_PrintSummary(void);

#ifdef __cplusplus
}
#endif
