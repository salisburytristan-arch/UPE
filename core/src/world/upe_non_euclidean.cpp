#include "upe/upe_non_euclidean.h"
#include <math.h>
#include <string.h>

#define UPE_PI 3.14159265358979f

/* -----------------------------------------------------------------------
   Geodesic distance
   ----------------------------------------------------------------------- */

float UpeNonEuclid_GeodesicDistance(const UpeNonEuclidSpace *space,
                                     UpeVec3 a, UpeVec3 b) {
    if (!space) {
        float dx = a.x-b.x, dy = a.y-b.y, dz = a.z-b.z;
        return sqrtf(dx*dx + dy*dy + dz*dz);
    }
    switch (space->topology) {
    case UPE_TOPOLOGY_TORUS: {
        float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
        /* Minimum image: wrap each axis */
        float wx = 2.0f * space->wrap_bounds.x;
        float wy = 2.0f * space->wrap_bounds.y;
        float wz = 2.0f * space->wrap_bounds.z;
        if (wx > 0.0f) { dx = fmodf(dx + wx*10.0f, wx); if (dx > wx*0.5f) dx -= wx; }
        if (wy > 0.0f) { dy = fmodf(dy + wy*10.0f, wy); if (dy > wy*0.5f) dy -= wy; }
        if (wz > 0.0f) { dz = fmodf(dz + wz*10.0f, wz); if (dz > wz*0.5f) dz -= wz; }
        return sqrtf(dx*dx + dy*dy + dz*dz);
    }
    case UPE_TOPOLOGY_SPHERE: {
        float R = space->scale_factor;
        if (R <= 0.0f) R = 1.0f;
        /* Project onto unit sphere, then great circle */
        float ra = sqrtf(a.x*a.x + a.y*a.y + a.z*a.z);
        float rb = sqrtf(b.x*b.x + b.y*b.y + b.z*b.z);
        if (ra < 1e-7f || rb < 1e-7f) return 0.0f;
        float dot = (a.x*b.x + a.y*b.y + a.z*b.z) / (ra * rb);
        if (dot >  1.0f) dot =  1.0f;
        if (dot < -1.0f) dot = -1.0f;
        return R * acosf(dot);
    }
    case UPE_TOPOLOGY_HYPERBOLIC:
    case UPE_TOPOLOGY_KLEIN:
    case UPE_TOPOLOGY_FLAT:
    default: {
        float dx = a.x-b.x, dy = a.y-b.y, dz = a.z-b.z;
        return sqrtf(dx*dx + dy*dy + dz*dz);
    }
    }
}

/* -----------------------------------------------------------------------
   Wrap position
   ----------------------------------------------------------------------- */

UpeVec3 UpeNonEuclid_WrapPosition(const UpeNonEuclidSpace *space, UpeVec3 pos) {
    if (!space || space->topology != UPE_TOPOLOGY_TORUS) return pos;
    UpeVec3 out = pos;
    float wx = 2.0f * space->wrap_bounds.x;
    float wy = 2.0f * space->wrap_bounds.y;
    float wz = 2.0f * space->wrap_bounds.z;
    if (wx > 0.0f) {
        out.x = fmodf(pos.x + wx * 1000.0f, wx);
        if (out.x > space->wrap_bounds.x) out.x -= wx;
    }
    if (wy > 0.0f) {
        out.y = fmodf(pos.y + wy * 1000.0f, wy);
        if (out.y > space->wrap_bounds.y) out.y -= wy;
    }
    if (wz > 0.0f) {
        out.z = fmodf(pos.z + wz * 1000.0f, wz);
        if (out.z > space->wrap_bounds.z) out.z -= wz;
    }
    return out;
}

/* -----------------------------------------------------------------------
   Gravity direction
   ----------------------------------------------------------------------- */

UpeVec3 UpeNonEuclid_GravityDirection(const UpeNonEuclidSpace *space,
                                       UpeVec3 object_pos) {
    UpeVec3 down = {0.0f, -1.0f, 0.0f};
    if (!space || space->topology != UPE_TOPOLOGY_SPHERE) return down;
    /* Sphere: gravity points toward origin (planet center) */
    float len = sqrtf(object_pos.x*object_pos.x + object_pos.y*object_pos.y + object_pos.z*object_pos.z);
    if (len < 1e-7f) return down;
    down.x = -object_pos.x / len;
    down.y = -object_pos.y / len;
    down.z = -object_pos.z / len;
    return down;
}

/* -----------------------------------------------------------------------
   Poincaré disk Möbius translation
   T_a(z) = (z + a) / (1 + conj(a)*z)  (complex Möbius)
   Using 2D vectors as complex numbers.
   ----------------------------------------------------------------------- */

UpeVec2 UpeNonEuclid_PoincareTranslate(UpeVec2 a_disk, UpeVec2 t_disk) {
    /* z = a_disk + t_disk (Möbius transform) */
    /* numerator: (a + t) */
    float num_x = a_disk.x + t_disk.x;
    float num_y = a_disk.y + t_disk.y;
    /* denominator: 1 + conj(t) * a = 1 + (tx - i*ty)*(ax + i*ay)
                                     = (1 + tx*ax + ty*ay) + i*(tx*ay - ty*ax) */
    float den_x = 1.0f + t_disk.x * a_disk.x + t_disk.y * a_disk.y;
    float den_y = t_disk.x * a_disk.y - t_disk.y * a_disk.x;
    float den_sq = den_x*den_x + den_y*den_y;
    if (den_sq < 1e-14f) { UpeVec2 r = {0.0f,0.0f}; return r; }
    UpeVec2 result;
    result.x = (num_x * den_x + num_y * den_y) / den_sq;
    result.y = (num_y * den_x - num_x * den_y) / den_sq;
    /* Clamp to disk */
    float mag = sqrtf(result.x*result.x + result.y*result.y);
    if (mag >= 1.0f) { float s = 0.9999f / mag; result.x *= s; result.y *= s; }
    return result;
}

UpeVec3 UpeNonEuclid_ProjectToSphere(UpeVec3 world_pos, float sphere_radius) {
    float len = sqrtf(world_pos.x*world_pos.x + world_pos.y*world_pos.y + world_pos.z*world_pos.z);
    if (len < 1e-7f) { UpeVec3 r = {0.0f, sphere_radius, 0.0f}; return r; }
    float s = sphere_radius / len;
    UpeVec3 r = { world_pos.x * s, world_pos.y * s, world_pos.z * s };
    return r;
}

float UpeNonEuclid_SphericalExcess(float alpha, float beta, float gamma) {
    return (alpha + beta + gamma) - UPE_PI;
}

float UpeNonEuclid_SphericalAngularSize(float chord_m, float sphere_radius_m) {
    if (sphere_radius_m <= 0.0f) return 0.0f;
    return 2.0f * asinf(chord_m / (2.0f * sphere_radius_m));
}
