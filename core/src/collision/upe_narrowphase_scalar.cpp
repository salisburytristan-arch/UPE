#include "upe/upe_narrowphase_scalar.h"

static UpeScalar scalar_min(UpeScalar a, UpeScalar b) {
    return UpeScalar_Min(a, b);
}

static UpeScalar scalar_max(UpeScalar a, UpeScalar b) {
    return UpeScalar_Max(a, b);
}

static UpeScalar scalar_clamp(UpeScalar value, UpeScalar min_value, UpeScalar max_value) {
    return scalar_max(min_value, scalar_min(value, max_value));
}

static UpeScalar scalar_abs(UpeScalar value) {
    return UpeScalar_Cmp(value, UpeScalar_FromI32(0)) < 0 ? UpeScalar_Neg(value) : value;
}

static UpeScalarVec3 scalar_aabb_closest_point(UpeScalarAabb3 box, UpeScalarVec3 point) {
    UpeScalarVec3 out;
    out.x = scalar_clamp(point.x, box.min.x, box.max.x);
    out.y = scalar_clamp(point.y, box.min.y, box.max.y);
    out.z = scalar_clamp(point.z, box.min.z, box.max.z);
    return out;
}

static UpeScalar scalar_clamp01(UpeScalar value) {
    UpeScalar zero = UpeScalar_FromI32(0);
    UpeScalar one = UpeScalar_FromI32(1);
    return scalar_clamp(value, zero, one);
}

static UpeScalar scalar_segment_closest_points_sq(UpeScalarVec3 p1, UpeScalarVec3 q1,
                                                  UpeScalarVec3 p2, UpeScalarVec3 q2,
                                                  UpeScalarVec3 *c1, UpeScalarVec3 *c2) {
    UpeScalarVec3 d1 = UpeScalarVec3_Sub(q1, p1);
    UpeScalarVec3 d2 = UpeScalarVec3_Sub(q2, p2);
    UpeScalarVec3 r = UpeScalarVec3_Sub(p1, p2);
    UpeScalar a = UpeScalarVec3_Dot(d1, d1);
    UpeScalar e = UpeScalarVec3_Dot(d2, d2);
    UpeScalar f = UpeScalarVec3_Dot(d2, r);

    UpeScalar zero = UpeScalar_FromI32(0);
    UpeScalar s = zero;
    UpeScalar t = zero;

    if (UpeScalar_Cmp(a, zero) == 0 && UpeScalar_Cmp(e, zero) == 0) {
        *c1 = p1;
        *c2 = p2;
        UpeScalarVec3 d = UpeScalarVec3_Sub(*c1, *c2);
        return UpeScalarVec3_Dot(d, d);
    }

    if (UpeScalar_Cmp(a, zero) == 0) {
        s = zero;
        t = scalar_clamp01(UpeScalar_Div(f, e));
    } else {
        UpeScalar c = UpeScalarVec3_Dot(d1, r);
        if (UpeScalar_Cmp(e, zero) == 0) {
            t = zero;
            s = scalar_clamp01(UpeScalar_Div(UpeScalar_Neg(c), a));
        } else {
            UpeScalar b = UpeScalarVec3_Dot(d1, d2);
            UpeScalar denom = UpeScalar_Sub(UpeScalar_Mul(a, e), UpeScalar_Mul(b, b));
            if (UpeScalar_Cmp(denom, zero) != 0) {
                UpeScalar num = UpeScalar_Sub(UpeScalar_Mul(b, f), UpeScalar_Mul(c, e));
                s = scalar_clamp01(UpeScalar_Div(num, denom));
            }
            t = UpeScalar_Div(UpeScalar_Add(UpeScalar_Mul(b, s), f), e);
            if (UpeScalar_Cmp(t, zero) < 0) {
                t = zero;
                s = scalar_clamp01(UpeScalar_Div(UpeScalar_Neg(c), a));
            } else {
                UpeScalar one = UpeScalar_FromI32(1);
                if (UpeScalar_Cmp(t, one) > 0) {
                    t = one;
                    s = scalar_clamp01(UpeScalar_Div(UpeScalar_Sub(b, c), a));
                }
            }
        }
    }

    *c1 = UpeScalarVec3_Add(p1, UpeScalarVec3_Scale(d1, s));
    *c2 = UpeScalarVec3_Add(p2, UpeScalarVec3_Scale(d2, t));
    UpeScalarVec3 d = UpeScalarVec3_Sub(*c1, *c2);
    return UpeScalarVec3_Dot(d, d);
}

