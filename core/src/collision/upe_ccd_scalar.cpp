#include "upe/upe_ccd_scalar.h"
#include "upe/upe_math.h"
#include <math.h>

/**
 * Continuous collision detection (CCD) scalar implementation
 *
 * Uses quadratic formula and iterative distance queries to compute
 * time of impact (TOI) for moving objects.
 */

typedef struct {
    uint32_t queries_performed;
    uint32_t collisions_detected;
    uint32_t tunneling_prevented;
} UpeCCDStatsImpl;

static UpeCCDStatsImpl g_ccd_stats = {};

typedef struct {
    UpeVec3 points[4];
    uint32_t count;
} UpeCcdSimplex;

static UpeVec3 UpeCcd_SupportPoint(const float *vertices, uint32_t vertex_count, UpeVec3 dir, UpeVec3 offset) {
    UpeVec3 best = {vertices[0], vertices[1], vertices[2]};
    best = UpeVec3_Add(best, offset);
    float max_dot = UpeVec3_Dot(best, dir);
    for (uint32_t i = 1; i < vertex_count; i++) {
        UpeVec3 v = {vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2]};
        v = UpeVec3_Add(v, offset);
        float d = UpeVec3_Dot(v, dir);
        if (d > max_dot) {
            max_dot = d;
            best = v;
        }
    }
    return best;
}

static UpeVec3 UpeCcd_MinkowskiSupport(const float *vertices_a, uint32_t count_a, UpeVec3 offset_a,
                                       const float *vertices_b, uint32_t count_b, UpeVec3 dir) {
    UpeVec3 support_a = UpeCcd_SupportPoint(vertices_a, count_a, dir, offset_a);
    UpeVec3 neg_dir = {-dir.x, -dir.y, -dir.z};
    UpeVec3 support_b = UpeCcd_SupportPoint(vertices_b, count_b, neg_dir, {0.0f, 0.0f, 0.0f});
    return UpeVec3_Sub(support_a, support_b);
}

static int UpeCcd_SimplexContainsOrigin(UpeCcdSimplex *simplex, UpeVec3 *dir) {
    UpeVec3 a = simplex->points[simplex->count - 1];
    UpeVec3 ao = {-a.x, -a.y, -a.z};

    if (simplex->count == 2) {
        UpeVec3 b = simplex->points[0];
        UpeVec3 ab = UpeVec3_Sub(b, a);
        UpeVec3 ab_perp = UpeVec3_Cross(UpeVec3_Cross(ab, ao), ab);
        if (UpeVec3_Length(ab_perp) < 1e-6f) {
            *dir = ao;
        } else {
            *dir = UpeVec3_Normalize(ab_perp);
        }
        return 0;
    }

    if (simplex->count == 3) {
        UpeVec3 b = simplex->points[1];
        UpeVec3 c = simplex->points[0];
        UpeVec3 ab = UpeVec3_Sub(b, a);
        UpeVec3 ac = UpeVec3_Sub(c, a);
        UpeVec3 abc = UpeVec3_Cross(ab, ac);

        UpeVec3 ab_perp = UpeVec3_Cross(UpeVec3_Cross(ab, abc), ab);
        if (UpeVec3_Dot(ab_perp, ao) > 0.0f) {
            simplex->points[0] = b;
            simplex->points[1] = a;
            simplex->count = 2;
            *dir = UpeVec3_Normalize(ab_perp);
            return 0;
        }

        UpeVec3 ac_perp = UpeVec3_Cross(UpeVec3_Cross(abc, ac), ac);
        if (UpeVec3_Dot(ac_perp, ao) > 0.0f) {
            simplex->points[0] = c;
            simplex->points[1] = a;
            simplex->count = 2;
            *dir = UpeVec3_Normalize(ac_perp);
            return 0;
        }

        if (UpeVec3_Dot(abc, ao) > 0.0f) {
            *dir = UpeVec3_Normalize(abc);
        } else {
            UpeVec3 tmp = simplex->points[0];
            simplex->points[0] = simplex->points[1];
            simplex->points[1] = tmp;
            *dir = UpeVec3_Normalize(UpeVec3_Scale(abc, -1.0f));
        }
        return 0;
    }

    if (simplex->count == 4) {
        UpeVec3 b = simplex->points[2];
        UpeVec3 c = simplex->points[1];
        UpeVec3 d = simplex->points[0];

        UpeVec3 ab = UpeVec3_Sub(b, a);
        UpeVec3 ac = UpeVec3_Sub(c, a);
        UpeVec3 ad = UpeVec3_Sub(d, a);

        UpeVec3 abc = UpeVec3_Cross(ab, ac);
        UpeVec3 acd = UpeVec3_Cross(ac, ad);
        UpeVec3 adb = UpeVec3_Cross(ad, ab);

        if (UpeVec3_Dot(abc, ao) > 0.0f) {
            simplex->points[0] = c;
            simplex->points[1] = b;
            simplex->points[2] = a;
            simplex->count = 3;
            *dir = UpeVec3_Normalize(abc);
            return 0;
        }
        if (UpeVec3_Dot(acd, ao) > 0.0f) {
            simplex->points[0] = d;
            simplex->points[1] = c;
            simplex->points[2] = a;
            simplex->count = 3;
            *dir = UpeVec3_Normalize(acd);
            return 0;
        }
        if (UpeVec3_Dot(adb, ao) > 0.0f) {
            simplex->points[0] = b;
            simplex->points[1] = d;
            simplex->points[2] = a;
            simplex->count = 3;
            *dir = UpeVec3_Normalize(adb);
            return 0;
        }

        return 1;
    }

    *dir = ao;
    return 0;
}

