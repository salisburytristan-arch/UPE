#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeWorld UpeWorld;

typedef enum UpeValidationFlags {
    UPE_VALIDATION_PASS = 0u,
    UPE_VALIDATION_SPEED_BUDGET = (1u << 0),
    UPE_VALIDATION_ENERGY_BUDGET = (1u << 1),
    UPE_VALIDATION_PENETRATION_BUDGET = (1u << 2),
    UPE_VALIDATION_TIMESTEP_BUDGET = (1u << 3),
    UPE_VALIDATION_UNCERTAINTY_BUDGET = (1u << 4),
    UPE_VALIDATION_NONFINITE = (1u << 5),
    UPE_VALIDATION_PLAUSIBILITY_LOW = (1u << 6)
} UpeValidationFlags;

typedef struct UpeWorldValidationMetrics {
    uint32_t validation_flags;
    float plausibility_score;
    float conservation_score;
    float contact_quality_score;
    float timestep_quality_score;
    float uncertainty_ratio;
    float speed_budget_ratio;
    float energy_budget_ratio;
    float penetration_budget_ratio;
} UpeWorldValidationMetrics;

typedef struct UpeWorldStepMetrics {
    uint64_t frame_index;
    uint64_t state_hash;
    uint32_t active_dynamic_bodies;
    uint32_t broadphase_pairs;
    uint32_t contacts;
    uint32_t constraints;
    uint32_t islands;
    uint32_t solver_iterations;
    uint32_t configured_solver_iterations;
    uint32_t solver_threads;
    uint32_t solver_island_tasks;
    uint32_t solver_parallel_islands;
    uint32_t solver_serial_islands;
    uint32_t ccd_candidate_pairs;
    uint32_t ccd_tested_pairs;
    uint32_t ccd_contact_hits;
    uint32_t adaptive_runtime_enabled;
    uint32_t integrator_mode;
    uint32_t stability_flags;
    uint32_t validation_flags;
    float peak_linear_speed;
    float total_kinetic_energy_si;
    float total_potential_energy_si;
    float total_energy_si;
    float energy_drift_ratio;
    float total_linear_momentum_si;
    float max_contact_penetration_si;
    float courant_number;
    float froude_number;
    float reynolds_estimate;
    float uncertainty_margin_si;
    float plausibility_score;
    float conservation_score;
    float contact_quality_score;
    float timestep_quality_score;
    float uncertainty_ratio;
    float unit_length_scale;
    float unit_mass_scale;
    float unit_time_scale;
} UpeWorldStepMetrics;

int UpeWorld_GetStepMetrics(const UpeWorld *world, UpeWorldStepMetrics *out_metrics);
int UpeWorld_GetValidationMetrics(const UpeWorld *world, UpeWorldValidationMetrics *out_metrics);
uint32_t UpeWorld_FormatValidationSummary(const UpeWorldValidationMetrics *metrics,
                                          char *buffer,
                                          uint32_t buffer_size);

#ifdef __cplusplus
}
#endif
