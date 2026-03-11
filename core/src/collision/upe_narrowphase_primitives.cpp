#include "upe/upe_narrowphase.h"
#include "upe/upe_heightfield.h"
#include "upe/upe_gjk.h"
#include "upe/upe_epa.h"
#include <math.h>
#include <float.h>

static UpeVec3 UpeNarrowphase_ConvexCentroid(const UpeNarrowphaseConvexShape *shape) {
    UpeVec3 centroid = {0.0f, 0.0f, 0.0f};
    if (!shape || !shape->vertices || shape->vertex_count == 0) {
        return centroid;
    }

    for (uint32_t i = 0; i < shape->vertex_count; ++i) {
        centroid = UpeVec3_Add(centroid, shape->vertices[i]);
    }
    return UpeVec3_Scale(centroid, 1.0f / (float)shape->vertex_count);
}

static UpeGjkSupport UpeNarrowphase_ConvexSupport(const void *shape_ptr, UpeVec3 direction) {
    UpeGjkSupport result = {};
    const UpeNarrowphaseConvexShape *shape = (const UpeNarrowphaseConvexShape *)shape_ptr;
    if (!shape || !shape->vertices || shape->vertex_count == 0) {
        result.point = {0.0f, 0.0f, 0.0f};
        return result;
    }

    float max_dot = -FLT_MAX;
    result.point = shape->vertices[0];
    for (uint32_t i = 0; i < shape->vertex_count; ++i) {
        float dot = UpeVec3_Dot(shape->vertices[i], direction);
        if (dot > max_dot) {
            max_dot = dot;
            result.point = shape->vertices[i];
        }
    }
    return result;
}

static UpeGjkSupport UpeNarrowphase_SphereSupport(const void *shape_ptr, UpeVec3 direction) {
    UpeGjkSupport result = {};
    const UpeNarrowphaseSphereShape *sphere = (const UpeNarrowphaseSphereShape *)shape_ptr;
    
    float dir_len = UpeVec3_Length(direction);
    if (dir_len < 1.0e-6f) {
        result.point = sphere->center;
    } else {
        UpeVec3 normalized_dir = UpeVec3_Scale(direction, 1.0f / dir_len);
        result.point = UpeVec3_Add(sphere->center, UpeVec3_Scale(normalized_dir, sphere->radius));
    }
    return result;
}

static UpeGjkSupport UpeNarrowphase_CapsuleSupport(const void *shape_ptr, UpeVec3 direction) {
    UpeGjkSupport result = {};
    const UpeNarrowphaseCapsuleShape *capsule = (const UpeNarrowphaseCapsuleShape *)shape_ptr;
    
    float dir_len = UpeVec3_Length(direction);
    if (dir_len < 1.0e-6f) {
        result.point = UpeVec3_Scale(UpeVec3_Add(capsule->a, capsule->b), 0.5f);
        return result;
    }
    
    UpeVec3 normalized_dir = UpeVec3_Scale(direction, 1.0f / dir_len);
    
    // Find the endpoint along the axis that points most in the direction
    float dot_a = UpeVec3_Dot(capsule->a, normalized_dir);
    float dot_b = UpeVec3_Dot(capsule->b, normalized_dir);
    
    UpeVec3 endpoint = dot_a > dot_b ? capsule->a : capsule->b;
    result.point = UpeVec3_Add(endpoint, UpeVec3_Scale(normalized_dir, capsule->radius));
    
    return result;
}

static UpeGjkSupport UpeNarrowphase_BoxSupport(const void *shape_ptr, UpeVec3 direction) {
    UpeGjkSupport result = {};
    const UpeAabb3 *box = (const UpeAabb3 *)shape_ptr;
    
    if (!box) {
        result.point = {0.0f, 0.0f, 0.0f};
        return result;
    }
    
    /* For each axis, pick the min or max depending on direction sign */
    result.point.x = direction.x > 0.0f ? box->max.x : box->min.x;
    result.point.y = direction.y > 0.0f ? box->max.y : box->min.y;
    result.point.z = direction.z > 0.0f ? box->max.z : box->min.z;
    
    return result;
}

static UpeGjkSupport UpeNarrowphase_ObbSupport(const void *shape_ptr, UpeVec3 direction) {
    UpeGjkSupport result = {};
    const UpeNarrowphaseObbShape *obb = (const UpeNarrowphaseObbShape *)shape_ptr;
    
    if (!obb) {
        result.point = {0.0f, 0.0f, 0.0f};
        return result;
    }
    
    /* OBB support: center + sum of (sign(dot(dir, axis)) * half_extent * axis) */
    UpeVec3 support = obb->center;
    
    /* X-axis */
    float dot_x = UpeVec3_Dot(direction, obb->axes[0]);
    support = UpeVec3_Add(support, UpeVec3_Scale(obb->axes[0], dot_x > 0.0f ? obb->half_extents.x : -obb->half_extents.x));
    
    /* Y-axis */
    float dot_y = UpeVec3_Dot(direction, obb->axes[1]);
    support = UpeVec3_Add(support, UpeVec3_Scale(obb->axes[1], dot_y > 0.0f ? obb->half_extents.y : -obb->half_extents.y));
    
    /* Z-axis */
    float dot_z = UpeVec3_Dot(direction, obb->axes[2]);
    support = UpeVec3_Add(support, UpeVec3_Scale(obb->axes[2], dot_z > 0.0f ? obb->half_extents.z : -obb->half_extents.z));
    
    result.point = support;
    return result;
}