static int UpeCcd_GjkIntersectAtTime(const float *vertices_a, uint32_t vertex_count_a,
                                     const float *vertices_b, uint32_t vertex_count_b,
                                     UpeVec3 velocity_a, float t, UpeVec3 *out_dir) {
    UpeVec3 offset_a = UpeVec3_Scale(velocity_a, t);
    UpeVec3 dir = UpeVec3_Normalize(UpeVec3_Scale(velocity_a, -1.0f));
    if (UpeVec3_Length(dir) < 1e-6f) {
        dir = {1.0f, 0.0f, 0.0f};
    }

    UpeCcdSimplex simplex = {};

    for (uint32_t iter = 0; iter < 32; iter++) {
        UpeVec3 support = UpeCcd_MinkowskiSupport(vertices_a, vertex_count_a, offset_a,
                                                  vertices_b, vertex_count_b, dir);
        if (UpeVec3_Dot(support, dir) < 0.0f) {
            if (out_dir) {
                *out_dir = dir;
            }
            return 0;
        }

        simplex.points[simplex.count++] = support;
        if (UpeCcd_SimplexContainsOrigin(&simplex, &dir)) {
            if (out_dir) {
                *out_dir = dir;
            }
            return 1;
        }
    }

    if (out_dir) {
        *out_dir = dir;
    }
    return 0;
}

/* Helper: solve quadratic at^2 + bt + c = 0 for smallest t in [0, 1] */
static float UpeQuadratic_Solve(float a, float b, float c, float *t1, float *t2) {
    float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f) {
        *t1 = -1.0f;  /* No real solution */
        *t2 = -1.0f;
        return -1.0f;
    }
    
    float sqrt_disc = sqrtf(discriminant);
    float inv_2a = 1.0f / (2.0f * a);
    
    *t1 = (-b - sqrt_disc) * inv_2a;
    *t2 = (-b + sqrt_disc) * inv_2a;
    
    /* Return earliest positive time in [0, 1] */
    if (*t1 >= 0.0f && *t1 <= 1.0f) return *t1;
    if (*t2 >= 0.0f && *t2 <= 1.0f) return *t2;
    return -1.0f;
}

