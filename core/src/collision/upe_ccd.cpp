#include "upe/upe_ccd.h"
#include "upe/upe_trimesh.h"
#include <math.h>

UpeCcdResult UpeCcd_SphereSphere(UpeSphere a, UpeVec3 vel_a, UpeSphere b, UpeVec3 vel_b, float dt) {
    UpeCcdResult result;
    result.hit = 0;
    result.toi = 1.0f;
    result.position.x = 0; result.position.y = 0; result.position.z = 0;
    result.normal.x = 0; result.normal.y = 0; result.normal.z = 0;

    UpeVec3 rel_vel = UpeVec3_Sub(vel_b, vel_a);
    UpeVec3 delta = UpeVec3_Sub(b.center, a.center);
    float r = a.radius + b.radius;

    float a_coeff = UpeVec3_Dot(rel_vel, rel_vel);
    float b_coeff = 2.0f * UpeVec3_Dot(delta, rel_vel);
    float c_coeff = UpeVec3_Dot(delta, delta) - r * r;

    float discriminant = b_coeff * b_coeff - 4.0f * a_coeff * c_coeff;
    if (discriminant < 0.0f || a_coeff < 1.0e-6f) {
        return result;
    }

    float sqrt_disc = sqrtf(discriminant);
    float t = (-b_coeff - sqrt_disc) / (2.0f * a_coeff);

    if (t >= 0.0f && t <= dt) {
        result.hit = 1;
        result.toi = t;
        UpeVec3 pa = UpeVec3_Add(a.center, UpeVec3_Scale(vel_a, t));
        UpeVec3 pb = UpeVec3_Add(b.center, UpeVec3_Scale(vel_b, t));
        result.position = UpeVec3_Scale(UpeVec3_Add(pa, pb), 0.5f);
        UpeVec3 normal_dir = UpeVec3_Sub(pb, pa);
        float len = UpeVec3_Length(normal_dir);
        if (len > 1.0e-6f) {
            result.normal.x = normal_dir.x / len;
            result.normal.y = normal_dir.y / len;
            result.normal.z = normal_dir.z / len;
        } else {
            result.normal.x = 1.0f;
            result.normal.y = 0.0f;
            result.normal.z = 0.0f;
        }
    }

    return result;
}

UpeCcdResult UpeCcd_CapsuleCapsule(UpeCapsule a, UpeVec3 vel_a, UpeCapsule b, UpeVec3 vel_b, float dt) {
    UpeCcdResult result;
    result.hit = 0;
    result.toi = 1.0f;
    result.position.x = 0; result.position.y = 0; result.position.z = 0;
    result.normal.x = 0; result.normal.y = 0; result.normal.z = 0;
    
    // Conservative advancement: sample multiple time steps to find first contact
    const int num_samples = 32;
    float time_step = dt / (float)num_samples;
    
    for (int i = 0; i < num_samples; ++i) {
        float t = (float)i * time_step;
        
        // Advance capsules to time t
        UpeCapsule a_t = a;
        UpeCapsule b_t = b;
        a_t.a = UpeVec3_Add(a.a, UpeVec3_Scale(vel_a, t));
        a_t.b = UpeVec3_Add(a.b, UpeVec3_Scale(vel_a, t));
        b_t.a = UpeVec3_Add(b.a, UpeVec3_Scale(vel_b, t));
        b_t.b = UpeVec3_Add(b.b, UpeVec3_Scale(vel_b, t));
        
        // Find closest points between the two line segments
        UpeVec3 dir_a = UpeVec3_Sub(a_t.b, a_t.a);
        UpeVec3 dir_b = UpeVec3_Sub(b_t.b, b_t.a);
        UpeVec3 r = UpeVec3_Sub(a_t.a, b_t.a);
        
        float a_dot = UpeVec3_Dot(dir_a, dir_a);
        float b_dot = UpeVec3_Dot(dir_b, dir_b);
        float ab_dot = UpeVec3_Dot(dir_a, dir_b);
        float ar = UpeVec3_Dot(dir_a, r);
        float br = UpeVec3_Dot(dir_b, r);
        
        float denom = a_dot * b_dot - ab_dot * ab_dot;
        float s = 0.0f;
        float t_param = 0.0f;
        
        if (denom > 1.0e-6f) {
            s = (ab_dot * br - b_dot * ar) / denom;
            s = s < 0.0f ? 0.0f : (s > 1.0f ? 1.0f : s);
        }
        
        t_param = (ab_dot * s + br) / (b_dot > 1.0e-6f ? b_dot : 1.0f);
        t_param = t_param < 0.0f ? 0.0f : (t_param > 1.0f ? 1.0f : t_param);
        
        UpeVec3 p_a = UpeVec3_Add(a_t.a, UpeVec3_Scale(dir_a, s));
        UpeVec3 p_b = UpeVec3_Add(b_t.a, UpeVec3_Scale(dir_b, t_param));
        UpeVec3 diff = UpeVec3_Sub(p_b, p_a);
        float dist_sq = UpeVec3_Dot(diff, diff);
        
        float combined_radius = a.radius + b.radius;
        if (dist_sq <= combined_radius * combined_radius) {
            // Contact detected at time t
            result.hit = 1;
            result.toi = t;
            result.position = UpeVec3_Scale(UpeVec3_Add(p_a, p_b), 0.5f);
            
            float dist = sqrtf(dist_sq);
            if (dist > 1.0e-6f) {
                result.normal.x = diff.x / dist;
                result.normal.y = diff.y / dist;
                result.normal.z = diff.z / dist;
            } else {
                result.normal.x = 1.0f;
                result.normal.y = 0.0f;
                result.normal.z = 0.0f;
            }
            break;
        }
    }
    
    return result;
}