static UpeVec3 UpeVec3_Lerp(UpeVec3 a, UpeVec3 b, float t) {
    return UpeVec3_Add(a, UpeVec3_Scale(UpeVec3_Sub(b, a), t));
}

static float UpeClamp01(float t) {
    if (t < 0.0f) {
        return 0.0f;
    }
    if (t > 1.0f) {
        return 1.0f;
    }
    return t;
}

static float UpeSegmentClosestPoints(UpeVec3 p1, UpeVec3 q1, UpeVec3 p2, UpeVec3 q2, UpeVec3 *c1, UpeVec3 *c2) {
    UpeVec3 d1 = UpeVec3_Sub(q1, p1);
    UpeVec3 d2 = UpeVec3_Sub(q2, p2);
    UpeVec3 r = UpeVec3_Sub(p1, p2);
    float a = UpeVec3_Dot(d1, d1);
    float e = UpeVec3_Dot(d2, d2);
    float f = UpeVec3_Dot(d2, r);

    float s = 0.0f;
    float t = 0.0f;
    if (a <= 1.0e-6f && e <= 1.0e-6f) {
        *c1 = p1;
        *c2 = p2;
        return UpeVec3_Length(UpeVec3_Sub(p1, p2));
    }
    if (a <= 1.0e-6f) {
        s = 0.0f;
        t = UpeClamp01(f / e);
    } else {
        float c = UpeVec3_Dot(d1, r);
        if (e <= 1.0e-6f) {
            t = 0.0f;
            s = UpeClamp01(-c / a);
        } else {
            float b = UpeVec3_Dot(d1, d2);
            float denom = a * e - b * b;
            if (denom != 0.0f) {
                s = UpeClamp01((b * f - c * e) / denom);
            }
            t = (b * s + f) / e;
            if (t < 0.0f) {
                t = 0.0f;
                s = UpeClamp01(-c / a);
            } else if (t > 1.0f) {
                t = 1.0f;
                s = UpeClamp01((b - c) / a);
            }
        }
    }

    *c1 = UpeVec3_Lerp(p1, q1, s);
    *c2 = UpeVec3_Lerp(p2, q2, t);
    return UpeVec3_Length(UpeVec3_Sub(*c1, *c2));
}

int UpeNarrowphase_AabbAabb(UpeAabb3 a, UpeAabb3 b, UpeContactManifold *manifold) {
    if (!UpeAabb3_Overlap(a, b)) {
        return 0;
    }
    if (manifold) {
        UpeContactManifold_Clear(manifold);
        
        // Compute overlap on each axis
        float overlap_x = (a.max.x < b.max.x ? a.max.x : b.max.x) - (a.min.x > b.min.x ? a.min.x : b.min.x);
        float overlap_y = (a.max.y < b.max.y ? a.max.y : b.max.y) - (a.min.y > b.min.y ? a.min.y : b.min.y);
        float overlap_z = (a.max.z < b.max.z ? a.max.z : b.max.z) - (a.min.z > b.min.z ? a.min.z : b.min.z);
        
        // Find axis of minimum penetration (separating axis with smallest overlap)
        UpeVec3 normal = {};
        float penetration = 0.0f;
        
        if (overlap_x < overlap_y && overlap_x < overlap_z) {
            // X axis has minimum penetration
            penetration = overlap_x;
            normal = (UpeAabb3_Center(a).x < UpeAabb3_Center(b).x) ? UpeVec3{1.0f, 0.0f, 0.0f} : UpeVec3{-1.0f, 0.0f, 0.0f};
        } else if (overlap_y < overlap_z) {
            // Y axis has minimum penetration
            penetration = overlap_y;
            normal = (UpeAabb3_Center(a).y < UpeAabb3_Center(b).y) ? UpeVec3{0.0f, 1.0f, 0.0f} : UpeVec3{0.0f, -1.0f, 0.0f};
        } else {
            // Z axis has minimum penetration
            penetration = overlap_z;
            normal = (UpeAabb3_Center(a).z < UpeAabb3_Center(b).z) ? UpeVec3{0.0f, 0.0f, 1.0f} : UpeVec3{0.0f, 0.0f, -1.0f};
        }
        
        UpeContactPoint point = {};
        point.normal = normal;
        point.penetration = penetration;
        // Contact point is at the midpoint of the overlap region
        UpeVec3 center_a = UpeAabb3_Center(a);
        UpeVec3 center_b = UpeAabb3_Center(b);
        point.position = UpeVec3_Scale(UpeVec3_Add(center_a, center_b), 0.5f);
        
        manifold->points[0] = point;
        manifold->count = 1;
    }
    return 1;
}