static UpeScalarVec3 scalar_segment_closest_point(UpeScalarVec3 a, UpeScalarVec3 b, UpeScalarVec3 p) {
    UpeScalarVec3 ab = UpeScalarVec3_Sub(b, a);
    UpeScalar ab_len_sq = UpeScalarVec3_Dot(ab, ab);
    UpeScalar zero = UpeScalar_FromI32(0);

    UpeScalar t = zero;
    if (UpeScalar_Cmp(ab_len_sq, zero) > 0) {
        t = UpeScalar_Div(UpeScalarVec3_Dot(UpeScalarVec3_Sub(p, a), ab), ab_len_sq);
    }
    t = scalar_clamp01(t);
    return UpeScalarVec3_Add(a, UpeScalarVec3_Scale(ab, t));
}

static UpeScalarVec3 scalar_dominant_axis_normal(UpeScalarVec3 delta) {
    UpeScalar abs_x = scalar_abs(delta.x);
    UpeScalar abs_y = scalar_abs(delta.y);
    UpeScalar abs_z = scalar_abs(delta.z);

    UpeScalar zero = UpeScalar_FromI32(0);
    UpeScalar one = UpeScalar_FromI32(1);
    UpeScalar neg_one = UpeScalar_Neg(one);

    UpeScalarVec3 normal = {zero, zero, zero};
    if (UpeScalar_Cmp(abs_x, abs_y) >= 0 && UpeScalar_Cmp(abs_x, abs_z) >= 0) {
        normal.x = UpeScalar_Cmp(delta.x, zero) >= 0 ? one : neg_one;
    } else if (UpeScalar_Cmp(abs_y, abs_z) >= 0) {
        normal.y = UpeScalar_Cmp(delta.y, zero) >= 0 ? one : neg_one;
    } else {
        normal.z = UpeScalar_Cmp(delta.z, zero) >= 0 ? one : neg_one;
    }
    return normal;
}

int UpeNarrowphaseScalar_AabbAabb(UpeScalarAabb3 a, UpeScalarAabb3 b, UpeScalarContactManifold *manifold) {
    if (!UpeScalarAabb3_Overlap(a, b)) {
        return 0;
    }

    if (!manifold) {
        return 1;
    }

    UpeScalarContactManifold_Clear(manifold);

    UpeScalar overlap_x = UpeScalar_Sub(scalar_min(a.max.x, b.max.x), scalar_max(a.min.x, b.min.x));
    UpeScalar overlap_y = UpeScalar_Sub(scalar_min(a.max.y, b.max.y), scalar_max(a.min.y, b.min.y));
    UpeScalar overlap_z = UpeScalar_Sub(scalar_min(a.max.z, b.max.z), scalar_max(a.min.z, b.min.z));

    UpeScalar penetration = overlap_x;
    int axis = 0;

    if (UpeScalar_Cmp(overlap_y, penetration) < 0) {
        penetration = overlap_y;
        axis = 1;
    }
    if (UpeScalar_Cmp(overlap_z, penetration) < 0) {
        penetration = overlap_z;
        axis = 2;
    }

    UpeScalarVec3 center_a = UpeScalarAabb3_Center(a);
    UpeScalarVec3 center_b = UpeScalarAabb3_Center(b);

    UpeScalar zero = UpeScalar_FromI32(0);
    UpeScalar one = UpeScalar_FromI32(1);
    UpeScalar neg_one = UpeScalar_Neg(one);

    UpeScalarContactPoint point = {};
    point.penetration = penetration;

    if (axis == 0) {
        point.normal = {
            UpeScalar_Cmp(center_a.x, center_b.x) < 0 ? one : neg_one,
            zero,
            zero
        };
    } else if (axis == 1) {
        point.normal = {
            zero,
            UpeScalar_Cmp(center_a.y, center_b.y) < 0 ? one : neg_one,
            zero
        };
    } else {
        point.normal = {
            zero,
            zero,
            UpeScalar_Cmp(center_a.z, center_b.z) < 0 ? one : neg_one
        };
    }

    point.position = UpeScalarVec3_Scale(UpeScalarVec3_Add(center_a, center_b), UpeScalar_Half());
    manifold->points[0] = point;
    manifold->count = 1;

    return 1;
}

