#include "upe/upe_sdf.h"
#include <math.h>
#include <string.h>

static float sdf_max(float a, float b) { return a > b ? a : b; }
static float sdf_min(float a, float b) { return a < b ? a : b; }
static float sdf_clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

static float v3_len(UpeVec3 v) { return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z); }
static UpeVec3 v3_make(float x, float y, float z) { UpeVec3 r = {x, y, z}; return r; }
static UpeVec3 v3_abs(UpeVec3 v) { UpeVec3 r = {fabsf(v.x), fabsf(v.y), fabsf(v.z)}; return r; }
static UpeVec3 v3_sub(UpeVec3 a, UpeVec3 b) { UpeVec3 r = {a.x-b.x, a.y-b.y, a.z-b.z}; return r; }
static UpeVec3 v3_max0(UpeVec3 v) { UpeVec3 r = {sdf_max(v.x,0.f), sdf_max(v.y,0.f), sdf_max(v.z,0.f)}; return r; }

/* -----------------------------------------------------------------------
   Primitives
   ----------------------------------------------------------------------- */

float UpeSDF_Sphere(UpeVec3 p, UpeVec3 center, float radius) {
    return v3_len(v3_sub(p, center)) - radius;
}

float UpeSDF_Box(UpeVec3 p, UpeVec3 center, UpeVec3 half_extents) {
    UpeVec3 q = v3_abs(v3_sub(p, center));
    q = v3_sub(q, half_extents);
    UpeVec3 q0 = v3_max0(q);
    float outside = v3_len(q0);
    float inside  = sdf_min(sdf_max(q.x, sdf_max(q.y, q.z)), 0.0f);
    return outside + inside;
}

float UpeSDF_Capsule(UpeVec3 p, UpeVec3 a, UpeVec3 b, float radius) {
    /* Project p onto segment AB, clamp, subtract */
    UpeVec3 ab = v3_sub(b, a);
    UpeVec3 ap = v3_sub(p, a);
    float ab2 = ab.x*ab.x + ab.y*ab.y + ab.z*ab.z;
    if (ab2 < 1e-10f) return v3_len(ap) - radius;
    float t = sdf_clamp((ap.x*ab.x + ap.y*ab.y + ap.z*ab.z) / ab2, 0.0f, 1.0f);
    UpeVec3 closest = { a.x + ab.x*t, a.y + ab.y*t, a.z + ab.z*t };
    return v3_len(v3_sub(p, closest)) - radius;
}

float UpeSDF_Torus(UpeVec3 p, UpeVec3 center, float major_r, float minor_r) {
    UpeVec3 q = v3_sub(p, center);
    float xz = sqrtf(q.x*q.x + q.z*q.z) - major_r;
    return sqrtf(xz*xz + q.y*q.y) - minor_r;
}

float UpeSDF_Cylinder(UpeVec3 p, UpeVec3 center, float radius, float half_height) {
    UpeVec3 q = v3_sub(p, center);
    float dr = sqrtf(q.x*q.x + q.z*q.z) - radius;
    float dy = fabsf(q.y) - half_height;
    UpeVec3 outside = v3_make(sdf_max(dr, 0.0f), sdf_max(dy, 0.0f), 0.0f);
    return sdf_min(sdf_max(dr, dy), 0.0f) + v3_len(outside);
}

float UpeSDF_Cone(UpeVec3 p, UpeVec3 apex, UpeVec3 axis_dir, float half_angle_rad, float height) {
    UpeVec3 q = v3_sub(p, apex);
    float al = sqrtf(axis_dir.x*axis_dir.x + axis_dir.y*axis_dir.y + axis_dir.z*axis_dir.z);
    if (al < 1e-7f) return v3_len(q);
    float h = (q.x*axis_dir.x + q.y*axis_dir.y + q.z*axis_dir.z) / al;
    float r = sqrtf(q.x*q.x + q.y*q.y + q.z*q.z - h*h);
    /* SDF: check distance to cone surface and cap */
    float s = r - h * tanf(half_angle_rad);
    return sdf_max(s, h - height);
}

float UpeSDF_Plane(UpeVec3 p, UpeVec3 point_on_plane, UpeVec3 normal) {
    UpeVec3 d = v3_sub(p, point_on_plane);
    return d.x*normal.x + d.y*normal.y + d.z*normal.z;
}

float UpeSDF_Ellipsoid(UpeVec3 p, UpeVec3 center, UpeVec3 radii) {
    if (radii.x <= 0.0f || radii.y <= 0.0f || radii.z <= 0.0f) return 0.0f;
    UpeVec3 q = v3_sub(p, center);
    UpeVec3 s = { q.x/radii.x, q.y/radii.y, q.z/radii.z };
    float k1 = v3_len(s);
    UpeVec3 inv_r2 = v3_make(
        q.x / (radii.x * radii.x),
        q.y / (radii.y * radii.y),
        q.z / (radii.z * radii.z)
    );
    float k0 = v3_len(inv_r2);
    if (k0 < 1e-10f) return -sdf_min(sdf_min(radii.x,radii.y),radii.z);
    return k1 * (k1 - 1.0f) / k0;
}

/* -----------------------------------------------------------------------
   Boolean / blending
   ----------------------------------------------------------------------- */

