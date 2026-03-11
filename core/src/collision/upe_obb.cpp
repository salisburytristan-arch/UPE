#include "upe/upe_obb.h"
#include <math.h>
#include <float.h>
#include <string.h>

static const float UPE_OBB_EPS = 1e-6f;

static inline float upe_absf(float x) { return x >= 0.0f ? x : -x; }
static inline float upe_minf(float a, float b) { return a < b ? a : b; }
static inline float upe_maxf(float a, float b) { return a > b ? a : b; }
static inline float upe_clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static UpeVec3 upe_vec3_make(float x, float y, float z) {
    UpeVec3 v; v.x = x; v.y = y; v.z = z; return v;
}

static void upe_obb_compute_axes(UpeOBB *obb) {
    obb->axes[0] = UpeQuat_RotateVec3(obb->rotation, upe_vec3_make(1.0f, 0.0f, 0.0f));
    obb->axes[1] = UpeQuat_RotateVec3(obb->rotation, upe_vec3_make(0.0f, 1.0f, 0.0f));
    obb->axes[2] = UpeQuat_RotateVec3(obb->rotation, upe_vec3_make(0.0f, 0.0f, 1.0f));
}

void UpeOBB_FromCenterExtents(UpeOBB *obb, UpeVec3 center, UpeVec3 half_extents, UpeQuat rotation) {
    if (!obb) return;
    obb->center = center;
    obb->half_extents = half_extents;
    obb->rotation = rotation;
    upe_obb_compute_axes(obb);
}

void UpeOBB_FromAABB(UpeOBB *obb, UpeVec3 min_bounds, UpeVec3 max_bounds, UpeQuat rotation) {
    if (!obb) return;
    UpeVec3 center = upe_vec3_make(0.5f * (min_bounds.x + max_bounds.x),
                                   0.5f * (min_bounds.y + max_bounds.y),
                                   0.5f * (min_bounds.z + max_bounds.z));
    UpeVec3 half_extents = upe_vec3_make(0.5f * (max_bounds.x - min_bounds.x),
                                         0.5f * (max_bounds.y - min_bounds.y),
                                         0.5f * (max_bounds.z - min_bounds.z));
    UpeOBB_FromCenterExtents(obb, center, half_extents, rotation);
}

static UpeVec3 upe_vec3_add(UpeVec3 a, UpeVec3 b) { return UpeVec3_Add(a, b); }
static UpeVec3 upe_vec3_sub(UpeVec3 a, UpeVec3 b) { return UpeVec3_Sub(a, b); }
static UpeVec3 upe_vec3_scale(UpeVec3 v, float s) { return UpeVec3_Scale(v, s); }
static float upe_vec3_dot(UpeVec3 a, UpeVec3 b) { return UpeVec3_Dot(a, b); }
static UpeVec3 upe_vec3_cross(UpeVec3 a, UpeVec3 b) { return UpeVec3_Cross(a, b); }
static float upe_vec3_len(UpeVec3 v) { return UpeVec3_Length(v); }
static UpeVec3 upe_vec3_normalize(UpeVec3 v) { return UpeVec3_Normalize(v); }

