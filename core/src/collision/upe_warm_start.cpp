#include "upe/upe_warm_start.h"
#include "upe/upe_math.h"
#include <string.h>

void UpeWarmStart_InitStats(UpeWarmStartStats *stats) {
    if (!stats) return;
    memset(stats, 0, sizeof(UpeWarmStartStats));
}

uint32_t UpeWarmStart_ApplyManifoldImpulses(UpeWorld *world, UpeCachedManifold *manifold,
                                             UpeBody *body_a, UpeBody *body_b,
                                             float reuse_threshold) {
    /* Stub: warm-start not implemented in v0.11.0 */
    (void)world; (void)manifold; (void)body_a; (void)body_b; (void)reuse_threshold;
    return 0;
}

int UpeWarmStart_ValidateManifoldReuse(const UpeCachedManifold *manifold,
                                        const UpeVec3 *contact_points_a,
                                        const UpeVec3 *contact_points_b,
                                        uint32_t contact_count,
                                        float reuse_threshold) {
    (void)manifold; (void)contact_points_a; (void)contact_points_b;
    (void)contact_count; (void)reuse_threshold;
    return 0;  /* No reuse */
}

void UpeWarmStart_ClearImpulses(UpeCachedManifold *manifold) {
    if (!manifold) return;
    for (uint32_t i = 0; i < manifold->point_count; i++) {
        manifold->points[i].normal_impulse = 0.0f;
        manifold->points[i].tangent_impulse[0] = 0.0f;
        manifold->points[i].tangent_impulse[1] = 0.0f;
    }
}

void UpeWarmStart_GetStats(const UpeWorld *world, UpeWarmStartStats *stats) {
    (void)world;
    if (!stats) return;
    memset(stats, 0, sizeof(UpeWarmStartStats));
}

void UpeWarmStart_ResetStats(UpeWorld *world) {
    (void)world;
}
