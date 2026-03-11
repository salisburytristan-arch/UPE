#include "upe/upe_telemetry.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>

/**
 * Event name and unit lookup tables
 */
static const char *kEventNames[UPE_TELEMETRY_NUM_EVENTS] = {
    "broadphase_pairs",
    "narrowphase_contacts",
    "solver_iterations",
    "solver_constraints",
    "bodies_awake",
    "bodies_sleeping",
    "particles_alive",
    "cloth_elements",
    "frame_time",
    "step_time",
    "broadphase_time",
    "narrowphase_time",
    "solve_time",
    "integrate_time",
    "islands_found",
    "ccd_tests",
};

static const char *kEventUnits[UPE_TELEMETRY_NUM_EVENTS] = {
    "count", "count", "count", "count",  /* counts */
    "count", "count", "count", "count",  /* counts */
    "us", "us", "us", "us", "us", "us",  /* times */
    "count", "count",                     /* counts */
};

/**
 * Ring buffer implementation
 */
typedef struct UpeTelemetryBuffer {
    UpeTelemetryEntry *entries;
    uint32_t capacity;
    uint32_t capacity_mask;     /* capacity - 1 for fast modulo */
    std::atomic<uint64_t> write_index;
    std::atomic<uint64_t> stats[UPE_TELEMETRY_NUM_EVENTS * 4];  /* min, max, sum, count per event */
} UpeTelemetryBufferImpl;

static void *UpeTelemetry_Alloc(size_t size) {
    return std::malloc(size);
}

static void UpeTelemetry_Free(void *ptr) {
    std::free(ptr);
}

UpeTelemetryBuffer *UpeTelemetry_Create(uint32_t capacity) {
    /* Validate power of 2 */
    if (capacity == 0 || (capacity & (capacity - 1)) != 0) {
        return NULL;
    }

    UpeTelemetryBufferImpl *buffer = (UpeTelemetryBufferImpl *)UpeTelemetry_Alloc(sizeof(UpeTelemetryBufferImpl));
    if (buffer == NULL) {
        return NULL;
    }

    buffer->entries = (UpeTelemetryEntry *)UpeTelemetry_Alloc(capacity * sizeof(UpeTelemetryEntry));
    if (buffer->entries == NULL) {
        UpeTelemetry_Free(buffer);
        return NULL;
    }

    buffer->capacity = capacity;
    buffer->capacity_mask = capacity - 1;
    buffer->write_index.store(0, std::memory_order_relaxed);
    for (uint32_t i = 0; i < UPE_TELEMETRY_NUM_EVENTS * 4u; ++i) {
        buffer->stats[i].store(0, std::memory_order_relaxed);
    }
    std::memset(buffer->entries, 0, capacity * sizeof(UpeTelemetryEntry));

    return (UpeTelemetryBuffer *)buffer;
}

void UpeTelemetry_Destroy(UpeTelemetryBuffer *buffer) {
    if (buffer == NULL) {
        return;
    }
    UpeTelemetryBufferImpl *impl = (UpeTelemetryBufferImpl *)buffer;
    UpeTelemetry_Free(impl->entries);
    UpeTelemetry_Free(impl);
}

void UpeTelemetry_Record(UpeTelemetryBuffer *buffer, uint64_t frame_number,
                        UpeTelemetryEvent event_id, uint64_t value) {
    if (buffer == NULL || event_id >= UPE_TELEMETRY_NUM_EVENTS) {
        return;
    }

    UpeTelemetryBufferImpl *impl = (UpeTelemetryBufferImpl *)buffer;

    /* Get current write position and advance */
    uint64_t idx = impl->write_index.fetch_add(1, std::memory_order_relaxed);
    uint32_t pos = idx & impl->capacity_mask;

    /* Write entry */
    impl->entries[pos].frame_number = frame_number;
    impl->entries[pos].event_id = event_id;
    impl->entries[pos].value = value;
    impl->entries[pos].thread_id = 0;  /* TODO: get thread ID */

    /* Update statistics (lock-free approximate aggregates) */
    uint32_t stat_base = event_id * 4;
    
    /* This is approximate lock-free; exact statistics require synchronization */
    impl->stats[stat_base + 2].fetch_add(value, std::memory_order_relaxed);  /* sum */
    impl->stats[stat_base + 3].fetch_add(1, std::memory_order_relaxed);       /* count */
    
    /* Min/max are approximate due to TOCTOU */
    uint64_t old_min = impl->stats[stat_base].load(std::memory_order_relaxed);
    if (old_min == 0 || value < old_min) {
        impl->stats[stat_base].store(value, std::memory_order_relaxed);
    }
    uint64_t old_max = impl->stats[stat_base + 1].load(std::memory_order_relaxed);
    if (value > old_max) {
        impl->stats[stat_base + 1].store(value, std::memory_order_relaxed);
    }
}

