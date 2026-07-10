#pragma once

#include <stdint.h>
#include "upe/upe_units.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Engine profile selector: compile-time or runtime
 * Profiles trade off features vs overhead for specific use cases
 */
typedef enum {
    UPE_PROFILE_GAME = 0,           /**< Realtime game mode: balanced throughput + features, fixed dt */
    UPE_PROFILE_CINEMATIC = 1,      /**< Offline/non-realtime: unlimited iterations, adaptive dt, max accuracy */
    UPE_PROFILE_SCIENTIFIC = 2,     /**< Research/analysis: exhaustive precision + validation, minimal early exits */
} UpeProfile;

/**
 * Determinism mode selector
 */
typedef enum {
    UPE_DETERMINISM_DISABLED = 0,   /**< Standard float16/64, fast but non-deterministic */
    UPE_DETERMINISM_FIXED_POINT = 1, /**< Q48.16 fixed-point scalars, deterministic across platforms */
    UPE_DETERMINISM_COMPATIBLE = 2, /**< Deterministic within same binary version, new version may differ */
} UpeDeterminismMode;

/**
 * Adaptive runtime tuning mode.
 * Lets the engine scale solver quality and CCD aggressiveness based on scene pressure.
 */
typedef enum {
    UPE_ADAPTIVE_DISABLED = 0,   /**< Fixed behavior only; no runtime tuning */
    UPE_ADAPTIVE_BALANCED = 1,   /**< Balance stability and frame budget */
    UPE_ADAPTIVE_RESPONSIVE = 2, /**< Prefer throughput and low-latency response */
    UPE_ADAPTIVE_STABLE = 3,     /**< Prefer more solver quality under heavy contact load */
} UpeAdaptiveMode;

/**
 * Runtime tuning knobs for adaptive stepping quality.
 */
typedef struct {
    int enabled;                          /**< Non-zero to enable adaptive runtime tuning */
    UpeAdaptiveMode mode;                /**< Tuning bias */
    float target_step_time_ms;           /**< Target simulation budget in milliseconds (0 = derive from fixed_dt) */
    uint32_t min_solver_iterations;      /**< Lower clamp for runtime-selected solver iterations */
    uint32_t max_solver_iterations;      /**< Upper clamp for runtime-selected solver iterations */
    float ccd_velocity_threshold;        /**< Auto-enable CCD above this speed (units/sec) */
} UpeAdaptiveRuntimeConfig;

/**
 * Integrator selection for world stepping.
 */
typedef enum {
    UPE_INTEGRATOR_EULER = 0,           /**< Fast baseline, least stable */
    UPE_INTEGRATOR_SEMI_IMPLICIT = 1,   /**< Stable default for rigid-body gameplay */
    UPE_INTEGRATOR_VELOCITY_VERLET = 2, /**< Better energy behavior for scientific/long-horizon runs */
    UPE_INTEGRATOR_ADAPTIVE = 3,        /**< Runtime chooses between fast/stable schemes */
} UpeIntegratorMode;

/**
 * Stability/status flags surfaced in step metrics.
 */
typedef enum {
    UPE_STABILITY_OK = 0u,
    UPE_STABILITY_HIGH_SPEED = (1u << 0),
    UPE_STABILITY_ENERGY_DRIFT = (1u << 1),
    UPE_STABILITY_DEEP_PENETRATION = (1u << 2),
    UPE_STABILITY_NONFINITE = (1u << 3),
    UPE_STABILITY_HIGH_COURANT = (1u << 4),
    UPE_STABILITY_HIGH_REYNOLDS = (1u << 5),
} UpeScientificStabilityFlags;

typedef struct {
    float length_tolerance_si;
    float velocity_tolerance_si;
    float energy_tolerance_ratio;
    float contact_margin_factor;
    float uncertainty_sigma;
} UpeScienceErrorBudget;

/**
 * Scientific runtime controls for units, integrator selection, and quality diagnostics.
 */
typedef struct {
    int enabled;                         /**< Non-zero to collect deeper scientific diagnostics */
    UpeUnits units;                     /**< SI scale metadata for converting engine values */
    UpePhysicalConstants constants;     /**< Physical constants registry for non-dimensional diagnostics */
    UpeScienceErrorBudget error_budget; /**< Error budget and uncertainty propagation knobs */
    UpeIntegratorMode integrator_mode;  /**< Requested integration scheme */
    float max_expected_linear_speed;    /**< Speed budget before a stability flag is raised */
    float max_energy_drift_ratio;       /**< Allowed fractional total-energy drift before flagging */
} UpeScientificRuntimeConfig;

/**
 * Feature capability bits: runtime discovery for bindings/tools
 */