void UpeOBB_FromPoints(UpeOBB *obb, const UpeVec3 *points, uint32_t point_count) {
    if (!obb || !points || point_count == 0) return;

    // Compute centroid
    UpeVec3 centroid = upe_vec3_make(0.0f, 0.0f, 0.0f);
    for (uint32_t i = 0; i < point_count; ++i) {
        centroid.x += points[i].x;
        centroid.y += points[i].y;
        centroid.z += points[i].z;
    }
    float inv_count = 1.0f / (float)point_count;
    centroid = upe_vec3_scale(centroid, inv_count);

    // Covariance matrix for PCA (approximate best-fit axes)
    float xx = 0.0f, xy = 0.0f, xz = 0.0f, yy = 0.0f, yz = 0.0f, zz = 0.0f;
    for (uint32_t i = 0; i < point_count; ++i) {
        UpeVec3 p = upe_vec3_sub(points[i], centroid);
        xx += p.x * p.x; xy += p.x * p.y; xz += p.x * p.z;
        yy += p.y * p.y; yz += p.y * p.z; zz += p.z * p.z;
    }

    // Power iteration for dominant axis
    UpeVec3 axis0 = upe_vec3_make(1.0f, 0.0f, 0.0f);
    for (int iter = 0; iter < 10; ++iter) {
        UpeVec3 v;
        v.x = xx * axis0.x + xy * axis0.y + xz * axis0.z;
        v.y = xy * axis0.x + yy * axis0.y + yz * axis0.z;
        v.z = xz * axis0.x + yz * axis0.y + zz * axis0.z;
        axis0 = upe_vec3_normalize(v);
    }

    // Choose an arbitrary vector not parallel to axis0 for axis1 seed
    UpeVec3 seed = (upe_absf(axis0.x) < 0.9f) ? upe_vec3_make(1.0f, 0.0f, 0.0f)
                                              : upe_vec3_make(0.0f, 1.0f, 0.0f);
    UpeVec3 axis2 = upe_vec3_normalize(upe_vec3_cross(axis0, seed));
    UpeVec3 axis1 = upe_vec3_normalize(upe_vec3_cross(axis2, axis0));

    // Project points to find extents along axes
    float min0 = FLT_MAX, max0 = -FLT_MAX;
    float min1 = FLT_MAX, max1 = -FLT_MAX;
    float min2 = FLT_MAX, max2 = -FLT_MAX;

    for (uint32_t i = 0; i < point_count; ++i) {
        UpeVec3 p = upe_vec3_sub(points[i], centroid);
        float d0 = upe_vec3_dot(p, axis0);
        float d1 = upe_vec3_dot(p, axis1);
        float d2 = upe_vec3_dot(p, axis2);
        min0 = upe_minf(min0, d0); max0 = upe_maxf(max0, d0);
        min1 = upe_minf(min1, d1); max1 = upe_maxf(max1, d1);
        min2 = upe_minf(min2, d2); max2 = upe_maxf(max2, d2);
    }

    // Center in world space
    UpeVec3 center = centroid;
    center = upe_vec3_add(center, upe_vec3_scale(axis0, 0.5f * (min0 + max0)));
    center = upe_vec3_add(center, upe_vec3_scale(axis1, 0.5f * (min1 + max1)));
    center = upe_vec3_add(center, upe_vec3_scale(axis2, 0.5f * (min2 + max2)));

    obb->center = center;
    obb->half_extents = upe_vec3_make(0.5f * (max0 - min0), 0.5f * (max1 - min1), 0.5f * (max2 - min2));

    // Build rotation from axes (columns)
    UpeMat3 basis;
    basis.m[0][0] = axis0.x; basis.m[0][1] = axis1.x; basis.m[0][2] = axis2.x;
    basis.m[1][0] = axis0.y; basis.m[1][1] = axis1.y; basis.m[1][2] = axis2.y;
    basis.m[2][0] = axis0.z; basis.m[2][1] = axis1.z; basis.m[2][2] = axis2.z;

    // Convert basis to quaternion (approximate)
    float trace = basis.m[0][0] + basis.m[1][1] + basis.m[2][2];
    UpeQuat q;
    if (trace > 0.0f) {
        float s = sqrtf(trace + 1.0f) * 2.0f;
        q.w = 0.25f * s;
        q.x = (basis.m[2][1] - basis.m[1][2]) / s;
        q.y = (basis.m[0][2] - basis.m[2][0]) / s;
        q.z = (basis.m[1][0] - basis.m[0][1]) / s;
    } else if (basis.m[0][0] > basis.m[1][1] && basis.m[0][0] > basis.m[2][2]) {
        float s = sqrtf(1.0f + basis.m[0][0] - basis.m[1][1] - basis.m[2][2]) * 2.0f;
        q.w = (basis.m[2][1] - basis.m[1][2]) / s;
        q.x = 0.25f * s;
        q.y = (basis.m[0][1] + basis.m[1][0]) / s;
        q.z = (basis.m[0][2] + basis.m[2][0]) / s;
    } else if (basis.m[1][1] > basis.m[2][2]) {
        float s = sqrtf(1.0f + basis.m[1][1] - basis.m[0][0] - basis.m[2][2]) * 2.0f;
        q.w = (basis.m[0][2] - basis.m[2][0]) / s;
        q.x = (basis.m[0][1] + basis.m[1][0]) / s;
        q.y = 0.25f * s;
        q.z = (basis.m[1][2] + basis.m[2][1]) / s;
    } else {
        float s = sqrtf(1.0f + basis.m[2][2] - basis.m[0][0] - basis.m[1][1]) * 2.0f;
        q.w = (basis.m[1][0] - basis.m[0][1]) / s;
        q.x = (basis.m[0][2] + basis.m[2][0]) / s;
        q.y = (basis.m[1][2] + basis.m[2][1]) / s;
        q.z = 0.25f * s;
    }
    obb->rotation = q;
    upe_obb_compute_axes(obb);
}