int UpeNarrowphaseScalar_SphereSphereOverlap(UpeScalarSphere a, UpeScalarSphere b) {
    UpeScalarVec3 delta = UpeScalarVec3_Sub(b.center, a.center);
    UpeScalar dist_sq = UpeScalarVec3_Dot(delta, delta);
    UpeScalar radius_sum = UpeScalar_Add(a.radius, b.radius);
    UpeScalar radius_sq = UpeScalar_Mul(radius_sum, radius_sum);
    return UpeScalar_Cmp(dist_sq, radius_sq) <= 0;
}

int UpeNarrowphaseScalar_SphereSphere(UpeScalarSphere a, UpeScalarSphere b, UpeScalarContactManifold *manifold) {
    if (!UpeNarrowphaseScalar_SphereSphereOverlap(a, b)) {
        return 0;
    }

    if (!manifold) {
        return 1;
    }

    UpeScalarContactManifold_Clear(manifold);

    UpeScalarVec3 delta = UpeScalarVec3_Sub(b.center, a.center);
    UpeScalar abs_x = scalar_abs(delta.x);
    UpeScalar abs_y = scalar_abs(delta.y);
    UpeScalar abs_z = scalar_abs(delta.z);

    UpeScalar zero = UpeScalar_FromI32(0);
    UpeScalar one = UpeScalar_FromI32(1);
    UpeScalar neg_one = UpeScalar_Neg(one);

    UpeScalarContactPoint point = {};
    point.normal = {zero, zero, zero};

    UpeScalar dominant = abs_x;
    int axis = 0;
    if (UpeScalar_Cmp(abs_y, dominant) > 0) {
        dominant = abs_y;
        axis = 1;
    }
    if (UpeScalar_Cmp(abs_z, dominant) > 0) {
        dominant = abs_z;
        axis = 2;
    }

    if (axis == 0) {
        point.normal.x = UpeScalar_Cmp(delta.x, zero) >= 0 ? one : neg_one;
    } else if (axis == 1) {
        point.normal.y = UpeScalar_Cmp(delta.y, zero) >= 0 ? one : neg_one;
    } else {
        point.normal.z = UpeScalar_Cmp(delta.z, zero) >= 0 ? one : neg_one;
    }

    UpeScalar radius_sum = UpeScalar_Add(a.radius, b.radius);
    point.penetration = UpeScalar_Sub(radius_sum, dominant);
    point.position = UpeScalarVec3_Add(
        a.center,
        UpeScalarVec3_Scale(point.normal, UpeScalar_Sub(a.radius, UpeScalar_Mul(UpeScalar_Half(), point.penetration)))
    );

    manifold->points[0] = point;
    manifold->count = 1;
    return 1;
}

int UpeNarrowphaseScalar_SphereCapsuleOverlap(UpeScalarSphere sphere, UpeScalarCapsule capsule) {
    UpeScalarVec3 closest = scalar_segment_closest_point(capsule.a, capsule.b, sphere.center);
    UpeScalarVec3 delta = UpeScalarVec3_Sub(sphere.center, closest);
    UpeScalar dist_sq = UpeScalarVec3_Dot(delta, delta);
    UpeScalar radius = UpeScalar_Add(sphere.radius, capsule.radius);
    UpeScalar radius_sq = UpeScalar_Mul(radius, radius);
    return UpeScalar_Cmp(dist_sq, radius_sq) <= 0;
}

int UpeNarrowphaseScalar_SphereCapsule(UpeScalarSphere sphere, UpeScalarCapsule capsule, UpeScalarContactManifold *manifold) {
    if (!UpeNarrowphaseScalar_SphereCapsuleOverlap(sphere, capsule)) {
        return 0;
    }

    if (!manifold) {
        return 1;
    }

    UpeScalarContactManifold_Clear(manifold);

    UpeScalarVec3 closest = scalar_segment_closest_point(capsule.a, capsule.b, sphere.center);
    UpeScalarVec3 delta = UpeScalarVec3_Sub(sphere.center, closest);
    UpeScalarVec3 normal = scalar_dominant_axis_normal(delta);

    UpeScalar abs_x = scalar_abs(delta.x);
    UpeScalar abs_y = scalar_abs(delta.y);
    UpeScalar abs_z = scalar_abs(delta.z);
    UpeScalar dominant = abs_x;
    if (UpeScalar_Cmp(abs_y, dominant) > 0) {
        dominant = abs_y;
    }
    if (UpeScalar_Cmp(abs_z, dominant) > 0) {
        dominant = abs_z;
    }

    UpeScalar radius = UpeScalar_Add(sphere.radius, capsule.radius);
    UpeScalar penetration = UpeScalar_Sub(radius, dominant);

    UpeScalarContactPoint point = {};
    point.normal = normal;
    point.penetration = penetration;
    point.position = UpeScalarVec3_Sub(
        sphere.center,
        UpeScalarVec3_Scale(normal, UpeScalar_Sub(sphere.radius, UpeScalar_Mul(UpeScalar_Half(), penetration)))
    );

    manifold->points[0] = point;
    manifold->count = 1;
    return 1;
}