// Helper: closest point on line segment to point
static UpeVec3 ClosestPointOnSegment(UpeVec3 p, UpeVec3 a, UpeVec3 b) {
    UpeVec3 ab;
    ab.x = b.x - a.x;
    ab.y = b.y - a.y;
    ab.z = b.z - a.z;
    
    UpeVec3 ap;
    ap.x = p.x - a.x;
    ap.y = p.y - a.y;
    ap.z = p.z - a.z;
    
    float ab_dot = ab.x * ab.x + ab.y * ab.y + ab.z * ab.z;
    float t = (ap.x * ab.x + ap.y * ab.y + ap.z * ab.z) / (ab_dot > 1e-6f ? ab_dot : 1.0f);
    
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    
    UpeVec3 result;
    result.x = a.x + ab.x * t;
    result.y = a.y + ab.y * t;
    result.z = a.z + ab.z * t;
    return result;
}

UpeCcdResult UpeCcd_SphereCapsule(UpeSphere sphere, UpeVec3 vel_s, UpeCapsule capsule, UpeVec3 vel_c, float dt) {
    UpeCcdResult result;
    result.hit = 0;
    result.toi = dt;
    result.position.x = 0; result.position.y = 0; result.position.z = 0;
    result.normal.x = 0; result.normal.y = 0; result.normal.z = 0;
    
    const int num_samples = 32;
    float time_step = dt / (float)num_samples;
    
    for (int i = 0; i < num_samples; ++i) {
        float t = (float)i * time_step;
        
        // Advance shapes
        UpeVec3 sphere_center;
        sphere_center.x = sphere.center.x + vel_s.x * t;
        sphere_center.y = sphere.center.y + vel_s.y * t;
        sphere_center.z = sphere.center.z + vel_s.z * t;
        
        UpeVec3 cap_a;
        cap_a.x = capsule.a.x + vel_c.x * t;
        cap_a.y = capsule.a.y + vel_c.y * t;
        cap_a.z = capsule.a.z + vel_c.z * t;
        
        UpeVec3 cap_b;
        cap_b.x = capsule.b.x + vel_c.x * t;
        cap_b.y = capsule.b.y + vel_c.y * t;
        cap_b.z = capsule.b.z + vel_c.z * t;
        
        // Find closest point on capsule axis to sphere center
        UpeVec3 closest = ClosestPointOnSegment(sphere_center, cap_a, cap_b);
        
        UpeVec3 diff;
        diff.x = sphere_center.x - closest.x;
        diff.y = sphere_center.y - closest.y;
        diff.z = sphere_center.z - closest.z;
        
        float dist_sq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        float combined_radius = sphere.radius + capsule.radius;
        
        if (dist_sq <= combined_radius * combined_radius) {
            result.hit = 1;
            result.toi = t;
            result.position = closest;
            
            float dist = sqrtf(dist_sq);
            if (dist > 1e-6f) {
                result.normal.x = diff.x / dist;
                result.normal.y = diff.y / dist;
                result.normal.z = diff.z / dist;
            } else {
                result.normal.x = 0; result.normal.y = 1; result.normal.z = 0;
            }
            break;
        }
    }
    
    return result;
}