int UpeNarrowphase_SphereHeightfield(UpeSphere sphere, const UpeHeightfield *hf, UpeContactManifold *manifold) {
    if (!hf || !hf->heights || hf->rows == 0 || hf->cols == 0) {
        return 0;
    }
    
    float terrain_height = UpeHeightfield_SampleHeight(hf, sphere.center.x, sphere.center.z);
    float penetration = sphere.radius - (sphere.center.y - terrain_height);
    
    if (penetration <= 0.0f) {
        return 0;
    }
    
    if (manifold) {
        UpeContactManifold_Clear(manifold);
        UpeContactPoint point = {};
        point.normal = {0.0f, 1.0f, 0.0f};
        point.penetration = penetration;
        point.position = {sphere.center.x, terrain_height, sphere.center.z};
        manifold->points[0] = point;
        manifold->count = 1;
    }
    return 1;
}

int UpeNarrowphase_CapsuleHeightfield(UpeCapsule capsule, const UpeHeightfield *hf, UpeContactManifold *manifold) {
    if (!hf || !hf->heights || hf->rows == 0 || hf->cols == 0) {
        return 0;
    }
    
    // Sample at capsule endpoints
    float h_a = UpeHeightfield_SampleHeight(hf, capsule.a.x, capsule.a.z);
    float h_b = UpeHeightfield_SampleHeight(hf, capsule.b.x, capsule.b.z);
    
    float pen_a = capsule.radius - (capsule.a.y - h_a);
    float pen_b = capsule.radius - (capsule.b.y - h_b);
    
    if (pen_a <= 0.0f && pen_b <= 0.0f) {
        return 0;
    }
    
    if (manifold) {
        UpeContactManifold_Clear(manifold);
        if (pen_a > 0.0f) {
            UpeContactPoint point = {};
            point.normal = {0.0f, 1.0f, 0.0f};
            point.penetration = pen_a;
            point.position = {capsule.a.x, h_a, capsule.a.z};
            manifold->points[manifold->count++] = point;
        }
        if (pen_b > 0.0f && manifold->count < 4) {
            UpeContactPoint point = {};
            point.normal = {0.0f, 1.0f, 0.0f};
            point.penetration = pen_b;
            point.position = {capsule.b.x, h_b, capsule.b.z};
            manifold->points[manifold->count++] = point;
        }
    }
    return 1;
}

// Simple box-box (AABB) manifold with up to 2 contact points.
// This refines the overlap by emitting contacts on the minimum-penetration face center(s).
int UpeNarrowphase_BoxBox(UpeAabb3 a, UpeAabb3 b, UpeContactManifold *manifold) {
    if (!UpeAabb3_Overlap(a, b)) {
        return 0;
    }

    if (!manifold) {
        return 1;
    }

    UpeContactManifold_Clear(manifold);

    // Compute overlap on each axis
    float overlap_x = (a.max.x < b.max.x ? a.max.x : b.max.x) - (a.min.x > b.min.x ? a.min.x : b.min.x);
    float overlap_y = (a.max.y < b.max.y ? a.max.y : b.max.y) - (a.min.y > b.min.y ? a.min.y : b.min.y);
    float overlap_z = (a.max.z < b.max.z ? a.max.z : b.max.z) - (a.min.z > b.min.z ? a.min.z : b.min.z);

    // Pick axis of minimum penetration for normal
    UpeVec3 normal = {};
    float penetration = overlap_x;
    int axis = 0; // 0=x,1=y,2=z

    if (overlap_y < penetration) {
        penetration = overlap_y;
        axis = 1;
    }
    if (overlap_z < penetration) {
        penetration = overlap_z;
        axis = 2;
    }

    UpeVec3 center_a = UpeAabb3_Center(a);
    UpeVec3 center_b = UpeAabb3_Center(b);

    if (axis == 0) {
        normal = (center_a.x < center_b.x) ? UpeVec3{1.0f, 0.0f, 0.0f} : UpeVec3{-1.0f, 0.0f, 0.0f};
    } else if (axis == 1) {
        normal = (center_a.y < center_b.y) ? UpeVec3{0.0f, 1.0f, 0.0f} : UpeVec3{0.0f, -1.0f, 0.0f};
    } else {
        normal = (center_a.z < center_b.z) ? UpeVec3{0.0f, 0.0f, 1.0f} : UpeVec3{0.0f, 0.0f, -1.0f};
    }

    // Contact positions: project centers onto the contact plane and clamp to overlap region for stability.
    // This yields up to two points along the dominant overlapping edges.
    UpeVec3 overlap_min = { fmaxf(a.min.x, b.min.x), fmaxf(a.min.y, b.min.y), fmaxf(a.min.z, b.min.z) };
    UpeVec3 overlap_max = { fminf(a.max.x, b.max.x), fminf(a.max.y, b.max.y), fminf(a.max.z, b.max.z) };
    UpeVec3 contact_center = UpeVec3_Scale(UpeVec3_Add(overlap_min, overlap_max), 0.5f);

    // Emit one or two contacts depending on whether two axes tie for minimal penetration.
    manifold->count = 0;
    auto emit_contact = [&](UpeVec3 pos) {
        if (manifold->count >= 4) return;
        UpeContactPoint cp = {};
        cp.normal = normal;
        cp.penetration = penetration;
        cp.position = pos;
        manifold->points[manifold->count++] = cp;
    };

    emit_contact(contact_center);

    // Secondary contact if another axis is within 10% of min penetration for better stability on edges.
    float min_pen = penetration;
    if (fabsf(overlap_x - min_pen) < 0.1f * min_pen && axis != 0) {
        UpeVec3 pos = contact_center;
        pos.x = (center_a.x < center_b.x) ? overlap_max.x : overlap_min.x;
        emit_contact(pos);
    }
    if (fabsf(overlap_y - min_pen) < 0.1f * min_pen && axis != 1) {
        UpeVec3 pos = contact_center;
        pos.y = (center_a.y < center_b.y) ? overlap_max.y : overlap_min.y;
        emit_contact(pos);
    }
    if (fabsf(overlap_z - min_pen) < 0.1f * min_pen && axis != 2) {
        UpeVec3 pos = contact_center;
        pos.z = (center_a.z < center_b.z) ? overlap_max.z : overlap_min.z;
        emit_contact(pos);
    }

    if (manifold->count == 0) {
        UpeContactPoint cp = {};
        cp.normal = normal;
        cp.penetration = penetration;
        cp.position = contact_center;
        manifold->points[0] = cp;
        manifold->count = 1;
    }

    return 1;
}

