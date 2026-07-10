#include "upe/upe_warm_start.h"
#include "upe/upe_math.h"
#include "upe/upe_body.h"
#include <string.h>
#include <math.h>

static UpeWarmStartStats g_upe_warm_start_stats = {};

static UpeVec3 UpeWarmStart_NormalizeOrDefault(UpeVec3 v) {
    const float len_sq = v.x * v.x + v.y * v.y + v.z * v.z;
    if (len_sq <= 1.0e-12f) {
        return UpeVec3{0.0f, 1.0f, 0.0f};
    }
    const float inv_len = 1.0f / sqrtf(len_sq);
    return UpeVec3{v.x * inv_len, v.y * inv_len, v.z * inv_len};
}

static UpeVec3 UpeWarmStart_BuildTangent(UpeVec3 normal) {
    UpeVec3 axis = (fabsf(normal.y) < 0.9f) ? UpeVec3{0.0f, 1.0f, 0.0f} : UpeVec3{1.0f, 0.0f, 0.0f};
    return UpeWarmStart_NormalizeOrDefault(UpeVec3_Cross(axis, normal));
}

void UpeWarmStart_InitStats(UpeWarmStartStats *stats) {
    if (!stats) return;
    memset(stats, 0, sizeof(UpeWarmStartStats));
}

uint32_t UpeWarmStart_ApplyManifoldImpulses(UpeWorld *world, UpeCachedManifold *manifold,
                                             UpeBody *body_a, UpeBody *body_b,
                                             float reuse_threshold) {
    (void)world;
    if (!manifold || !body_a || !body_b || !manifold->is_valid || manifold->point_count == 0u) {
        return 0u;
    }

    const UpeVec3 normal = UpeWarmStart_NormalizeOrDefault(manifold->normal);
    const UpeVec3 tangent0 = UpeWarmStart_BuildTangent(normal);
    const UpeVec3 tangent1 = UpeWarmStart_NormalizeOrDefault(UpeVec3_Cross(normal, tangent0));

    uint32_t reused = 0u;
    for (uint32_t i = 0; i < manifold->point_count; ++i) {
        UpeManifoldPoint *point = &manifold->points[i];
        if (!point->is_active) {
            continue;
        }

        float impulse_measure = fabsf(point->normal_impulse) +
                                fabsf(point->tangent_impulse[0]) +
                                fabsf(point->tangent_impulse[1]);
        if (impulse_measure <= fmaxf(1.0e-6f, reuse_threshold * 0.01f)) {
            continue;
        }

        UpeVec3 impulse = UpeVec3_Add(UpeVec3_Scale(normal, point->normal_impulse),
                                      UpeVec3_Add(UpeVec3_Scale(tangent0, point->tangent_impulse[0]),
                                                  UpeVec3_Scale(tangent1, point->tangent_impulse[1])));
        UpeBody_ApplyImpulse(body_a, UpeVec3_Scale(impulse, -1.0f));
        UpeBody_ApplyImpulse(body_b, impulse);
        reused++;
    }

    g_upe_warm_start_stats.frames_processed++;
    if (reused > 0u) {
        g_upe_warm_start_stats.manifolds_warmed++;
        g_upe_warm_start_stats.total_impulses_reused += reused;
        uint32_t age = (manifold->frame_last_updated >= manifold->frame_created)
            ? (manifold->frame_last_updated - manifold->frame_created)
            : 0u;
        float warmed = (float)g_upe_warm_start_stats.manifolds_warmed;
        g_upe_warm_start_stats.avg_manifold_age =
            ((g_upe_warm_start_stats.avg_manifold_age * (warmed - 1.0f)) + (float)age) / warmed;
    } else {
        g_upe_warm_start_stats.rejections_due_to_movement++;
    }

    return reused;
}

int UpeWarmStart_ValidateManifoldReuse(const UpeCachedManifold *manifold,
                                        const UpeVec3 *contact_points_a,
                                        const UpeVec3 *contact_points_b,
                                        uint32_t contact_count,
                                        float reuse_threshold) {
    if (!manifold || !contact_points_a || !contact_points_b || !manifold->is_valid ||
        contact_count == 0u || manifold->point_count == 0u) {
        return 0;
    }
    if (manifold->point_count > contact_count) {
        return 0;
    }

    uint32_t age = (manifold->frame_last_updated >= manifold->frame_created)
        ? (manifold->frame_last_updated - manifold->frame_created)
        : 0u;
    if (age > 240u) {
        return 0;
    }

    const float tolerance = fmaxf(reuse_threshold, 1.0e-4f);
    for (uint32_t i = 0; i < manifold->point_count; ++i) {
        const UpeManifoldPoint *point = &manifold->points[i];
        if (!point->is_active) {
            continue;
        }
        if (!UpeManifoldPoint_IsValid(point, contact_points_a[i], contact_points_b[i], tolerance)) {
            return 0;
        }
    }
    return 1;
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
    *stats = g_upe_warm_start_stats;
}

void UpeWarmStart_ResetStats(UpeWorld *world) {
    (void)world;
    memset(&g_upe_warm_start_stats, 0, sizeof(g_upe_warm_start_stats));
}