// Helper: closest point on AABB to point
static UpeVec3 ClosestPointOnAabb(UpeVec3 p, UpeAabb3 box) {
    UpeVec3 result;
    result.x = (p.x < box.min.x) ? box.min.x : ((p.x > box.max.x) ? box.max.x : p.x);
    result.y = (p.y < box.min.y) ? box.min.y : ((p.y > box.max.y) ? box.max.y : p.y);
    result.z = (p.z < box.min.z) ? box.min.z : ((p.z > box.max.z) ? box.max.z : p.z);
    return result;
}

UpeCcdResult UpeCcd_SphereBox(UpeSphere sphere, UpeVec3 vel_s, UpeAabb3 box, UpeVec3 vel_b, float dt) {
    UpeCcdResult result;
    result.hit = 0;
    result.toi = dt;
    result.position.x = 0; result.position.y = 0; result.position.z = 0;
    result.normal.x = 0; result.normal.y = 0; result.normal.z = 0;
    
    const int num_samples = 32;
    float time_step = dt / (float)num_samples;
    
    for (int i = 0; i < num_samples; ++i) {
        float t = (float)i * time_step;
        
        // Advance shapes
        UpeVec3 sphere_center;
        sphere_center.x = sphere.center.x + vel_s.x * t;
        sphere_center.y = sphere.center.y + vel_s.y * t;
        sphere_center.z = sphere.center.z + vel_s.z * t;
        
        UpeAabb3 box_t = box;
        box_t.min.x += vel_b.x * t;
        box_t.min.y += vel_b.y * t;
        box_t.min.z += vel_b.z * t;
        box_t.max.x += vel_b.x * t;
        box_t.max.y += vel_b.y * t;
        box_t.max.z += vel_b.z * t;
        
        // Find closest point on box to sphere center
        UpeVec3 closest = ClosestPointOnAabb(sphere_center, box_t);
        
        UpeVec3 diff;
        diff.x = sphere_center.x - closest.x;
        diff.y = sphere_center.y - closest.y;
        diff.z = sphere_center.z - closest.z;
        
        float dist_sq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        
        if (dist_sq <= sphere.radius * sphere.radius) {
            result.hit = 1;
            result.toi = t;
            result.position = closest;
            
            float dist = sqrtf(dist_sq);
            if (dist > 1e-6f) {
                result.normal.x = diff.x / dist;
                result.normal.y = diff.y / dist;
                result.normal.z = diff.z / dist;
            } else {
                result.normal.x = 0; result.normal.y = 1; result.normal.z = 0;
            }
            break;
        }
    }
    
    return result;
}

static int UpeAabb3_OverlapFast(UpeAabb3 a, UpeAabb3 b) {
    return !(a.max.x < b.min.x || a.min.x > b.max.x ||
             a.max.y < b.min.y || a.min.y > b.max.y ||
             a.max.z < b.min.z || a.min.z > b.max.z);
}

static UpeAabb3 UpeAabb3_Expand(UpeAabb3 box, float radius) {
    box.min.x -= radius;
    box.min.y -= radius;
    box.min.z -= radius;
    box.max.x += radius;
    box.max.y += radius;
    box.max.z += radius;
    return box;
}