void UpeOBB_UpdateAxes(UpeOBB *obb) {
    if (!obb) return;
    upe_obb_compute_axes(obb);
}

int UpeOBB_ContainsPoint(const UpeOBB *obb, UpeVec3 point) {
    UpeVec3 d = upe_vec3_sub(point, obb->center);
    for (int i = 0; i < 3; ++i) {
        float dist = upe_vec3_dot(d, obb->axes[i]);
        if (upe_absf(dist) > obb->half_extents.x + UPE_OBB_EPS && i == 0) return 0;
        if (upe_absf(dist) > obb->half_extents.y + UPE_OBB_EPS && i == 1) return 0;
        if (upe_absf(dist) > obb->half_extents.z + UPE_OBB_EPS && i == 2) return 0;
    }
    return 1;
}

static float upe_project_extent(const UpeOBB *obb, UpeVec3 axis) {
    return upe_absf(upe_vec3_dot(axis, obb->axes[0])) * obb->half_extents.x +
           upe_absf(upe_vec3_dot(axis, obb->axes[1])) * obb->half_extents.y +
           upe_absf(upe_vec3_dot(axis, obb->axes[2])) * obb->half_extents.z;
}

UpeVec3 UpeOBB_ClosestPoint(const UpeOBB *obb, UpeVec3 point) {
    UpeVec3 d = upe_vec3_sub(point, obb->center);
    UpeVec3 q = obb->center;
    float proj;

    proj = upe_vec3_dot(d, obb->axes[0]);
    proj = upe_clampf(proj, -obb->half_extents.x, obb->half_extents.x);
    q = upe_vec3_add(q, upe_vec3_scale(obb->axes[0], proj));

    proj = upe_vec3_dot(d, obb->axes[1]);
    proj = upe_clampf(proj, -obb->half_extents.y, obb->half_extents.y);
    q = upe_vec3_add(q, upe_vec3_scale(obb->axes[1], proj));

    proj = upe_vec3_dot(d, obb->axes[2]);
    proj = upe_clampf(proj, -obb->half_extents.z, obb->half_extents.z);
    q = upe_vec3_add(q, upe_vec3_scale(obb->axes[2], proj));

    return q;
}

float UpeOBB_DistanceToPoint(const UpeOBB *obb, UpeVec3 point) {
    UpeVec3 cp = UpeOBB_ClosestPoint(obb, point);
    UpeVec3 diff = upe_vec3_sub(point, cp);
    return sqrtf(upe_vec3_dot(diff, diff));
}

void UpeOBB_ToAABB(const UpeOBB *obb, UpeVec3 *min_bounds, UpeVec3 *max_bounds) {
    UpeVec3 c = obb->center;
    UpeVec3 e;
    e.x = upe_absf(obb->axes[0].x) * obb->half_extents.x + upe_absf(obb->axes[1].x) * obb->half_extents.y + upe_absf(obb->axes[2].x) * obb->half_extents.z;
    e.y = upe_absf(obb->axes[0].y) * obb->half_extents.x + upe_absf(obb->axes[1].y) * obb->half_extents.y + upe_absf(obb->axes[2].y) * obb->half_extents.z;
    e.z = upe_absf(obb->axes[0].z) * obb->half_extents.x + upe_absf(obb->axes[1].z) * obb->half_extents.y + upe_absf(obb->axes[2].z) * obb->half_extents.z;
    if (min_bounds) *min_bounds = upe_vec3_sub(c, e);
    if (max_bounds) *max_bounds = upe_vec3_add(c, e);
}