int UpeNarrowphase_SphereSphere(UpeSphere a, UpeSphere b, UpeContactManifold *manifold) {
    UpeVec3 delta = UpeVec3_Sub(b.center, a.center);
    float dist = UpeVec3_Length(delta);
    float radius = a.radius + b.radius;
    if (dist > radius) {
        return 0;
    }
    if (manifold) {
        UpeContactManifold_Clear(manifold);
        UpeVec3 normal = dist > 1.0e-6f ? UpeVec3_Scale(delta, 1.0f / dist) : UpeVec3{1.0f, 0.0f, 0.0f};
        UpeContactPoint point = {};
        point.normal = normal;
        point.penetration = radius - dist;
        point.position = UpeVec3_Add(a.center, UpeVec3_Scale(normal, a.radius - 0.5f * point.penetration));
        manifold->points[0] = point;
        manifold->count = 1;
    }
    return 1;
}

int UpeNarrowphase_SphereCapsule(UpeSphere sphere, UpeCapsule capsule, UpeContactManifold *manifold) {
    UpeVec3 ab = UpeVec3_Sub(capsule.b, capsule.a);
    float ab_len2 = UpeVec3_Dot(ab, ab);
    float t = 0.0f;
    if (ab_len2 > 1.0e-6f) {
        t = UpeVec3_Dot(UpeVec3_Sub(sphere.center, capsule.a), ab) / ab_len2;
    }
    t = UpeClamp01(t);
    UpeVec3 closest = UpeVec3_Lerp(capsule.a, capsule.b, t);
    UpeVec3 delta = UpeVec3_Sub(sphere.center, closest);
    float dist = UpeVec3_Length(delta);
    float radius = sphere.radius + capsule.radius;
    if (dist > radius) {
        return 0;
    }
    if (manifold) {
        UpeContactManifold_Clear(manifold);
        UpeVec3 normal = dist > 1.0e-6f ? UpeVec3_Scale(delta, 1.0f / dist) : UpeVec3{1.0f, 0.0f, 0.0f};
        UpeContactPoint point = {};
        point.normal = normal;
        point.penetration = radius - dist;
        point.position = UpeVec3_Sub(sphere.center, UpeVec3_Scale(normal, sphere.radius - 0.5f * point.penetration));
        manifold->points[0] = point;
        manifold->count = 1;
    }
    return 1;
}

int UpeNarrowphase_CapsuleCapsule(UpeCapsule a, UpeCapsule b, UpeContactManifold *manifold) {
    UpeVec3 c1 = {};
    UpeVec3 c2 = {};
    float dist = UpeSegmentClosestPoints(a.a, a.b, b.a, b.b, &c1, &c2);
    float radius = a.radius + b.radius;
    if (dist > radius) {
        return 0;
    }
    if (manifold) {
        UpeContactManifold_Clear(manifold);
        UpeVec3 delta = UpeVec3_Sub(c2, c1);
        UpeVec3 normal = dist > 1.0e-6f ? UpeVec3_Scale(delta, 1.0f / dist) : UpeVec3{1.0f, 0.0f, 0.0f};
        UpeContactPoint point = {};
        point.normal = normal;
        point.penetration = radius - dist;
        point.position = UpeVec3_Add(c1, UpeVec3_Scale(normal, a.radius - 0.5f * point.penetration));
        manifold->points[0] = point;
        manifold->count = 1;
    }
    return 1;
}

