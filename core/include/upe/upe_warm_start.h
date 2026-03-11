#ifndef UPE_WARM_START_H
#define UPE_WARM_START_H

#include <stdint.h>
#include "upe/upe_math.h"
#include "upe/upe_manifold_cache.h"

typedef struct UpeWorld UpeWorld;
typedef struct UpeBody UpeBody;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Warm-starting: reuse solver impulses from previous frame to accelerate convergence
 *
 * When bodies are in persistent contact, their manifold from the previous frame
 * contains the impulses that resolved collisions. By applying these impulses at the
 * start of the new frame's solver, we provide a "warm start" that significantly
 * reduces the number of iterations needed to reach equilibrium.
 *
 * This is especially effective for stacked objects, friction-heavy contacts, and
 * constraint-heavy manifolds.
 */

/* Warm-start statistics for profiling */
typedef struct {
    uint32_t frames_processed;
    uint32_t manifolds_warmed;
    uint32_t total_impulses_reused;
    uint32_t rejections_due_to_movement;  /* Manifolds moved too much to reuse */
    float avg_manifold_age;
} UpeWarmStartStats;

/**
 * Initialize warm-starting statistics
 */
void UpeWarmStart_InitStats(UpeWarmStartStats *stats);

/**
 * Apply warm-start impulses from cached manifold to bodies
 *
 * This is called during solver initialization before constraint solving begins.
 * It applies the previous frame's normal and tangent impulses directly to the
 * bodies, giving them velocities that satisfy the constraints without solving.
 *
 * Returns how many impulses were applied (for statistics).
 */
uint32_t UpeWarmStart_ApplyManifoldImpulses(UpeWorld *world, UpeCachedManifold *manifold,
                                             UpeBody *body_a, UpeBody *body_b,
                                             float reuse_threshold);

/**
 * Validate if a cached manifold is still valid for warm-starting
 *
 * Checks that:
 * - Contact points haven't moved too much (within reuse_threshold)
 * - Both bodies are still in contact
 * - Manifold isn't too old
 *
 * Returns 1 if valid, 0 otherwise.
 */
int UpeWarmStart_ValidateManifoldReuse(const UpeCachedManifold *manifold,
                                        const UpeVec3 *contact_points_a,
                                        const UpeVec3 *contact_points_b,
                                        uint32_t contact_count,
                                        float reuse_threshold);

/**
 * Clear impulses in manifold (useful for game pauses, resets, etc.)
 */
void UpeWarmStart_ClearImpulses(UpeCachedManifold *manifold);

/**
 * Get warm-start stats for profiling
 */
void UpeWarmStart_GetStats(const UpeWorld *world, UpeWarmStartStats *stats);

/**
 * Reset warm-start statistics
 */
void UpeWarmStart_ResetStats(UpeWorld *world);

#ifdef __cplusplus
}
#endif

#endif /* UPE_WARM_START_H */