int UpeNarrowphaseScalar_SphereAabbOverlap(UpeScalarSphere sphere, UpeScalarAabb3 box) {
    UpeScalarVec3 closest = scalar_aabb_closest_point(box, sphere.center);

    UpeScalarVec3 diff = UpeScalarVec3_Sub(sphere.center, closest);
    UpeScalar dist_sq = UpeScalarVec3_Dot(diff, diff);
    UpeScalar radius_sq = UpeScalar_Mul(sphere.radius, sphere.radius);

    return UpeScalar_Cmp(dist_sq, radius_sq) <= 0;
}

int UpeNarrowphaseScalar_SphereAabb(UpeScalarSphere sphere, UpeScalarAabb3 box, UpeScalarContactManifold *manifold) {
    if (!UpeNarrowphaseScalar_SphereAabbOverlap(sphere, box)) {
        return 0;
    }

    if (!manifold) {
        return 1;
    }

    UpeScalarContactManifold_Clear(manifold);

    UpeScalarVec3 closest = scalar_aabb_closest_point(box, sphere.center);
    UpeScalarVec3 diff = UpeScalarVec3_Sub(sphere.center, closest);

    UpeScalar abs_x = scalar_abs(diff.x);
    UpeScalar abs_y = scalar_abs(diff.y);
    UpeScalar abs_z = scalar_abs(diff.z);

    UpeScalar zero = UpeScalar_FromI32(0);
    UpeScalar one = UpeScalar_FromI32(1);
    UpeScalar neg_one = UpeScalar_Neg(one);

    UpeScalarContactPoint point = {};
    point.position = closest;

    if (UpeScalar_Cmp(abs_x, zero) == 0 && UpeScalar_Cmp(abs_y, zero) == 0 && UpeScalar_Cmp(abs_z, zero) == 0) {
        UpeScalar dx_min = UpeScalar_Sub(sphere.center.x, box.min.x);
        UpeScalar dx_max = UpeScalar_Sub(box.max.x, sphere.center.x);
        UpeScalar dy_min = UpeScalar_Sub(sphere.center.y, box.min.y);
        UpeScalar dy_max = UpeScalar_Sub(box.max.y, sphere.center.y);
        UpeScalar dz_min = UpeScalar_Sub(sphere.center.z, box.min.z);
        UpeScalar dz_max = UpeScalar_Sub(box.max.z, sphere.center.z);

        UpeScalar min_face = dx_min;
        int axis = 0;
        int dir = -1;

        if (UpeScalar_Cmp(dx_max, min_face) < 0) { min_face = dx_max; axis = 0; dir = 1; }
        if (UpeScalar_Cmp(dy_min, min_face) < 0) { min_face = dy_min; axis = 1; dir = -1; }
        if (UpeScalar_Cmp(dy_max, min_face) < 0) { min_face = dy_max; axis = 1; dir = 1; }
        if (UpeScalar_Cmp(dz_min, min_face) < 0) { min_face = dz_min; axis = 2; dir = -1; }
        if (UpeScalar_Cmp(dz_max, min_face) < 0) { min_face = dz_max; axis = 2; dir = 1; }

        point.penetration = UpeScalar_Add(sphere.radius, min_face);
        point.normal = {zero, zero, zero};
        if (axis == 0) point.normal.x = dir < 0 ? neg_one : one;
        if (axis == 1) point.normal.y = dir < 0 ? neg_one : one;
        if (axis == 2) point.normal.z = dir < 0 ? neg_one : one;
    } else {
        point.penetration = sphere.radius;

        point.normal = {zero, zero, zero};
        if (UpeScalar_Cmp(abs_x, abs_y) >= 0 && UpeScalar_Cmp(abs_x, abs_z) >= 0) {
            point.normal.x = UpeScalar_Cmp(diff.x, zero) >= 0 ? one : neg_one;
            point.penetration = UpeScalar_Sub(sphere.radius, abs_x);
        } else if (UpeScalar_Cmp(abs_y, abs_z) >= 0) {
            point.normal.y = UpeScalar_Cmp(diff.y, zero) >= 0 ? one : neg_one;
            point.penetration = UpeScalar_Sub(sphere.radius, abs_y);
        } else {
            point.normal.z = UpeScalar_Cmp(diff.z, zero) >= 0 ? one : neg_one;
            point.penetration = UpeScalar_Sub(sphere.radius, abs_z);
        }
    }

    manifold->points[0] = point;
    manifold->count = 1;
    return 1;
}