int UpeNarrowphase_SphereAabb(UpeSphere sphere, UpeAabb3 box, UpeContactManifold *manifold) {
    // Find closest point on AABB to sphere center
    UpeVec3 closest = sphere.center;
    closest.x = closest.x < box.min.x ? box.min.x : (closest.x > box.max.x ? box.max.x : closest.x);
    closest.y = closest.y < box.min.y ? box.min.y : (closest.y > box.max.y ? box.max.y : closest.y);
    closest.z = closest.z < box.min.z ? box.min.z : (closest.z > box.max.z ? box.max.z : closest.z);
    
    UpeVec3 diff = UpeVec3_Sub(sphere.center, closest);
    float dist_sq = UpeVec3_Dot(diff, diff);
    float radius = sphere.radius;
    
    if (dist_sq > radius * radius) {
        return 0;
    }
    
    if (manifold) {
        UpeContactManifold_Clear(manifold);
        float dist = sqrtf(dist_sq);
        UpeVec3 normal = dist > 1.0e-6f ? UpeVec3_Scale(diff, 1.0f / dist) : UpeVec3{1.0f, 0.0f, 0.0f};
        
        UpeContactPoint point = {};
        point.normal = normal;
        point.penetration = radius - dist;
        point.position = closest;
        manifold->points[0] = point;
        manifold->count = 1;
    }
    return 1;
}

int UpeNarrowphase_CapsuleAabb(UpeCapsule capsule, UpeAabb3 box, UpeContactManifold *manifold) {
    // Find closest point on capsule axis to AABB
    UpeVec3 dir = UpeVec3_Sub(capsule.b, capsule.a);
    UpeVec3 box_center = UpeVec3_Scale(UpeVec3_Add(box.min, box.max), 0.5f);
    UpeVec3 to_box = UpeVec3_Sub(box_center, capsule.a);
    
    float dir_len_sq = UpeVec3_Dot(dir, dir);
    float t = dir_len_sq > 1.0e-6f ? UpeVec3_Dot(to_box, dir) / dir_len_sq : 0.0f;
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    
    UpeVec3 capsule_point = UpeVec3_Add(capsule.a, UpeVec3_Scale(dir, t));
    
    // Closest point on AABB to capsule point
    UpeVec3 closest = capsule_point;
    closest.x = closest.x < box.min.x ? box.min.x : (closest.x > box.max.x ? box.max.x : closest.x);
    closest.y = closest.y < box.min.y ? box.min.y : (closest.y > box.max.y ? box.max.y : closest.y);
    closest.z = closest.z < box.min.z ? box.min.z : (closest.z > box.max.z ? box.max.z : closest.z);
    
    UpeVec3 diff = UpeVec3_Sub(capsule_point, closest);
    float dist_sq = UpeVec3_Dot(diff, diff);
    float radius = capsule.radius;
    
    if (dist_sq > radius * radius) {
        return 0;
    }
    
    if (manifold) {
        UpeContactManifold_Clear(manifold);
        float dist = sqrtf(dist_sq);
        UpeVec3 normal = dist > 1.0e-6f ? UpeVec3_Scale(diff, 1.0f / dist) : UpeVec3{1.0f, 0.0f, 0.0f};
        
        UpeContactPoint point = {};
        point.normal = normal;
        point.penetration = radius - dist;
        point.position = closest;
        manifold->points[0] = point;
        manifold->count = 1;
    }
    return 1;
}

// Convex hull collision using GJK+EPA
int UpeNarrowphase_ConvexConvex(const void *shape_a, const void *shape_b, UpeContactManifold *manifold) {
    const UpeNarrowphaseConvexShape *convex_a = (const UpeNarrowphaseConvexShape *)shape_a;
    const UpeNarrowphaseConvexShape *convex_b = (const UpeNarrowphaseConvexShape *)shape_b;

    if (manifold) {
        UpeContactManifold_Clear(manifold);
    }

    if (!convex_a || !convex_b || !convex_a->vertices || !convex_b->vertices ||
        convex_a->vertex_count == 0 || convex_b->vertex_count == 0) {
        return 0;
    }

    UpeVec3 center_a = UpeNarrowphase_ConvexCentroid(convex_a);
    UpeVec3 center_b = UpeNarrowphase_ConvexCentroid(convex_b);
    UpeVec3 initial_dir = UpeVec3_Sub(center_b, center_a);
    if (UpeVec3_Length(initial_dir) < 1.0e-6f) {
        initial_dir = {1.0f, 0.0f, 0.0f};
    }

    UpeGjkResult gjk = UpeGjk_Distance(convex_a, UpeNarrowphase_ConvexSupport,
                                       convex_b, UpeNarrowphase_ConvexSupport,
                                       initial_dir);

    if (!gjk.intersecting && gjk.distance > 1.0e-4f) {
        return 0;
    }

    if (!manifold) {
        return 1;
    }

    UpeVec3 normal = UpeVec3_Sub(gjk.witness_b, gjk.witness_a);
    float normal_len = UpeVec3_Length(normal);
    float penetration = 0.0f;

    if (gjk.intersecting) {
        UpeEpaResult epa = UpeEpa_Penetration(convex_a, UpeNarrowphase_ConvexSupport,
                                              convex_b, UpeNarrowphase_ConvexSupport);
        if (epa.depth > 0.0f) {
            penetration = epa.depth;
        }
        normal = epa.normal;
        normal_len = UpeVec3_Length(normal);
    }

    if (normal_len < 1.0e-6f) {
        normal = UpeVec3_Sub(center_b, center_a);
        normal_len = UpeVec3_Length(normal);
        if (normal_len < 1.0e-6f) {
            normal = {0.0f, 1.0f, 0.0f};
            normal_len = 1.0f;
        }
    }
    normal = UpeVec3_Scale(normal, 1.0f / normal_len);

    UpeContactPoint point = {};
    point.normal = normal;
    point.penetration = penetration;
    point.position = UpeVec3_Scale(UpeVec3_Add(gjk.witness_a, gjk.witness_b), 0.5f);
    manifold->points[0] = point;
    manifold->count = 1;

    return 1;
}