static int SegmentIntersectsAabb(UpeVec3 p0, UpeVec3 p1, UpeAabb3 box, float *t_enter) {
    UpeVec3 d = UpeVec3_Sub(p1, p0);
    float tmin = 0.0f;
    float tmax = 1.0f;
    const float eps = 1.0e-8f;

    float p_vals[3] = {p0.x, p0.y, p0.z};
    float d_vals[3] = {d.x, d.y, d.z};
    float min_vals[3] = {box.min.x, box.min.y, box.min.z};
    float max_vals[3] = {box.max.x, box.max.y, box.max.z};

    for (int i = 0; i < 3; ++i) {
        float pi = p_vals[i];
        float di = d_vals[i];
        if (fabsf(di) < eps) {
            if (pi < min_vals[i] || pi > max_vals[i]) {
                return 0;
            }
            continue;
        }
        float inv_d = 1.0f / di;
        float t1 = (min_vals[i] - pi) * inv_d;
        float t2 = (max_vals[i] - pi) * inv_d;
        if (t1 > t2) {
            float tmp = t1;
            t1 = t2;
            t2 = tmp;
        }
        if (t1 > tmin) {
            tmin = t1;
        }
        if (t2 < tmax) {
            tmax = t2;
        }
        if (tmin > tmax) {
            return 0;
        }
    }

    if (t_enter) {
        *t_enter = tmin;
    }
    return 1;
}

UpeCcdResult UpeCcd_BoxBox(UpeAabb3 a, UpeVec3 vel_a, UpeAabb3 b, UpeVec3 vel_b, float dt) {
    UpeCcdResult result;
    result.hit = 0;
    result.toi = dt;
    result.position = {0.0f, 0.0f, 0.0f};
    result.normal = {0.0f, 0.0f, 0.0f};

    const int num_samples = 32;
    float time_step = dt / (float)num_samples;

    for (int i = 0; i <= num_samples; ++i) {
        float t = (float)i * time_step;

        UpeAabb3 a_t = a;
        UpeAabb3 b_t = b;
        a_t.min = UpeVec3_Add(a_t.min, UpeVec3_Scale(vel_a, t));
        a_t.max = UpeVec3_Add(a_t.max, UpeVec3_Scale(vel_a, t));
        b_t.min = UpeVec3_Add(b_t.min, UpeVec3_Scale(vel_b, t));
        b_t.max = UpeVec3_Add(b_t.max, UpeVec3_Scale(vel_b, t));

        if (UpeAabb3_OverlapFast(a_t, b_t)) {
            result.hit = 1;
            result.toi = t;
            UpeVec3 center_a = UpeVec3_Scale(UpeVec3_Add(a_t.min, a_t.max), 0.5f);
            UpeVec3 center_b = UpeVec3_Scale(UpeVec3_Add(b_t.min, b_t.max), 0.5f);
            UpeVec3 normal_dir = UpeVec3_Sub(center_b, center_a);
            float len = UpeVec3_Length(normal_dir);
            if (len > 1.0e-6f) {
                result.normal = UpeVec3_Scale(normal_dir, 1.0f / len);
            } else {
                result.normal = {1.0f, 0.0f, 0.0f};
            }
            result.position = UpeVec3_Scale(UpeVec3_Add(center_a, center_b), 0.5f);
            break;
        }
    }

    return result;
}

