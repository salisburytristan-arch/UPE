#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Telemetry event types: recorded during each world step
 */
typedef enum {
    UPE_TELEMETRY_BROADPHASE_PAIRS = 0,
    UPE_TELEMETRY_NARROWPHASE_CONTACTS = 1,
    UPE_TELEMETRY_SOLVER_ITERATIONS = 2,
    UPE_TELEMETRY_SOLVER_CONSTRAINTS = 3,
    UPE_TELEMETRY_BODIES_AWAKE = 4,
    UPE_TELEMETRY_BODIES_SLEEPING = 5,
    UPE_TELEMETRY_PARTICLES_ALIVE = 6,
    UPE_TELEMETRY_CLOTH_ELEMENTS = 7,
    UPE_TELEMETRY_FRAME_TIME_US = 8,
    UPE_TELEMETRY_STEP_TIME_US = 9,
    UPE_TELEMETRY_BROADPHASE_TIME_US = 10,
    UPE_TELEMETRY_NARROWPHASE_TIME_US = 11,
    UPE_TELEMETRY_SOLVE_TIME_US = 12,
    UPE_TELEMETRY_INTEGRATE_TIME_US = 13,
    UPE_TELEMETRY_ISLANDS_FOUND = 14,
    UPE_TELEMETRY_CCD_TESTS = 15,
    UPE_TELEMETRY_NUM_EVENTS = 16,  /**< Must be last */
} UpeTelemetryEvent;

/**
 * Telemetry ring buffer: circular log of recent events
 * Fixed-size, low-overhead always-on measurement
 * Thread-safe for recording and reading
 */
typedef struct {
    uint64_t frame_number;     /**< Which world step (0-based) */
    uint32_t event_id;         /**< UpeTelemetryEvent value */
    uint64_t value;            /**< Event-specific measurement (count, time, etc) */
    uint32_t thread_id;        /**< Which thread recorded this */
} UpeTelemetryEntry;

/**
 * Telemetry statistics: per-event aggregates
 */
typedef struct {
    uint32_t event_id;
    uint64_t count;            /**< Number of times recorded */
    uint64_t min_value;        /**< Minimum recorded value */
    uint64_t max_value;        /**< Maximum recorded value */
    uint64_t sum_value;        /**< Sum of all values (for averaging) */
    uint64_t last_value;       /**< Most recent value */
} UpeTelemetryStats;

/**
 * Telemetry ring buffer handle
 */
typedef struct UpeTelemetryBuffer UpeTelemetryBuffer;

/**
 * Create a telemetry ring buffer with capacity for N entries
 * capacity must be power of 2
 * Returns NULL if allocation fails
 */
UpeTelemetryBuffer *UpeTelemetry_Create(uint32_t capacity);

/**
 * Destroy telemetry buffer and free memory
 */
void UpeTelemetry_Destroy(UpeTelemetryBuffer *buffer);

/**
 * Record a telemetry event (thread-safe, lock-free)
 * Called by engine internals on every frame worth recording
 */
void UpeTelemetry_Record(UpeTelemetryBuffer *buffer, uint64_t frame_number,
                        UpeTelemetryEvent event_id, uint64_t value);

/**
 * Get aggregate statistics for an event type (over all recorded frames)
 * Returns zero-initialized struct if event not yet recorded
 */
UpeTelemetryStats UpeTelemetry_GetStats(const UpeTelemetryBuffer *buffer, UpeTelemetryEvent event_id);

/**
 * Dump all entries in the buffer as JSON for analysis
 * Caller must free returned string
 * Returns NULL if allocation fails
 */
char *UpeTelemetry_DumpJson(const UpeTelemetryBuffer *buffer);

/**
 * Clear all entries in the buffer
 */
void UpeTelemetry_Clear(UpeTelemetryBuffer *buffer);

/**
 * Get current entry count in buffer
 */
uint32_t UpeTelemetry_GetEntryCount(const UpeTelemetryBuffer *buffer);

/**
 * Get event name string
 * Returns pointer to static string, do not free
 */
const char *UpeTelemetry_EventName(UpeTelemetryEvent event_id);

/**
 * Get unit string for an event (e.g., "us" for time, "count" for measurements)
 */
const char *UpeTelemetry_EventUnit(UpeTelemetryEvent event_id);

#ifdef __cplusplus
}
#endif