UpeTelemetryStats UpeTelemetry_GetStats(const UpeTelemetryBuffer *buffer, UpeTelemetryEvent event_id) {
    UpeTelemetryStats stats = {};
    if (buffer == NULL || event_id >= UPE_TELEMETRY_NUM_EVENTS) {
        return stats;
    }

    const UpeTelemetryBufferImpl *impl = (const UpeTelemetryBufferImpl *)buffer;
    uint32_t stat_base = event_id * 4;

    stats.event_id = event_id;
    stats.min_value = impl->stats[stat_base].load(std::memory_order_relaxed);
    stats.max_value = impl->stats[stat_base + 1].load(std::memory_order_relaxed);
    stats.sum_value = impl->stats[stat_base + 2].load(std::memory_order_relaxed);
    stats.count = impl->stats[stat_base + 3].load(std::memory_order_relaxed);
    if (stats.count > 0) {
        uint64_t last_idx = impl->write_index.load(std::memory_order_relaxed);
        if (last_idx > 0) {
            uint32_t pos = (uint32_t)((last_idx - 1u) & impl->capacity_mask);
            stats.last_value = impl->entries[pos].value;
        }
    }

    return stats;
}

char *UpeTelemetry_DumpJson(const UpeTelemetryBuffer *buffer) {
    if (buffer == NULL) {
        return NULL;
    }

    const UpeTelemetryBufferImpl *impl = (const UpeTelemetryBufferImpl *)buffer;
    uint64_t entry_count = impl->write_index.load(std::memory_order_relaxed);
    uint32_t actual_count = (entry_count > impl->capacity) ? impl->capacity : (uint32_t)entry_count;

    /* Estimate JSON size and allocate */
    size_t json_size = 512 + actual_count * 128;
    char *json = (char *)UpeTelemetry_Alloc(json_size);
    if (json == NULL) {
        return NULL;
    }

    /* Build JSON manually using write cursor (avoids repeated strlen scans) */
    size_t pos = 0;
    int written = snprintf(json,
                           json_size,
                           "{\"telemetry\":{\"capacity\":%u,\"entries\":%u,\"data\":[",
                           impl->capacity,
                           actual_count);
    if (written < 0) {
        json[0] = '\0';
        return json;
    }
    pos = ((size_t)written < json_size) ? (size_t)written : (json_size - 1u);

    for (uint32_t i = 0; i < actual_count; i++) {
        uint32_t entry_pos = i & impl->capacity_mask;
        const UpeTelemetryEntry *entry = &impl->entries[entry_pos];

        if (pos >= json_size - 1u) {
            break;
        }

        size_t remaining = json_size - pos;
        int chunk = snprintf(json + pos,
                             remaining,
                             "%s{\"frame\":%llu,\"event\":\"%s\",\"value\":%llu}",
                             (i > 0) ? "," : "",
                             (unsigned long long)entry->frame_number,
                             kEventNames[entry->event_id],
                             (unsigned long long)entry->value);
        if (chunk < 0) {
            break;
        }
        if ((size_t)chunk >= remaining) {
            pos = json_size - 1u;
            break;
        }
        pos += (size_t)chunk;
    }

    if (pos < json_size) {
        snprintf(json + pos, json_size - pos, "]}");
    } else {
        json[json_size - 1u] = '\0';
    }
    return json;
}

void UpeTelemetry_Clear(UpeTelemetryBuffer *buffer) {
    if (buffer == NULL) {
        return;
    }
    UpeTelemetryBufferImpl *impl = (UpeTelemetryBufferImpl *)buffer;
    impl->write_index.store(0, std::memory_order_relaxed);
    for (uint32_t i = 0; i < UPE_TELEMETRY_NUM_EVENTS * 4u; ++i) {
        impl->stats[i].store(0, std::memory_order_relaxed);
    }
    std::memset(impl->entries, 0, impl->capacity * sizeof(UpeTelemetryEntry));
}

uint32_t UpeTelemetry_GetEntryCount(const UpeTelemetryBuffer *buffer) {
    if (buffer == NULL) {
        return 0;
    }
    const UpeTelemetryBufferImpl *impl = (const UpeTelemetryBufferImpl *)buffer;
    uint64_t idx = impl->write_index.load(std::memory_order_relaxed);
    return (idx > impl->capacity) ? impl->capacity : (uint32_t)idx;
}

const char *UpeTelemetry_EventName(UpeTelemetryEvent event_id) {
    if (event_id >= UPE_TELEMETRY_NUM_EVENTS) {
        return "unknown";
    }
    return kEventNames[event_id];
}

const char *UpeTelemetry_EventUnit(UpeTelemetryEvent event_id) {
    if (event_id >= UPE_TELEMETRY_NUM_EVENTS) {
        return "unknown";
    }
    return kEventUnits[event_id];
}