UpeCcdResult UpeCcd_CapsuleBox(UpeCapsule capsule, UpeVec3 vel_c, UpeAabb3 box, UpeVec3 vel_b, float dt) {
    UpeCcdResult result;
    result.hit = 0;
    result.toi = dt;
    result.position = {0.0f, 0.0f, 0.0f};
    result.normal = {0.0f, 0.0f, 0.0f};

    const int num_samples = 32;
    float time_step = dt / (float)num_samples;
    UpeAabb3 expanded = UpeAabb3_Expand(box, capsule.radius);

    for (int i = 0; i <= num_samples; ++i) {
        float t = (float)i * time_step;
        UpeCapsule cap_t = capsule;
        cap_t.a = UpeVec3_Add(cap_t.a, UpeVec3_Scale(vel_c, t));
        cap_t.b = UpeVec3_Add(cap_t.b, UpeVec3_Scale(vel_c, t));

        UpeAabb3 box_t = expanded;
        box_t.min = UpeVec3_Add(box_t.min, UpeVec3_Scale(vel_b, t));
        box_t.max = UpeVec3_Add(box_t.max, UpeVec3_Scale(vel_b, t));

        float t_enter = 0.0f;
        if (SegmentIntersectsAabb(cap_t.a, cap_t.b, box_t, &t_enter)) {
            result.hit = 1;
            result.toi = t;
            UpeVec3 seg_dir = UpeVec3_Sub(cap_t.b, cap_t.a);
            UpeVec3 contact = UpeVec3_Add(cap_t.a, UpeVec3_Scale(seg_dir, t_enter));
            result.position = contact;

            UpeVec3 closest = ClosestPointOnAabb(contact, box_t);
            UpeVec3 normal_dir = UpeVec3_Sub(contact, closest);
            float len = UpeVec3_Length(normal_dir);
            if (len > 1.0e-6f) {
                result.normal = UpeVec3_Scale(normal_dir, 1.0f / len);
            } else {
                result.normal = {1.0f, 0.0f, 0.0f};
            }
            break;
        }
    }

    return result;
}

// Helper: ray-triangle intersection (Möller-Trumbore algorithm)
[[maybe_unused]] static int RayTriangle(UpeVec3 origin, UpeVec3 dir, UpeVec3 v0, UpeVec3 v1, UpeVec3 v2, float *t, float *u, float *v) {
    UpeVec3 e1 = UpeVec3_Sub(v1, v0);
    UpeVec3 e2 = UpeVec3_Sub(v2, v0);
    UpeVec3 h = UpeVec3_Cross(dir, e2);
    float a = UpeVec3_Dot(e1, h);
    if (a > -1.0e-6f && a < 1.0e-6f) {
        return 0; // Ray parallel to triangle
    }
    float f = 1.0f / a;
    UpeVec3 s = UpeVec3_Sub(origin, v0);
    *u = f * UpeVec3_Dot(s, h);
    if (*u < 0.0f || *u > 1.0f) {
        return 0;
    }
    UpeVec3 q = UpeVec3_Cross(s, e1);
    *v = f * UpeVec3_Dot(dir, q);
    if (*v < 0.0f || *u + *v > 1.0f) {
        return 0;
    }
    *t = f * UpeVec3_Dot(e2, q);
    return *t > 1.0e-6f;
}