int UpeOBB_TestSeparation(const UpeOBB *a, const UpeOBB *b, UpeVec3 axis, float *separation) {
    float len = upe_vec3_len(axis);
    if (len < UPE_OBB_EPS) {
        if (separation) *separation = 0.0f;
        return 0;
    }
    axis = upe_vec3_scale(axis, 1.0f / len);
    float proj_a = upe_project_extent(a, axis);
    float proj_b = upe_project_extent(b, axis);
    float dist = upe_absf(upe_vec3_dot(upe_vec3_sub(b->center, a->center), axis));
    float sep = dist - (proj_a + proj_b);
    if (separation) *separation = sep;
    return sep > 0.0f;
}

int UpeOBB_TestOBB(const UpeOBB *a, const UpeOBB *b, UpeOBBContact *contact) {
    float min_penetration = FLT_MAX;
    UpeVec3 best_axis = upe_vec3_make(1.0f, 0.0f, 0.0f);
    UpeVec3 T = upe_vec3_sub(b->center, a->center);

    // Test A's face normals
    for (int i = 0; i < 3; ++i) {
        float sep;
        if (UpeOBB_TestSeparation(a, b, a->axes[i], &sep)) {
            if (contact) contact->intersecting = 0;
            return 0;
        }
        if (-sep < min_penetration) {
            min_penetration = -sep;
            best_axis = a->axes[i];
        }
    }

    // Test B's face normals
    for (int j = 0; j < 3; ++j) {
        float sep;
        if (UpeOBB_TestSeparation(a, b, b->axes[j], &sep)) {
            if (contact) contact->intersecting = 0;
            return 0;
        }
        if (-sep < min_penetration) {
            min_penetration = -sep;
            best_axis = b->axes[j];
        }
    }

    // Test edge cross-product axes
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            UpeVec3 axis = upe_vec3_cross(a->axes[i], b->axes[j]);
            if (upe_vec3_len(axis) < UPE_OBB_EPS) continue;
            float sep;
            if (UpeOBB_TestSeparation(a, b, axis, &sep)) {
                if (contact) contact->intersecting = 0;
                return 0;
            }
            if (-sep < min_penetration) {
                min_penetration = -sep;
                best_axis = axis;
            }
        }
    }

    if (contact) {
        contact->intersecting = 1;
        if (upe_vec3_dot(best_axis, T) < 0.0f) best_axis = upe_vec3_scale(best_axis, -1.0f);
        contact->normal = best_axis;
        contact->penetration_depth = min_penetration;
        contact->contact_count = 0;
        UpeOBB_GenerateContacts(a, b, contact);
    }
    return 1;
}

static void upe_obb_compute_vertices(const UpeOBB *obb, UpeVec3 verts[8]) {
    UpeVec3 ex = upe_vec3_scale(obb->axes[0], obb->half_extents.x);
    UpeVec3 ey = upe_vec3_scale(obb->axes[1], obb->half_extents.y);
    UpeVec3 ez = upe_vec3_scale(obb->axes[2], obb->half_extents.z);

    int idx = 0;
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sy = -1; sy <= 1; sy += 2) {
            for (int sz = -1; sz <= 1; sz += 2) {
                UpeVec3 corner = obb->center;
                corner = upe_vec3_add(corner, upe_vec3_scale(ex, (float)sx));
                corner = upe_vec3_add(corner, upe_vec3_scale(ey, (float)sy));
                corner = upe_vec3_add(corner, upe_vec3_scale(ez, (float)sz));
                verts[idx++] = corner;
            }
        }
    }
}

