#pragma once

#include "upe/upe_api.h"
#include "upe/upe_math.h"
#include "upe/upe_contact.h"
#include "upe/upe_geometry.h"
#include "upe/upe_memory_context.h"

struct UpeBody;
struct UpeShape;
struct UpeReplayRecorder;
struct UpeBvhNode;
struct UpeProfiler;

typedef struct UpeGravityFieldState {
    uint32_t id;
    UpeVec3 position;
    float strength;
    float min_distance;
    float max_distance;
    float falloff_exponent;
    int enabled;
    int type;
} UpeGravityFieldState;

// Contact pair: stores body indices and contact manifold for collision pair
typedef struct UpeContactPair {
    uint32_t body_a;
    uint32_t body_b;
    struct UpeShape *shape_a;
    struct UpeShape *shape_b;
    UpeContactManifold manifold;
} UpeContactPair;

struct UpeWorld {
    float time;
    float fixed_dt;
    UpeVec3 gravity;
    UpeMemoryContext *memory;
    struct UpeBody *bodies;
    uint32_t body_count;
    uint32_t body_capacity;
    uint16_t *body_generations;
    uint8_t *body_alive;
    uint32_t *alive_body_ids;
    uint32_t alive_body_count;
    uint32_t alive_body_capacity;
    uint32_t *dynamic_body_ids;
    uint32_t dynamic_body_count;
    uint32_t dynamic_body_capacity;
    uint32_t *awake_dynamic_ids;
    uint32_t awake_dynamic_count;
    uint32_t awake_dynamic_capacity;
    UpeGravityFieldState *gravity_fields;
    uint32_t gravity_field_count;
    uint32_t gravity_field_capacity;
    uint32_t gravity_field_next_id;

    struct UpeShape *shapes;
    uint32_t shape_count;
    uint32_t shape_capacity;
    uint16_t *shape_generations;
    uint8_t *shape_alive;

    // Pipeline storage
    UpeContactPair *contacts;
    uint32_t contact_count;
    uint32_t contact_capacity;
    UpeAabb3 *scratch_bounds;
    uint32_t scratch_bounds_capacity;
    uint32_t *scratch_body_indices;
    uint32_t scratch_body_indices_capacity;
    void *scratch_pairs;
    uint32_t scratch_pairs_capacity;
    UpeContactPair *scratch_contacts;
    uint32_t scratch_contacts_capacity;
    UpeAabb3 *scratch_ccd_bounds;
    uint32_t scratch_ccd_capacity;
    uint32_t *scratch_ccd_body_ids;
    uint32_t *scratch_ccd_primary_shape_ids;
    uint8_t *scratch_ccd_fast_flags;
    void *scratch_solver_bodies;
    uint32_t scratch_solver_bodies_capacity;
    void *scratch_island_tasks;
    uint32_t scratch_island_tasks_capacity;
    void *scratch_constraints;
    uint32_t scratch_constraints_capacity;
    uint32_t *scratch_constraint_island_ids;
    uint32_t scratch_constraint_island_ids_capacity;
    uint32_t *scratch_constraint_cursor;
    uint32_t scratch_constraint_cursor_capacity;
    void *scratch_reordered_constraints;
    uint32_t scratch_reordered_constraints_capacity;
    int32_t *scratch_bucket_heads;
    uint32_t scratch_bucket_heads_capacity;
    uint32_t *scratch_bucket_next;
    uint32_t scratch_bucket_next_capacity;
    uint32_t *scratch_root_to_island;
    uint32_t scratch_root_to_island_capacity;
    void *scratch_thread_work_items;
    uint32_t scratch_thread_work_items_capacity;
    uint32_t *scratch_parallel_task_indices;
    uint32_t scratch_parallel_task_indices_capacity;
    void *scratch_island_batch_tasks;
    uint32_t scratch_island_batch_tasks_capacity;
    
    // Warm-starting: previous frame solver constraints for lambda persistence
    struct UpeSolverConstraint *prev_constraints;
    uint32_t prev_constraint_count;
    uint32_t prev_constraint_capacity;
    
    // Joint constraints
    struct UpeJoint *joints;
    uint32_t joint_count;
    uint32_t joint_capacity;
    
    // Island sleeping
    struct UpeIsland *islands;
    uint32_t island_count;
    uint32_t island_capacity;
    uint32_t *body_island_ids;
    uint32_t *island_constraint_offsets;
    uint32_t *island_constraint_counts;
    
    // Energy tracking (debug/diagnostics)
    double total_kinetic_energy;
    double total_potential_energy;
    double energy_dissipated_this_frame;
    double last_total_energy;
    float last_energy_drift_ratio;
    float last_total_linear_momentum;
    float last_max_contact_penetration;
    float last_courant_number;
    float last_froude_number;
    float last_reynolds_estimate;
    float last_uncertainty_margin_si;
    uint32_t last_stability_flags;

    // Query acceleration (BVH)
    UpeAabb3 *query_bounds;
    uint32_t query_bound_count;
    uint32_t query_bound_capacity;
    uint32_t *query_indices;
    uint32_t query_index_capacity;
    struct UpeBvhNode *query_nodes;
    uint32_t query_node_count;
    uint32_t query_node_capacity;
    int query_bvh_dirty;

    struct UpeProfiler *profiler;

    uint32_t solver_iterations;
    uint32_t base_solver_iterations;
    uint32_t solver_friction_iterations;
    uint32_t solver_thread_count;
    uint32_t max_pairs;
    uint32_t max_contacts;
    uint32_t last_solver_parallel_islands;
    uint32_t last_solver_serial_islands;
    int enable_warm_starting;
    int enable_sleeping;
    int ccd_enabled;
    int ccd_requested_by_user;
    int adaptive_runtime_enabled;
    int scientific_diagnostics_enabled;
    UpeAdaptiveMode adaptive_mode;
    UpeIntegratorMode integrator_mode;
    UpeIntegratorMode last_resolved_integrator_mode;
    UpeUnits units;
    UpePhysicalConstants constants;
    UpeScienceErrorBudget error_budget;
    int enable_parallel_island_solver;
    int owns_thread_pool;
    float ccd_threshold_sq;
    float adaptive_target_step_time_ms;
    float max_expected_linear_speed;
    float max_energy_drift_ratio;
    uint32_t adaptive_min_solver_iterations;
    uint32_t adaptive_max_solver_iterations;
    float adaptive_ccd_velocity_threshold_sq;
    float last_peak_linear_speed;
    uint32_t ccd_candidate_pairs;
    uint32_t ccd_tested_pairs;
    uint32_t ccd_contact_hits;
    uint32_t last_active_body_count;
    uint32_t last_broadphase_pair_count;
    uint32_t last_contact_count;
    uint32_t last_constraint_count;
    uint32_t last_island_count;
    uint32_t last_solver_iterations;
    uint32_t last_solver_island_task_count;
    UpeStepStageCallback step_stage_callback;
    void *step_stage_user_data;
    UpeStepStageStat last_step_stats[UPE_STEP_STAGE_COUNT];
    uint32_t last_step_stats_count;

    uint64_t frame_index;
    uint64_t last_state_hash;
    struct UpeReplayRecorder *replay;
    int replay_apply_enabled;
};