UpeCcdResult UpeCcd_SphereTriMesh(UpeSphere sphere, UpeVec3 vel_s, const UpeTriangleMesh *mesh, float dt) {
    UpeCcdResult result;
    result.hit = 0;
    result.toi = dt;
    result.position = {0.0f, 0.0f, 0.0f};
    result.normal = {0.0f, 0.0f, 0.0f};

    if (!mesh || mesh->index_count == 0) {
        return result;
    }

    const int num_samples = 32;
    float time_step = dt / (float)num_samples;
    uint32_t tri_count = mesh->index_count / 3;

    for (int i = 0; i <= num_samples; ++i) {
        float t = (float)i * time_step;
        UpeVec3 sphere_center = UpeVec3_Add(sphere.center, UpeVec3_Scale(vel_s, t));

        // Test sphere at time t against all triangles
        for (uint32_t tri = 0; tri < tri_count; ++tri) {
            uint32_t i0 = mesh->indices[tri * 3 + 0];
            uint32_t i1 = mesh->indices[tri * 3 + 1];
            uint32_t i2 = mesh->indices[tri * 3 + 2];
            UpeVec3 v0 = mesh->vertices[i0];
            UpeVec3 v1 = mesh->vertices[i1];
            UpeVec3 v2 = mesh->vertices[i2];

            // Compute closest point on triangle to sphere center
            // Simplified: test each vertex, edge, and face
            UpeVec3 e01 = UpeVec3_Sub(v1, v0);
            UpeVec3 e12 = UpeVec3_Sub(v2, v1);
            UpeVec3 e20 = UpeVec3_Sub(v0, v2);
            UpeVec3 n = UpeVec3_Cross(e01, UpeVec3_Sub(v2, v0));
            float n_len = UpeVec3_Length(n);
            if (n_len < 1.0e-6f) {
                continue; // Degenerate triangle
            }
            n = UpeVec3_Scale(n, 1.0f / n_len);

            // Project sphere center onto triangle plane
            UpeVec3 to_sphere = UpeVec3_Sub(sphere_center, v0);
            float dist_to_plane = UpeVec3_Dot(to_sphere, n);
            UpeVec3 projected = UpeVec3_Sub(sphere_center, UpeVec3_Scale(n, dist_to_plane));

            // Check if projection is inside triangle (barycentric)
            UpeVec3 c0 = UpeVec3_Cross(UpeVec3_Sub(projected, v0), e01);
            UpeVec3 c1 = UpeVec3_Cross(UpeVec3_Sub(projected, v1), e12);
            UpeVec3 c2 = UpeVec3_Cross(UpeVec3_Sub(projected, v2), e20);
            int inside = (UpeVec3_Dot(c0, n) >= 0.0f && UpeVec3_Dot(c1, n) >= 0.0f && UpeVec3_Dot(c2, n) >= 0.0f);

            UpeVec3 closest;
            if (inside) {
                closest = projected;
            } else {
                // Closest point on edges
                closest = v0;
                float min_dist_sq = UpeVec3_Dot(UpeVec3_Sub(sphere_center, v0), UpeVec3_Sub(sphere_center, v0));

                // Edge v0-v1
                UpeVec3 pt = ClosestPointOnSegment(sphere_center, v0, v1);
                float dist_sq = UpeVec3_Dot(UpeVec3_Sub(sphere_center, pt), UpeVec3_Sub(sphere_center, pt));
                if (dist_sq < min_dist_sq) {
                    min_dist_sq = dist_sq;
                    closest = pt;
                }

                // Edge v1-v2
                pt = ClosestPointOnSegment(sphere_center, v1, v2);
                dist_sq = UpeVec3_Dot(UpeVec3_Sub(sphere_center, pt), UpeVec3_Sub(sphere_center, pt));
                if (dist_sq < min_dist_sq) {
                    min_dist_sq = dist_sq;
                    closest = pt;
                }

                // Edge v2-v0
                pt = ClosestPointOnSegment(sphere_center, v2, v0);
                dist_sq = UpeVec3_Dot(UpeVec3_Sub(sphere_center, pt), UpeVec3_Sub(sphere_center, pt));
                if (dist_sq < min_dist_sq) {
                    closest = pt;
                }
            }

            UpeVec3 diff = UpeVec3_Sub(sphere_center, closest);
            float dist_sq = UpeVec3_Dot(diff, diff);
            if (dist_sq <= sphere.radius * sphere.radius) {
                // Collision at time t
                result.hit = 1;
                result.toi = t;
                result.position = closest;
                float dist = sqrtf(dist_sq);
                if (dist > 1.0e-6f) {
                    result.normal = UpeVec3_Scale(diff, 1.0f / dist);
                } else {
                    result.normal = n;
                }
                return result;
            }
        }
    }

    return result;
}