static int upe_point_inside_obb(const UpeOBB *obb, UpeVec3 p, float threshold) {
    UpeVec3 d = upe_vec3_sub(p, obb->center);
    float px = upe_absf(upe_vec3_dot(d, obb->axes[0])) - obb->half_extents.x;
    float py = upe_absf(upe_vec3_dot(d, obb->axes[1])) - obb->half_extents.y;
    float pz = upe_absf(upe_vec3_dot(d, obb->axes[2])) - obb->half_extents.z;
    return (px <= threshold && py <= threshold && pz <= threshold);
}

void UpeOBB_GenerateContacts(const UpeOBB *a, const UpeOBB *b, UpeOBBContact *contact) {
    if (!contact) return;
    contact->contact_count = 0;

    UpeVec3 verts_a[8], verts_b[8];
    upe_obb_compute_vertices(a, verts_a);
    upe_obb_compute_vertices(b, verts_b);

    // Collect vertices from B inside A
    for (int i = 0; i < 8 && contact->contact_count < 8; ++i) {
        if (upe_point_inside_obb(a, verts_b[i], 1e-4f)) {
            contact->contact_points[contact->contact_count++] = verts_b[i];
        }
    }
    // Collect vertices from A inside B
    for (int i = 0; i < 8 && contact->contact_count < 8; ++i) {
        if (upe_point_inside_obb(b, verts_a[i], 1e-4f)) {
            contact->contact_points[contact->contact_count++] = verts_a[i];
        }
    }

    // Fallback: if no vertices captured (edge-edge), take midpoint of centers projected onto contact plane
    if (contact->contact_count == 0) {
        UpeVec3 mid = upe_vec3_scale(upe_vec3_add(a->center, b->center), 0.5f);
        contact->contact_points[0] = mid;
        contact->contact_count = 1;
    }
}

int UpeOBB_TestSphere(const UpeOBB *obb, UpeVec3 sphere_center, float sphere_radius, UpeOBBContact *contact) {
    UpeVec3 cp = UpeOBB_ClosestPoint(obb, sphere_center);
    UpeVec3 d = upe_vec3_sub(sphere_center, cp);
    float dist2 = upe_vec3_dot(d, d);
    if (dist2 > sphere_radius * sphere_radius) {
        if (contact) contact->intersecting = 0;
        return 0;
    }
    if (contact) {
        float dist = sqrtf(dist2);
        contact->intersecting = 1;
        contact->penetration_depth = sphere_radius - dist;
        contact->normal = (dist > UPE_OBB_EPS) ? upe_vec3_scale(d, 1.0f / dist) : obb->axes[0];
        contact->contact_points[0] = cp;
        contact->contact_count = 1;
    }
    return 1;
}

// Helper: closest point on segment AB to point P
static UpeVec3 upe_closest_point_on_segment(UpeVec3 a, UpeVec3 b, UpeVec3 p) {
    UpeVec3 ab = upe_vec3_sub(b, a);
    float t = upe_vec3_dot(upe_vec3_sub(p, a), ab) / upe_vec3_dot(ab, ab);
    t = upe_clampf(t, 0.0f, 1.0f);
    return upe_vec3_add(a, upe_vec3_scale(ab, t));
}

