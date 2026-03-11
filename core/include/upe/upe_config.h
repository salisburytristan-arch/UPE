#pragma once

#include <stdint.h>

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

#ifdef __cplusplus
}
#endif