UpeCcdResult UpeCcd_CapsuleTriMesh(UpeCapsule capsule, UpeVec3 vel_c, const UpeTriangleMesh *mesh, float dt) {
    UpeCcdResult result;
    result.hit = 0;
    result.toi = dt;
    result.position = {0.0f, 0.0f, 0.0f};
    result.normal = {0.0f, 0.0f, 0.0f};

    if (!mesh || mesh->index_count == 0) {
        return result;
    }

    const int num_samples = 32;
    float time_step = dt / (float)num_samples;
    uint32_t tri_count = mesh->index_count / 3;

    for (int i = 0; i <= num_samples; ++i) {
        float t = (float)i * time_step;
        UpeCapsule cap_t = capsule;
        cap_t.a = UpeVec3_Add(cap_t.a, UpeVec3_Scale(vel_c, t));
        cap_t.b = UpeVec3_Add(cap_t.b, UpeVec3_Scale(vel_c, t));

        // Test capsule segment at time t against all triangles
        for (uint32_t tri = 0; tri < tri_count; ++tri) {
            uint32_t i0 = mesh->indices[tri * 3 + 0];
            uint32_t i1 = mesh->indices[tri * 3 + 1];
            uint32_t i2 = mesh->indices[tri * 3 + 2];
            UpeVec3 v0 = mesh->vertices[i0];
            UpeVec3 v1 = mesh->vertices[i1];
            UpeVec3 v2 = mesh->vertices[i2];

            // Compute triangle normal
            UpeVec3 e01 = UpeVec3_Sub(v1, v0);
            UpeVec3 e02 = UpeVec3_Sub(v2, v0);
            UpeVec3 n = UpeVec3_Cross(e01, e02);
            float n_len = UpeVec3_Length(n);
            if (n_len < 1.0e-6f) {
                continue;
            }
            n = UpeVec3_Scale(n, 1.0f / n_len);

            // Find closest point between capsule segment and triangle
            // Simplified: test capsule endpoints and midpoint as spheres
            UpeVec3 test_points[3] = {
                cap_t.a,
                cap_t.b,
                UpeVec3_Scale(UpeVec3_Add(cap_t.a, cap_t.b), 0.5f)
            };

            for (int p = 0; p < 3; ++p) {
                UpeVec3 pt = test_points[p];

                // Project point onto triangle plane
                UpeVec3 to_pt = UpeVec3_Sub(pt, v0);
                float dist_to_plane = UpeVec3_Dot(to_pt, n);
                UpeVec3 projected = UpeVec3_Sub(pt, UpeVec3_Scale(n, dist_to_plane));

                // Check if inside triangle
                UpeVec3 e12 = UpeVec3_Sub(v2, v1);
                UpeVec3 e20 = UpeVec3_Sub(v0, v2);
                UpeVec3 c0 = UpeVec3_Cross(UpeVec3_Sub(projected, v0), e01);
                UpeVec3 c1 = UpeVec3_Cross(UpeVec3_Sub(projected, v1), e12);
                UpeVec3 c2 = UpeVec3_Cross(UpeVec3_Sub(projected, v2), e20);
                int inside = (UpeVec3_Dot(c0, n) >= 0.0f && UpeVec3_Dot(c1, n) >= 0.0f && UpeVec3_Dot(c2, n) >= 0.0f);

                UpeVec3 closest;
                if (inside) {
                    closest = projected;
                } else {
                    // Closest on edges
                    closest = v0;
                    float min_dist_sq = UpeVec3_Dot(UpeVec3_Sub(pt, v0), UpeVec3_Sub(pt, v0));
                    UpeVec3 edge_pt = ClosestPointOnSegment(pt, v0, v1);
                    float dist_sq = UpeVec3_Dot(UpeVec3_Sub(pt, edge_pt), UpeVec3_Sub(pt, edge_pt));
                    if (dist_sq < min_dist_sq) {
                        min_dist_sq = dist_sq;
                        closest = edge_pt;
                    }
                    edge_pt = ClosestPointOnSegment(pt, v1, v2);
                    dist_sq = UpeVec3_Dot(UpeVec3_Sub(pt, edge_pt), UpeVec3_Sub(pt, edge_pt));
                    if (dist_sq < min_dist_sq) {
                        min_dist_sq = dist_sq;
                        closest = edge_pt;
                    }
                    edge_pt = ClosestPointOnSegment(pt, v2, v0);
                    dist_sq = UpeVec3_Dot(UpeVec3_Sub(pt, edge_pt), UpeVec3_Sub(pt, edge_pt));
                    if (dist_sq < min_dist_sq) {
                        closest = edge_pt;
                    }
                }

                UpeVec3 diff = UpeVec3_Sub(pt, closest);
                float dist_sq = UpeVec3_Dot(diff, diff);
                if (dist_sq <= capsule.radius * capsule.radius) {
                    result.hit = 1;
                    result.toi = t;
                    result.position = closest;
                    float dist = sqrtf(dist_sq);
                    if (dist > 1.0e-6f) {
                        result.normal = UpeVec3_Scale(diff, 1.0f / dist);
                    } else {
                        result.normal = n;
                    }
                    return result;
                }
            }
        }
    }

    return result;
}