int UpeCCD_SphereSphere(UpeVec3 p0_a, UpeVec3 p1_a, float radius_a,
                        UpeVec3 p_b, float radius_b,
                        UpeCCDResult *out_result) {
    if (!out_result) return 0;
    
    g_ccd_stats.queries_performed++;
    
    /* Relative position and velocity */
    UpeVec3 rel_p0 = UpeVec3_Sub(p0_a, p_b);
    UpeVec3 rel_v = UpeVec3_Sub(p1_a, p0_a);
    
    float combined_radius = radius_a + radius_b;
    
    /* Solve |p0 + v*t|^2 = r^2
     * => |p0|^2 + 2*(p0·v)*t + |v|^2*t^2 = r^2
     * => |v|^2*t^2 + 2*(p0·v)*t + (|p0|^2 - r^2) = 0 */
    
    float a = UpeVec3_Dot(rel_v, rel_v);
    float b = 2.0f * UpeVec3_Dot(rel_p0, rel_v);
    float c = UpeVec3_Dot(rel_p0, rel_p0) - combined_radius * combined_radius;
    
    /* Handle degenerate case (zero velocity) */
    if (fabsf(a) < 1e-6f) {
        if (c <= 0.0f) {
            out_result->time_of_impact = 0.0f;
            out_result->contact_point = p0_a;
            return 1;
        }
        return 0;
    }
    
    float t1, t2;
    float t_impact = UpeQuadratic_Solve(a, b, c, &t1, &t2);
    
    if (t_impact < 0.0f) return 0;  /* No collision during time step */
    
    /* Contact point at time t_impact */
    UpeVec3 contact_at_impact = UpeVec3_Add(p0_a, UpeVec3_Scale(rel_v, t_impact));
    
    out_result->time_of_impact = t_impact;
    out_result->contact_point = contact_at_impact;
    out_result->separation = 0.0f;
    
    g_ccd_stats.collisions_detected++;
    return 1;
}

int UpeCCD_SpherePlane(UpeVec3 p0, UpeVec3 p1, float radius,
                       UpeVec3 plane_point, UpeVec3 plane_normal,
                       UpeCCDResult *out_result) {
    if (!out_result) return 0;
    
    g_ccd_stats.queries_performed++;
    
    /* Normalize plane normal */
    UpeVec3 n = UpeVec3_Normalize(plane_normal);
    
    /* Distance from sphere start to plane */
    UpeVec3 rel_p = UpeVec3_Sub(p0, plane_point);
    float dist0 = UpeVec3_Dot(rel_p, n);
    
    /* Sphere velocity along normal */
    UpeVec3 vel = UpeVec3_Sub(p1, p0);
    float vel_n = UpeVec3_Dot(vel, n);
    
    /* If sphere is moving away from plane, no collision */
    if (vel_n >= 0.0f) return 0;
    
    /* Time to impact: when will |dist0 + vel_n * t| == radius */
    float impact_dist = dist0 - radius;
    
    if (impact_dist <= 0.0f) {
        /* Already penetrating */
        out_result->time_of_impact = 0.0f;
        out_result->contact_point = UpeVec3_Sub(p0, UpeVec3_Scale(n, radius));
        return 1;
    }
    
    float t_impact = -impact_dist / vel_n;  /* vel_n is negative */
    
    if (t_impact < 0.0f || t_impact > 1.0f) return 0;
    
    UpeVec3 contact_point = UpeVec3_Add(p0, UpeVec3_Scale(vel, t_impact));
    contact_point = UpeVec3_Sub(contact_point, UpeVec3_Scale(n, radius));
    
    out_result->time_of_impact = t_impact;
    out_result->contact_point = contact_point;
    out_result->separation = 0.0f;
    
    g_ccd_stats.collisions_detected++;
    return 1;
}

int UpeCCD_SphereCapsule(UpeVec3 p0, UpeVec3 p1, float radius,
                         UpeVec3 cap_a, UpeVec3 cap_b, float cap_radius,
                         UpeCCDResult *out_result) {
    if (!out_result) return 0;
    
    g_ccd_stats.queries_performed++;
    
    /* Capsule axis and length */
    UpeVec3 cap_axis = UpeVec3_Sub(cap_b, cap_a);
    float cap_len = UpeVec3_Length(cap_axis);
    
    if (cap_len < 1e-6f) {
        /* Degenerate capsule, treat as sphere */
        return UpeCCD_SphereSphere(p0, p1, radius, cap_a, cap_radius, out_result);
    }
    
    /* Normalize capsule axis */
    UpeVec3 cap_dir = UpeVec3_Scale(cap_axis, 1.0f / cap_len);
    
    /* Find closest point on capsule line to p0 */
    UpeVec3 rel_p = UpeVec3_Sub(p0, cap_a);
    float t = UpeVec3_Dot(rel_p, cap_dir);
    t = (t < 0.0f) ? 0.0f : (t > cap_len) ? cap_len : t;
    
    UpeVec3 closest_on_capsule = UpeVec3_Add(cap_a, UpeVec3_Scale(cap_dir, t));
    
    /* Use sphere-sphere collision with closest point as target */
    return UpeCCD_SphereSphere(p0, p1, radius, closest_on_capsule, cap_radius, out_result);
}