int UpeOBB_TestCapsule(const UpeOBB *obb, UpeVec3 capsule_a, UpeVec3 capsule_b,
                       float capsule_radius, UpeOBBContact *contact) {
    // Closest point between capsule segment and OBB
    // Sample via closest point on OBB to capsule segment
    const int samples = 4;
    UpeVec3 candidates[samples];
    candidates[0] = capsule_a;
    candidates[1] = capsule_b;
    candidates[2] = upe_vec3_add(capsule_a, upe_vec3_scale(upe_vec3_sub(capsule_b, capsule_a), 0.25f));
    candidates[3] = upe_vec3_add(capsule_a, upe_vec3_scale(upe_vec3_sub(capsule_b, capsule_a), 0.75f));

    float best_dist = FLT_MAX;
    UpeVec3 best_cp = obb->center;
    for (int i = 0; i < samples; ++i) {
        UpeVec3 cp = UpeOBB_ClosestPoint(obb, candidates[i]);
        float d2 = upe_vec3_dot(upe_vec3_sub(candidates[i], cp), upe_vec3_sub(candidates[i], cp));
        if (d2 < best_dist) { best_dist = d2; best_cp = cp; }
    }

    // Also check capsule vs OBB center projection
    UpeVec3 seg_cp = upe_closest_point_on_segment(capsule_a, capsule_b, best_cp);
    float dist2 = upe_vec3_dot(upe_vec3_sub(seg_cp, best_cp), upe_vec3_sub(seg_cp, best_cp));

    if (dist2 > capsule_radius * capsule_radius) {
        if (contact) contact->intersecting = 0;
        return 0;
    }

    if (contact) {
        float dist = sqrtf(dist2);
        contact->intersecting = 1;
        contact->penetration_depth = capsule_radius - dist;
        UpeVec3 normal = upe_vec3_sub(seg_cp, best_cp);
        if (upe_vec3_len(normal) < UPE_OBB_EPS) normal = obb->axes[0];
        else normal = upe_vec3_scale(normal, 1.0f / upe_vec3_len(normal));
        contact->normal = normal;
        contact->contact_points[0] = best_cp;
        contact->contact_count = 1;
    }
    return 1;
}

int UpeOBB_TestPlane(const UpeOBB *obb, UpeVec3 plane_normal, float plane_distance, UpeOBBContact *contact) {
    plane_normal = upe_vec3_normalize(plane_normal);
    float r = upe_project_extent(obb, plane_normal);
    float s = upe_vec3_dot(plane_normal, obb->center) - plane_distance;
    if (s > r) {
        if (contact) contact->intersecting = 0;
        return 0;
    }
    if (contact) {
        contact->intersecting = 1;
        contact->penetration_depth = r - s;
        contact->normal = plane_normal;
        contact->contact_points[0] = upe_vec3_sub(obb->center, upe_vec3_scale(plane_normal, s));
        contact->contact_count = 1;
    }
    return 1;
}

int UpeOBB_Raycast(const UpeOBB *obb, UpeVec3 ray_origin, UpeVec3 ray_direction,
                   float max_distance, float *hit_distance, UpeVec3 *hit_normal) {
    // Transform ray into OBB local space using transposed rotation matrix (axes are orthonormal)
    UpeVec3 p = upe_vec3_sub(ray_origin, obb->center);
    UpeVec3 o;
    o.x = upe_vec3_dot(p, obb->axes[0]);
    o.y = upe_vec3_dot(p, obb->axes[1]);
    o.z = upe_vec3_dot(p, obb->axes[2]);

    UpeVec3 d;
    d.x = upe_vec3_dot(ray_direction, obb->axes[0]);
    d.y = upe_vec3_dot(ray_direction, obb->axes[1]);
    d.z = upe_vec3_dot(ray_direction, obb->axes[2]);

    float tmin = 0.0f, tmax = max_distance;
    UpeVec3 normal = upe_vec3_make(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < 3; ++i) {
        float o_i = (&o.x)[i];
        float d_i = (&d.x)[i];
        float min_i = -(&obb->half_extents.x)[i];
        float max_i = (&obb->half_extents.x)[i];

        if (upe_absf(d_i) < UPE_OBB_EPS) {
            if (o_i < min_i || o_i > max_i) return 0; // Parallel and outside slab
        } else {
            float inv_d = 1.0f / d_i;
            float t1 = (min_i - o_i) * inv_d;
            float t2 = (max_i - o_i) * inv_d;
            float sign = -1.0f;
            if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; sign = 1.0f; }
            if (t1 > tmin) {
                tmin = t1;
                normal = obb->axes[i];
                normal = upe_vec3_scale(normal, sign);
            }
            if (t2 < tmax) tmax = t2;
            if (tmin > tmax) return 0;
        }
    }

    if (tmin < 0.0f) return 0;
    if (hit_distance) *hit_distance = tmin;
    if (hit_normal) *hit_normal = normal;
    return 1;
}