typedef enum {
    UPE_CAP_SCALAR_MATH = (1u << 0),              /**< Scalar abstraction layer available */
    UPE_CAP_FIXED_POINT_Q48_16 = (1u << 1),      /**< Q48.16 fixed-point math compiled in */
    UPE_CAP_BROADPHASE_SAP = (1u << 2),           /**< SAP broad-phase available */
    UPE_CAP_BROADPHASE_BVH = (1u << 3),           /**< BVH broad-phase available (future) */
    UPE_CAP_NARROWPHASE_GJK = (1u << 4),          /**< GJK collision kernel available */
    UPE_CAP_NARROWPHASE_SCALAR = (1u << 5),       /**< Scalar deterministic narrowphase available */
    UPE_CAP_CONSTRAINTS_JACOBIAN = (1u << 6),     /**< Jacobian-based constraint solver */
    UPE_CAP_FLUIDS_SPH = (1u << 7),               /**< SPH fluid simulation */
    UPE_CAP_CLOTH_XPBD = (1u << 8),               /**< XPBD cloth/hair */
    UPE_CAP_FRACTURE = (1u << 9),                 /**< Fracture dynamics */
    UPE_CAP_VEHICLES = (1u << 10),                /**< Vehicle dynamics (tire/suspension) */
    UPE_CAP_RAGDOLL_ARTICULATION = (1u << 11),    /**< Articulation (Featherstone/ABA) */
    UPE_CAP_CHARACTER_CONTROLLER = (1u << 12),    /**< Character controller (step-up, slopes) */
    UPE_CAP_2D_MODE = (1u << 13),                 /**< Pure 2D physics fast-path */
    UPE_CAP_CCD = (1u << 14),                     /**< Continuous collision detection */
    UPE_CAP_MANIFOLD_CACHE = (1u << 15),          /**< Contact manifold caching */
    UPE_CAP_REPLAY_DETERMINISM = (1u << 16),      /**< Replay + determinism validation */
    UPE_CAP_GPU_BROADPHASE = (1u << 17),          /**< GPU compute broad-phase (CUDA/compute shader) */
    UPE_CAP_GPU_SOLVER = (1u << 18),              /**< GPU compute solver (future) */
    UPE_CAP_TELEMETRY = (1u << 19),               /**< Performance telemetry ring buffer */
    UPE_CAP_FIELD_FORCES = (1u << 20),            /**< Vector field forces (wind, buoyancy, etc) */
    UPE_CAP_SCIENCE_CATALOG = (1u << 21),         /**< Science domain status/catalog API available */
    UPE_CAP_SCIENCE_TOPICS = (1u << 22),          /**< Science topic-level catalog API available */
    UPE_CAP_ADAPTIVE_RUNTIME = (1u << 23),        /**< Adaptive runtime quality tuning exposed through the public API */
    UPE_CAP_SCIENTIFIC_RUNTIME = (1u << 24),      /**< Unit-aware diagnostics and scientific runtime controls available */
    UPE_CAP_NUMERICAL_TOOLBOX = (1u << 25),       /**< Sparse solvers, ODE steppers, FFT, and optimization helpers available */
} UpeCapabilityBits;

/**
 * Engine version tuple: major.minor.patch
 */
typedef struct {
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
    uint16_t reserved;
} UpeVersion;

/**
 * Engine capabilities and limits reported at runtime
 */
typedef struct {
    UpeVersion version;
    uint32_t capability_bits;            /**< Bitmask of UpeCapabilityBits */
    
    uint32_t simd_width;                 /**< SIMD register width: 4=SSE, 8=AVX2, 16=AVX-512 */
    uint32_t max_bodies;                 /**< Hard limit on body count */
    uint32_t max_shapes;                 /**< Hard limit on shape count */
    uint32_t max_contacts;               /**< Hard limit on active contact count */
    uint32_t max_constraints;            /**< Hard limit on solver constraint rows */
    
    uint32_t threading_model;            /**< 0=disabled, >0=thread count */
    UpeDeterminismMode determinism_mode; /**< Determinism guarantee level */
    UpeProfile profile;                  /**< Compile-time profile */
    
    uint32_t reserved[8];                /**< Reserved for future expansion */
} UpeCapabilities;

/**
 * Query engine capabilities at runtime
 * Safe to call any time; returns immutable struct
 */
UpeCapabilities UpeEngine_GetCapabilities(void);

/**
 * Get human-readable version string
 * Returns static pointer, do not free
 */
const char *UpeEngine_GetVersionString(void);

/**
 * Get build configuration string (profile, determinism mode, compile flags)
 * Returns static pointer, do not free
 */
const char *UpeEngine_GetBuildConfig(void);

/**
 * Get recommended default adaptive runtime tuning settings.
 */
UpeAdaptiveRuntimeConfig UpeAdaptiveRuntimeConfig_Default(void);

/**
 * Get recommended default uncertainty/error budget settings.
 */
UpeScienceErrorBudget UpeScienceErrorBudget_Default(void);

/**
 * Get recommended default scientific runtime settings.
 */
UpeScientificRuntimeConfig UpeScientificRuntimeConfig_Default(void);

#ifdef __cplusplus
}
#endif