int UpeCCD_CapsuleCapsule(UpeVec3 p0_a, UpeVec3 p1_a, UpeVec3 p0_b, UpeVec3 p1_b,
                          float radius_a, float radius_b,
                          UpeCCDResult *out_result) {
    if (!out_result) return 0;
    
    g_ccd_stats.queries_performed++;
    
    /* For now, approximate as sphere of capsule A sweeping against capsule B */
    UpeVec3 center_a = UpeVec3_Scale(UpeVec3_Add(p0_a, p1_a), 0.5f);
    UpeVec3 center_a_end = UpeVec3_Scale(UpeVec3_Add(p1_a, p1_a), 0.5f);
    
    return UpeCCD_SphereCapsule(center_a, center_a_end, radius_a,
                                p0_b, p1_b, radius_b, out_result);
}

int UpeCCD_ConvexConvex(const float *vertices_a, uint32_t vertex_count_a,
                        const float *vertices_b, uint32_t vertex_count_b,
                        UpeVec3 velocity_a,
                        UpeCCDResult *out_result) {
    if (!out_result || !vertices_a || !vertices_b) return 0;
    if (vertex_count_a == 0 || vertex_count_b == 0) return 0;
    
    g_ccd_stats.queries_performed++;
    
    UpeVec3 dir = {1.0f, 0.0f, 0.0f};
    if (UpeCcd_GjkIntersectAtTime(vertices_a, vertex_count_a,
                                  vertices_b, vertex_count_b,
                                  velocity_a, 0.0f, &dir)) {
        out_result->time_of_impact = 0.0f;
        out_result->contact_point = UpeCcd_SupportPoint(vertices_a, vertex_count_a, UpeVec3_Scale(dir, -1.0f), {0.0f, 0.0f, 0.0f});
        out_result->separation = 0.0f;
        g_ccd_stats.collisions_detected++;
        return 1;
    }

    if (!UpeCcd_GjkIntersectAtTime(vertices_a, vertex_count_a,
                                   vertices_b, vertex_count_b,
                                   velocity_a, 1.0f, &dir)) {
        return 0;
    }

    float lo = 0.0f;
    float hi = 1.0f;
    for (uint32_t iter = 0; iter < 24; iter++) {
        float mid = 0.5f * (lo + hi);
        if (UpeCcd_GjkIntersectAtTime(vertices_a, vertex_count_a,
                                      vertices_b, vertex_count_b,
                                      velocity_a, mid, &dir)) {
            hi = mid;
        } else {
            lo = mid;
        }
    }

    UpeVec3 offset = UpeVec3_Scale(velocity_a, hi);
    UpeVec3 point_a = UpeCcd_SupportPoint(vertices_a, vertex_count_a, UpeVec3_Scale(dir, -1.0f), offset);
    UpeVec3 zero_offset = {0.0f, 0.0f, 0.0f};
    UpeVec3 point_b = UpeCcd_SupportPoint(vertices_b, vertex_count_b, dir, zero_offset);

    out_result->time_of_impact = hi;
    out_result->contact_point = UpeVec3_Scale(UpeVec3_Add(point_a, point_b), 0.5f);
    out_result->separation = UpeVec3_Length(UpeVec3_Sub(point_a, point_b));

    g_ccd_stats.collisions_detected++;
    g_ccd_stats.tunneling_prevented++;
    return 1;
}

float UpeCCD_SafeTOI(float raw_toi) {
    if (raw_toi < 0.0f) return 0.0f;
    if (raw_toi > 1.0f) return 1.0f;
    return raw_toi;
}

void UpeCCD_GetStats(UpeCCDStats *stats) {
    if (!stats) return;
    stats->queries_performed = g_ccd_stats.queries_performed;
    stats->collisions_detected = g_ccd_stats.collisions_detected;
    stats->tunneling_prevented = g_ccd_stats.tunneling_prevented;
}

void UpeCCD_ResetStats(void) {
    g_ccd_stats.queries_performed = 0;
    g_ccd_stats.collisions_detected = 0;
    g_ccd_stats.tunneling_prevented = 0;
}