int UpeNarrowphase_SphereConvex(const void *sphere_ptr, const void *convex_ptr, UpeContactManifold *manifold) {
    const UpeNarrowphaseSphereShape *sphere = (const UpeNarrowphaseSphereShape *)sphere_ptr;
    const UpeNarrowphaseConvexShape *convex = (const UpeNarrowphaseConvexShape *)convex_ptr;

    if (manifold) {
        UpeContactManifold_Clear(manifold);
    }

    if (!sphere || !convex || !convex->vertices || convex->vertex_count == 0) {
        return 0;
    }

    UpeVec3 center_convex = UpeNarrowphase_ConvexCentroid(convex);
    UpeVec3 initial_dir = UpeVec3_Sub(sphere->center, center_convex);
    if (UpeVec3_Length(initial_dir) < 1.0e-6f) {
        initial_dir = {1.0f, 0.0f, 0.0f};
    }

    UpeGjkResult gjk = UpeGjk_Distance(sphere, UpeNarrowphase_SphereSupport,
                                       convex, UpeNarrowphase_ConvexSupport,
                                       initial_dir);

    if (!gjk.intersecting && gjk.distance > 1.0e-4f) {
        return 0;
    }

    if (!manifold) {
        return 1;
    }

    UpeVec3 normal = UpeVec3_Sub(gjk.witness_b, gjk.witness_a);
    float normal_len = UpeVec3_Length(normal);
    float penetration = 0.0f;

    if (gjk.intersecting) {
        UpeEpaResult epa = UpeEpa_Penetration(sphere, UpeNarrowphase_SphereSupport,
                                              convex, UpeNarrowphase_ConvexSupport);
        if (epa.depth > 0.0f) {
            penetration = epa.depth;
        }
        normal = epa.normal;
        normal_len = UpeVec3_Length(normal);
    }

    if (normal_len < 1.0e-6f) {
        normal = UpeVec3_Sub(sphere->center, center_convex);
        normal_len = UpeVec3_Length(normal);
        if (normal_len < 1.0e-6f) {
            normal = {0.0f, 1.0f, 0.0f};
            normal_len = 1.0f;
        }
    }
    normal = UpeVec3_Scale(normal, 1.0f / normal_len);

    UpeContactPoint point = {};
    point.normal = normal;
    point.penetration = penetration;
    point.position = UpeVec3_Scale(UpeVec3_Add(gjk.witness_a, gjk.witness_b), 0.5f);
    manifold->points[0] = point;
    manifold->count = 1;

    return 1;
}

int UpeNarrowphase_CapsuleConvex(const void *capsule_ptr, const void *convex_ptr, UpeContactManifold *manifold) {
    const UpeNarrowphaseCapsuleShape *capsule = (const UpeNarrowphaseCapsuleShape *)capsule_ptr;
    const UpeNarrowphaseConvexShape *convex = (const UpeNarrowphaseConvexShape *)convex_ptr;

    if (manifold) {
        UpeContactManifold_Clear(manifold);
    }

    if (!capsule || !convex || !convex->vertices || convex->vertex_count == 0) {
        return 0;
    }

    UpeVec3 center_capsule = UpeVec3_Scale(UpeVec3_Add(capsule->a, capsule->b), 0.5f);
    UpeVec3 center_convex = UpeNarrowphase_ConvexCentroid(convex);
    UpeVec3 initial_dir = UpeVec3_Sub(center_capsule, center_convex);
    if (UpeVec3_Length(initial_dir) < 1.0e-6f) {
        initial_dir = {1.0f, 0.0f, 0.0f};
    }

    UpeGjkResult gjk = UpeGjk_Distance(capsule, UpeNarrowphase_CapsuleSupport,
                                       convex, UpeNarrowphase_ConvexSupport,
                                       initial_dir);

    if (!gjk.intersecting && gjk.distance > 1.0e-4f) {
        return 0;
    }

    if (!manifold) {
        return 1;
    }

    UpeVec3 normal = UpeVec3_Sub(gjk.witness_b, gjk.witness_a);
    float normal_len = UpeVec3_Length(normal);
    float penetration = 0.0f;

    if (gjk.intersecting) {
        UpeEpaResult epa = UpeEpa_Penetration(capsule, UpeNarrowphase_CapsuleSupport,
                                              convex, UpeNarrowphase_ConvexSupport);
        if (epa.depth > 0.0f) {
            penetration = epa.depth;
        }
        normal = epa.normal;
        normal_len = UpeVec3_Length(normal);
    }

    if (normal_len < 1.0e-6f) {
        normal = UpeVec3_Sub(center_capsule, center_convex);
        normal_len = UpeVec3_Length(normal);
        if (normal_len < 1.0e-6f) {
            normal = {0.0f, 1.0f, 0.0f};
            normal_len = 1.0f;
        }
    }
    normal = UpeVec3_Scale(normal, 1.0f / normal_len);

    UpeContactPoint point = {};
    point.normal = normal;
    point.penetration = penetration;
    point.position = UpeVec3_Scale(UpeVec3_Add(gjk.witness_a, gjk.witness_b), 0.5f);
    manifold->points[0] = point;
    manifold->count = 1;

    return 1;
}

