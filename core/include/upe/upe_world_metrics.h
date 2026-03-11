#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeWorld UpeWorld;

typedef struct UpeWorldStepMetrics {
    uint64_t frame_index;
    uint64_t state_hash;
    uint32_t active_dynamic_bodies;
    uint32_t broadphase_pairs;
    uint32_t contacts;
    uint32_t constraints;
    uint32_t islands;
    uint32_t solver_iterations;
    uint32_t solver_threads;
    uint32_t solver_island_tasks;
    uint32_t solver_parallel_islands;
    uint32_t solver_serial_islands;
    uint32_t ccd_candidate_pairs;
    uint32_t ccd_tested_pairs;
    uint32_t ccd_contact_hits;
} UpeWorldStepMetrics;

int UpeWorld_GetStepMetrics(const UpeWorld *world, UpeWorldStepMetrics *out_metrics);

#ifdef __cplusplus
}
#endif