UpeVec3 UpeOBB_Support(const UpeOBB *obb, UpeVec3 direction) {
    // Support point in given direction
    UpeVec3 result = obb->center;
    for (int i = 0; i < 3; ++i) {
        float sign = (upe_vec3_dot(direction, obb->axes[i]) >= 0.0f) ? 1.0f : -1.0f;
        float extent = (&obb->half_extents.x)[i];
        result = upe_vec3_add(result, upe_vec3_scale(obb->axes[i], sign * extent));
    }
    return result;
}

float UpeOBB_Volume(const UpeOBB *obb) {
    float w = obb->half_extents.x * 2.0f;
    float h = obb->half_extents.y * 2.0f;
    float d = obb->half_extents.z * 2.0f;
    return w * h * d;
}

float UpeOBB_SurfaceArea(const UpeOBB *obb) {
    float w = obb->half_extents.x * 2.0f;
    float h = obb->half_extents.y * 2.0f;
    float d = obb->half_extents.z * 2.0f;
    return 2.0f * (w * h + w * d + h * d);
}

float UpeOBB_Mass(const UpeOBB *obb, float density) {
    return density * UpeOBB_Volume(obb);
}

static UpeMat3 upe_mat3_mul(const UpeMat3 *a, const UpeMat3 *b) {
    UpeMat3 r;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            r.m[i][j] = a->m[i][0] * b->m[0][j] + a->m[i][1] * b->m[1][j] + a->m[i][2] * b->m[2][j];
        }
    }
    return r;
}

UpeMat3 UpeOBB_InertiaTensor(const UpeOBB *obb, float mass) {
    float w = obb->half_extents.x * 2.0f;
    float h = obb->half_extents.y * 2.0f;
    float d = obb->half_extents.z * 2.0f;
    float factor = mass / 12.0f;

    UpeMat3 I_local = {{{0.0f}}};
    I_local.m[0][0] = factor * (h * h + d * d);
    I_local.m[1][1] = factor * (w * w + d * d);
    I_local.m[2][2] = factor * (w * w + h * h);

    // Rotation matrix from axes columns
    UpeMat3 R;
    R.m[0][0] = obb->axes[0].x; R.m[0][1] = obb->axes[1].x; R.m[0][2] = obb->axes[2].x;
    R.m[1][0] = obb->axes[0].y; R.m[1][1] = obb->axes[1].y; R.m[1][2] = obb->axes[2].y;
    R.m[2][0] = obb->axes[0].z; R.m[2][1] = obb->axes[1].z; R.m[2][2] = obb->axes[2].z;

    // I_world = R * I_local * R^T
    // Compute R * I_local
    UpeMat3 tmp = upe_mat3_mul(&R, &I_local);
    // Compute R^T
    UpeMat3 R_T;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            R_T.m[i][j] = R.m[j][i];

    return upe_mat3_mul(&tmp, &R_T);
}

void UpeOBB_Transform(UpeOBB *obb, UpeVec3 translation, UpeQuat rotation) {
    if (!obb) return;
    obb->center = upe_vec3_add(obb->center, translation);
    obb->rotation = UpeQuat_Multiply(rotation, obb->rotation);
    upe_obb_compute_axes(obb);
}

void UpeOBB_Merge(const UpeOBB *a, const UpeOBB *b, UpeOBB *result) {
    // Simple merge: convert to AABBs and merge, then build OBB aligned to AABB (cheap fallback)
    UpeVec3 min_a, max_a, min_b, max_b;
    UpeOBB_ToAABB(a, &min_a, &max_a);
    UpeOBB_ToAABB(b, &min_b, &max_b);
    UpeVec3 min = upe_vec3_make(upe_minf(min_a.x, min_b.x), upe_minf(min_a.y, min_b.y), upe_minf(min_a.z, min_b.z));
    UpeVec3 max = upe_vec3_make(upe_maxf(max_a.x, max_b.x), upe_maxf(max_a.y, max_b.y), upe_maxf(max_a.z, max_b.z));
    UpeOBB_FromAABB(result, min, max, UpeQuat_Identity());
}

void UpeOBB_DebugDraw(const UpeOBB *obb, void *debug_draw, float r, float g, float b) {
    (void)debug_draw; (void)r; (void)g; (void)b;
    // Hook into engine debug draw when available
}