int UpeNarrowphase_BoxConvex(const void *box_ptr, const void *convex_ptr, UpeContactManifold *manifold) {
    const UpeAabb3 *box = (const UpeAabb3 *)box_ptr;
    const UpeNarrowphaseConvexShape *convex = (const UpeNarrowphaseConvexShape *)convex_ptr;

    if (manifold) {
        UpeContactManifold_Clear(manifold);
    }

    if (!box || !convex || !convex->vertices || convex->vertex_count == 0) {
        return 0;
    }

    /* Box center and convex center */
    UpeVec3 box_center = {
        (box->min.x + box->max.x) * 0.5f,
        (box->min.y + box->max.y) * 0.5f,
        (box->min.z + box->max.z) * 0.5f
    };
    UpeVec3 center_convex = UpeNarrowphase_ConvexCentroid(convex);
    UpeVec3 initial_dir = UpeVec3_Sub(box_center, center_convex);
    if (UpeVec3_Length(initial_dir) < 1.0e-6f) {
        initial_dir = {1.0f, 0.0f, 0.0f};
    }

    /* Using GJK with box and convex support functions */
    UpeGjkResult gjk = UpeGjk_Distance(box, UpeNarrowphase_BoxSupport,
                                       convex, UpeNarrowphase_ConvexSupport,
                                       initial_dir);

    if (!gjk.intersecting && gjk.distance > 1.0e-4f) {
        return 0;
    }

    if (!manifold) {
        return 1;
    }

    UpeVec3 normal = UpeVec3_Sub(gjk.witness_b, gjk.witness_a);
    float normal_len = UpeVec3_Length(normal);
    float penetration = 0.0f;

    if (gjk.intersecting) {
        UpeEpaResult epa = UpeEpa_Penetration(box, UpeNarrowphase_BoxSupport,
                                              convex, UpeNarrowphase_ConvexSupport);
        if (epa.depth > 0.0f) {
            penetration = epa.depth;
        }
        normal = epa.normal;
        normal_len = UpeVec3_Length(normal);
    }

    if (normal_len < 1.0e-6f) {
        normal = UpeVec3_Sub(box_center, center_convex);
        normal_len = UpeVec3_Length(normal);
        if (normal_len < 1.0e-6f) {
            normal = {0.0f, 1.0f, 0.0f};
            normal_len = 1.0f;
        }
    }
    normal = UpeVec3_Scale(normal, 1.0f / normal_len);

    UpeContactPoint point = {};
    point.normal = normal;
    point.penetration = penetration;
    point.position = UpeVec3_Scale(UpeVec3_Add(gjk.witness_a, gjk.witness_b), 0.5f);
    manifold->points[0] = point;
    manifold->count = 1;

    return 1;
}

