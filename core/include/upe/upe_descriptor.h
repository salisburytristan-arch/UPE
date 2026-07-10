#pragma once

#include <stdint.h>
#include <stddef.h>
#include "upe/upe_config.h"

#ifndef UPE_ENABLE_DESCRIPTOR_UTILS
#define UPE_ENABLE_DESCRIPTOR_UTILS 0
#endif

#if UPE_ENABLE_DESCRIPTOR_UTILS
#include "upe/upe_error.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Solver algorithm selector
 */
typedef enum {
    UPE_SOLVER_PGS = 0,         /**< Projected Gauss-Seidel (fast, stable) */
    UPE_SOLVER_XPBD = 1,        /**< XPBD (compliant constraint, high quality) */
} UpeSolverAlgorithm;

/**
 * Broadphase algorithm selector
 */
typedef enum {
    UPE_BROADPHASE_SAP = 0,     /**< Sweep-and-prune (default, good for axis-aligned updates) */
    UPE_BROADPHASE_BVH = 1,     /**< BVH (future, for complex scenes) */
} UpeBroadphaseAlgorithm;

/**
 * World configuration descriptor
 * All fields have sensible defaults; use UpeWorldDesc_Default() to initialize
 */
typedef struct {
    /* Basic */
    float fixed_dt;                     /**< Fixed simulation timestep in seconds (e.g., 1/60.0) */
    float gravity[3];                   /**< Gravity vector (typically {0, -9.81, 0}) */
    
    /* Solver configuration */
    UpeSolverAlgorithm solver_algorithm;
    uint32_t solver_iterations;         /**< Iterations per step (e.g., 4) */
    uint32_t solver_friction_iterations; /**< Friction-specific iterations (e.g., 1) */
    float slider_bias;                  /**< Baumgarte bias for position error (0.01-0.1) */
    float restitution_threshold;        /**< Minimum velocity for restitution bounce (e.g., 0.5) */
    
    /* Broadphase configuration */
    UpeBroadphaseAlgorithm broadphase_algorithm;
    uint32_t broadphase_grid_cells;     /**< Grid size for spatial hashing (if used, e.g., 32) */
    
    /* Contact filtering */
    uint32_t max_contacts;              /**< Hard limit on active contacts */
    uint32_t max_pairs;                 /**< Hard limit on broad-phase pairs */
    
    /* CCD configuration */
    float ccd_threshold;                /**< Velocity thresholds where CCD activates */
    int ccd_enabled;                    /**< Enable continuous collision detection */
    
    /* Determinism configuration */
    int enable_determinism_mode;        /**< Enforce deterministic execution */
    float determinism_epsilon;          /**< Epsilon for deterministic comparisons */
    
    /* Performance */
    float thread_pool_size;             /**< Number of parallel threads (0 = auto) */
    uint32_t contact_persistence_frames; /**< Keep manifolds across N frames */
    
    /* Memory budgets per frame */
    size_t max_temp_memory;             /**< Max scratch memory per step (e.g., 16 MB) */
    
    /* Optional features */
    int enable_sleeping;                /**< Enable island-based sleeping */
    int enable_ccd;                     /**< Enable continuous collision detection */
    int enable_warm_starting;           /**< Keep constraint impulses across frames */
    int enable_manifold_caching;        /**< Reuse contact manifolds across frames */

    /* Adaptive runtime tuning */
    int enable_adaptive_runtime;        /**< Allow the runtime to tune solver quality and CCD under load */
    UpeAdaptiveMode adaptive_mode;      /**< Tuning bias for adaptive runtime behavior */
    float adaptive_target_step_time_ms; /**< Desired step budget in milliseconds (0 = derive from fixed_dt) */
    uint32_t adaptive_min_solver_iterations; /**< Lower clamp for runtime-selected solver iterations */
    uint32_t adaptive_max_solver_iterations; /**< Upper clamp for runtime-selected solver iterations */
    float adaptive_ccd_velocity_threshold;   /**< Auto-enable CCD when peak speed exceeds this threshold */

    /* Scientific runtime / metrology */
    UpeIntegratorMode integrator_mode;  /**< Integration scheme for dynamic body advancement */
    int enable_scientific_diagnostics;  /**< Track energy, momentum, and stability diagnostics per step */
    float max_expected_linear_speed;    /**< Speed budget before a stability warning is raised (units/sec) */
    float max_energy_drift_ratio;       /**< Fractional total-energy drift budget before flagging */
    UpeUnits units;                     /**< SI scale metadata for interpreting world-space values */
    UpePhysicalConstants constants;     /**< Physical constants used for dimensionless/scientific reporting */
    UpeScienceErrorBudget error_budget; /**< Error-budget and uncertainty controls */
    
    uint32_t reserved[16];              /**< Reserved for future expansion */
} UpeWorldDesc;

/**
 * Solver configuration descriptor (can be changed per-world after creation)
 */
typedef struct {
    UpeSolverAlgorithm algorithm;
    uint32_t iterations;
    uint32_t friction_iterations;
    float bias;
    float restitution_threshold;
    float damping;
    int enable_warm_starting;
    
    uint32_t reserved[8];
} UpeSolverDesc;

/**
 * Body configuration descriptor (v0.10.0 extended version)
 * Note: Use UpeBodyDesc from upe_body.h for current API
 */
typedef struct {
    float position[3];
    float rotation[4];          /**< Quaternion {x, y, z, w} */
    float linear_velocity[3];
    float angular_velocity[3];
    float mass;                 /**< 0 = static body */
    float inertia_scale;        /**< Multiplier on computed inertia (e.g., 1.0) */
    int body_type;              /**< 0=dynamic, 1=static, 2=kinematic */
    uint32_t collision_layer;
    uint32_t collision_mask;
    
    uint32_t reserved[8];
} UpeBodyDesc_Ex;

/**
 * Get default world descriptor (recommended starting point)
 */
#if UPE_ENABLE_DESCRIPTOR_UTILS
UpeWorldDesc UpeWorldDesc_Default(void);

/**
 * Validate world descriptor
 * Returns UPE_OK if valid, otherwise UPE_ERR_* code
 * Sets detailed error message in last-error context
 */
UpeError UpeWorldDesc_Validate(const UpeWorldDesc *desc);

/**
 * Get default solver descriptor
 */
UpeSolverDesc UpeSolverDesc_Default(void);

/**
 * Validate solver descriptor
 */
UpeError UpeSolverDesc_Validate(const UpeSolverDesc *desc);

/**
 * Get default body descriptor (v0.10.0 extended version)
 * Note: Use simple UpeBodyDesc from upe_body.h for current API
 */
UpeBodyDesc_Ex UpeBodyDesc_ExDefault(void);

/**
 * Validate body descriptor
 */
UpeError UpeBodyDesc_ExValidate(const UpeBodyDesc_Ex *desc);
#endif

#ifdef __cplusplus
}
#endif