int UpeNarrowphaseScalar_CapsuleAabbOverlap(UpeScalarCapsule capsule, UpeScalarAabb3 box) {
    UpeScalarVec3 dir = UpeScalarVec3_Sub(capsule.b, capsule.a);
    UpeScalarVec3 box_center = UpeScalarAabb3_Center(box);
    UpeScalarVec3 to_box = UpeScalarVec3_Sub(box_center, capsule.a);

    UpeScalar dir_len_sq = UpeScalarVec3_Dot(dir, dir);
    UpeScalar zero = UpeScalar_FromI32(0);
    UpeScalar one = UpeScalar_FromI32(1);

    UpeScalar t = zero;
    if (UpeScalar_Cmp(dir_len_sq, zero) > 0) {
        t = UpeScalar_Div(UpeScalarVec3_Dot(to_box, dir), dir_len_sq);
    }
    t = scalar_clamp(t, zero, one);

    UpeScalarVec3 capsule_point = UpeScalarVec3_Add(capsule.a, UpeScalarVec3_Scale(dir, t));
    UpeScalarVec3 closest = scalar_aabb_closest_point(box, capsule_point);
    UpeScalarVec3 diff = UpeScalarVec3_Sub(capsule_point, closest);
    UpeScalar dist_sq = UpeScalarVec3_Dot(diff, diff);
    UpeScalar radius_sq = UpeScalar_Mul(capsule.radius, capsule.radius);

    return UpeScalar_Cmp(dist_sq, radius_sq) <= 0;
}

int UpeNarrowphaseScalar_CapsuleCapsuleOverlap(UpeScalarCapsule a, UpeScalarCapsule b) {
    UpeScalarVec3 c1 = {};
    UpeScalarVec3 c2 = {};
    UpeScalar dist_sq = scalar_segment_closest_points_sq(a.a, a.b, b.a, b.b, &c1, &c2);
    UpeScalar radius = UpeScalar_Add(a.radius, b.radius);
    UpeScalar radius_sq = UpeScalar_Mul(radius, radius);
    return UpeScalar_Cmp(dist_sq, radius_sq) <= 0;
}

int UpeNarrowphaseScalar_CapsuleCapsule(UpeScalarCapsule a, UpeScalarCapsule b, UpeScalarContactManifold *manifold) {
    if (!UpeNarrowphaseScalar_CapsuleCapsuleOverlap(a, b)) {
        return 0;
    }

    if (!manifold) {
        return 1;
    }

    UpeScalarContactManifold_Clear(manifold);

    UpeScalarVec3 c1 = {};
    UpeScalarVec3 c2 = {};
    (void)scalar_segment_closest_points_sq(a.a, a.b, b.a, b.b, &c1, &c2);

    UpeScalarVec3 delta = UpeScalarVec3_Sub(c2, c1);
    UpeScalarVec3 normal = scalar_dominant_axis_normal(delta);

    UpeScalar abs_x = scalar_abs(delta.x);
    UpeScalar abs_y = scalar_abs(delta.y);
    UpeScalar abs_z = scalar_abs(delta.z);
    UpeScalar dominant = abs_x;
    if (UpeScalar_Cmp(abs_y, dominant) > 0) {
        dominant = abs_y;
    }
    if (UpeScalar_Cmp(abs_z, dominant) > 0) {
        dominant = abs_z;
    }

    UpeScalar radius = UpeScalar_Add(a.radius, b.radius);
    UpeScalar penetration = UpeScalar_Sub(radius, dominant);

    UpeScalarContactPoint point = {};
    point.normal = normal;
    point.penetration = penetration;
    point.position = UpeScalarVec3_Add(
        c1,
        UpeScalarVec3_Scale(normal, UpeScalar_Sub(a.radius, UpeScalar_Mul(UpeScalar_Half(), penetration)))
    );

    manifold->points[0] = point;
    manifold->count = 1;
    return 1;
}