int UpeNarrowphase_ObbConvex(const void *obb_ptr, const void *convex_ptr, UpeContactManifold *manifold) {
    const UpeNarrowphaseObbShape *obb = (const UpeNarrowphaseObbShape *)obb_ptr;
    const UpeNarrowphaseConvexShape *convex = (const UpeNarrowphaseConvexShape *)convex_ptr;

    if (manifold) {
        UpeContactManifold_Clear(manifold);
    }

    if (!obb || !convex || !convex->vertices || convex->vertex_count == 0) {
        return 0;
    }

    UpeVec3 center_convex = UpeNarrowphase_ConvexCentroid(convex);
    UpeVec3 initial_dir = UpeVec3_Sub(obb->center, center_convex);
    if (UpeVec3_Length(initial_dir) < 1.0e-6f) {
        initial_dir = {1.0f, 0.0f, 0.0f};
    }

    /* Using GJK with OBB and convex support functions */
    UpeGjkResult gjk = UpeGjk_Distance(obb, UpeNarrowphase_ObbSupport,
                                       convex, UpeNarrowphase_ConvexSupport,
                                       initial_dir);

    if (!gjk.intersecting && gjk.distance > 1.0e-4f) {
        return 0;
    }

    if (!manifold) {
        return 1;
    }

    UpeVec3 normal = UpeVec3_Sub(gjk.witness_b, gjk.witness_a);
    float normal_len = UpeVec3_Length(normal);
    float penetration = 0.0f;

    if (gjk.intersecting) {
        UpeEpaResult epa = UpeEpa_Penetration(obb, UpeNarrowphase_ObbSupport,
                                              convex, UpeNarrowphase_ConvexSupport);
        if (epa.depth > 0.0f) {
            penetration = epa.depth;
        }
        normal = epa.normal;
        normal_len = UpeVec3_Length(normal);
    }

    if (normal_len < 1.0e-6f) {
        normal = UpeVec3_Sub(obb->center, center_convex);
        normal_len = UpeVec3_Length(normal);
        if (normal_len < 1.0e-6f) {
            normal = {0.0f, 1.0f, 0.0f};
            normal_len = 1.0f;
        }
    }
    normal = UpeVec3_Scale(normal, 1.0f / normal_len);

    UpeContactPoint point = {};
    point.normal = normal;
    point.penetration = penetration;
    point.position = UpeVec3_Scale(UpeVec3_Add(gjk.witness_a, gjk.witness_b), 0.5f);
    manifold->points[0] = point;
    manifold->count = 1;

    return 1;
}

static UpeGjkSupport UpeNarrowphase_MeshSupport(const void *shape_ptr, UpeVec3 direction) {
    const UpeNarrowphaseMeshShape *mesh = (const UpeNarrowphaseMeshShape *)shape_ptr;
    UpeGjkSupport result = {};

    if (!mesh || !mesh->vertices || mesh->vertex_count == 0) {
        return result;
    }

    /* Find vertex with maximum dot product with direction */
    float max_dot = -1.0e8f;
    UpeVec3 support = mesh->vertices[0];

    for (uint32_t i = 0; i < mesh->vertex_count; ++i) {
        float dot = mesh->vertices[i].x * direction.x +
                    mesh->vertices[i].y * direction.y +
                    mesh->vertices[i].z * direction.z;
        if (dot > max_dot) {
            max_dot = dot;
            support = mesh->vertices[i];
        }
    }

    result.point = support;
    return result;
}

int UpeNarrowphase_MeshConvex(const void *mesh_ptr, const void *convex_ptr, UpeContactManifold *manifold) {
    const UpeNarrowphaseMeshShape *mesh = (const UpeNarrowphaseMeshShape *)mesh_ptr;
    const UpeNarrowphaseConvexShape *convex = (const UpeNarrowphaseConvexShape *)convex_ptr;

    if (manifold) {
        UpeContactManifold_Clear(manifold);
    }

    if (!mesh || !mesh->vertices || mesh->vertex_count == 0 ||
        !convex || !convex->vertices || convex->vertex_count == 0) {
        return 0;
    }

    UpeVec3 center_convex = UpeNarrowphase_ConvexCentroid(convex);
    UpeVec3 initial_dir = UpeVec3_Sub(mesh->center, center_convex);
    if (UpeVec3_Length(initial_dir) < 1.0e-6f) {
        initial_dir = {1.0f, 0.0f, 0.0f};
    }

    /* Using GJK with mesh and convex support functions */
    UpeGjkResult gjk = UpeGjk_Distance(mesh, UpeNarrowphase_MeshSupport,
                                       convex, UpeNarrowphase_ConvexSupport,
                                       initial_dir);

    if (!gjk.intersecting && gjk.distance > 1.0e-4f) {
        return 0;
    }

    if (!manifold) {
        return 1;
    }

    UpeVec3 normal = UpeVec3_Sub(gjk.witness_b, gjk.witness_a);
    float normal_len = UpeVec3_Length(normal);
    float penetration = 0.0f;

    if (gjk.intersecting) {
        UpeEpaResult epa = UpeEpa_Penetration(mesh, UpeNarrowphase_MeshSupport,
                                              convex, UpeNarrowphase_ConvexSupport);
        if (epa.depth > 0.0f) {
            penetration = epa.depth;
        }
        normal = epa.normal;
        normal_len = UpeVec3_Length(normal);
    }

    if (normal_len < 1.0e-6f) {
        normal = UpeVec3_Sub(mesh->center, center_convex);
        normal_len = UpeVec3_Length(normal);
        if (normal_len < 1.0e-6f) {
            normal = {0.0f, 1.0f, 0.0f};
            normal_len = 1.0f;
        }
    }
    normal = UpeVec3_Scale(normal, 1.0f / normal_len);

    UpeContactPoint point = {};
    point.normal = normal;
    point.penetration = penetration;
    point.position = UpeVec3_Scale(UpeVec3_Add(gjk.witness_a, gjk.witness_b), 0.5f);
    manifold->points[0] = point;
    manifold->count = 1;

    return 1;
}