float UpeSDF_Union(float d1, float d2)        { return sdf_min(d1, d2); }
float UpeSDF_Intersection(float d1, float d2) { return sdf_max(d1, d2); }
float UpeSDF_Subtraction(float d1, float d2)  { return sdf_max(d1, -d2); }

/* Polynomial smooth union (Inigo Quilez) */
float UpeSDF_SmoothUnion(float d1, float d2, float k) {
    float h = sdf_clamp(0.5f + 0.5f*(d2-d1)/k, 0.0f, 1.0f);
    return d2 + (d1 - d2) * h - k * h * (1.0f - h);
}
float UpeSDF_SmoothSubtraction(float d1, float d2, float k) {
    float h = sdf_clamp(0.5f - 0.5f*(d2+d1)/k, 0.0f, 1.0f);
    return d2 + (-d1 - d2) * h + k * h * (1.0f - h);
}
float UpeSDF_SmoothIntersection(float d1, float d2, float k) {
    float h = sdf_clamp(0.5f - 0.5f*(d2-d1)/k, 0.0f, 1.0f);
    return d2 + (d1 - d2) * h + k * h * (1.0f - h);
}

float UpeSDF_Onion(float d, float thickness) { return fabsf(d) - thickness; }
float UpeSDF_Round(float d, float radius)    { return d - radius; }

UpeVec3 UpeSDF_RepeatDomain(UpeVec3 p, UpeVec3 period) {
    UpeVec3 r;
    r.x = (period.x > 0.0f) ? fmodf(p.x + period.x*0.5f, period.x) - period.x*0.5f : p.x;
    r.y = (period.y > 0.0f) ? fmodf(p.y + period.y*0.5f, period.y) - period.y*0.5f : p.y;
    r.z = (period.z > 0.0f) ? fmodf(p.z + period.z*0.5f, period.z) - period.z*0.5f : p.z;
    return r;
}

/* -----------------------------------------------------------------------
   Gradient / normal via central differences
   ----------------------------------------------------------------------- */

UpeVec3 UpeSDF_EstimateNormal(UpeVec3 p, UpeSDF_Func func, void *user_data, float h) {
    UpeVec3 n;
    UpeVec3 pxp = v3_make(p.x + h, p.y, p.z);
    UpeVec3 pxm = v3_make(p.x - h, p.y, p.z);
    UpeVec3 pyp = v3_make(p.x, p.y + h, p.z);
    UpeVec3 pym = v3_make(p.x, p.y - h, p.z);
    UpeVec3 pzp = v3_make(p.x, p.y, p.z + h);
    UpeVec3 pzm = v3_make(p.x, p.y, p.z - h);
    n.x = func(pxp, user_data) - func(pxm, user_data);
    n.y = func(pyp, user_data) - func(pym, user_data);
    n.z = func(pzp, user_data) - func(pzm, user_data);
    float len = sqrtf(n.x*n.x + n.y*n.y + n.z*n.z);
    if (len < 1e-7f) { n.y = 1.0f; return n; }
    n.x /= len; n.y /= len; n.z /= len;
    return n;
}

/* -----------------------------------------------------------------------
   Grid sampler
   ----------------------------------------------------------------------- */

void UpeSDF_SampleGrid(float *grid, uint32_t nx, uint32_t ny, uint32_t nz,
                        UpeVec3 min_corner, UpeVec3 max_corner,
                        UpeSDF_Func func, void *user_data) {
    if (!grid || !func || nx == 0 || ny == 0 || nz == 0) return;
    float dx = (nx > 1) ? (max_corner.x - min_corner.x) / (float)(nx - 1) : 0.0f;
    float dy = (ny > 1) ? (max_corner.y - min_corner.y) / (float)(ny - 1) : 0.0f;
    float dz = (nz > 1) ? (max_corner.z - min_corner.z) / (float)(nz - 1) : 0.0f;
    for (uint32_t iz = 0; iz < nz; iz++) {
        for (uint32_t iy = 0; iy < ny; iy++) {
            for (uint32_t ix = 0; ix < nx; ix++) {
                UpeVec3 p = {
                    min_corner.x + ix * dx,
                    min_corner.y + iy * dy,
                    min_corner.z + iz * dz
                };
                grid[iz * ny * nx + iy * nx + ix] = func(p, user_data);
            }
        }
    }
}

/* -----------------------------------------------------------------------
   Sphere-trace raymarching
   ----------------------------------------------------------------------- */

float UpeSDF_Raymarch(UpeVec3 ray_origin, UpeVec3 ray_dir,
                       UpeSDF_Func func, void *user_data,
                       float max_dist, uint32_t max_steps) {
    if (!func) return -1.0f;
    /* Normalize direction */
    float dl = sqrtf(ray_dir.x*ray_dir.x + ray_dir.y*ray_dir.y + ray_dir.z*ray_dir.z);
    if (dl < 1e-7f) return -1.0f;
    ray_dir.x /= dl; ray_dir.y /= dl; ray_dir.z /= dl;

    float t = 0.0f;
    for (uint32_t i = 0; i < max_steps; i++) {
        UpeVec3 p = {
            ray_origin.x + ray_dir.x * t,
            ray_origin.y + ray_dir.y * t,
            ray_origin.z + ray_dir.z * t
        };
        float d = func(p, user_data);
        if (d < 1e-4f) return t;
        t += d;
        if (t >= max_dist) return -1.0f;
    }
    return -1.0f;
}
