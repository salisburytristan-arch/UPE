#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Signed Distance Field (SDF) Utilities (Math/CS — Phase 10)

  Pure math functions for SDF primitives, boolean operations,
  and gradient estimation.  No allocations — callers own grids.

  SDFs are fundamental for:
    - Soft-body collision (implicit body contact)
    - Procedural terrain carving
    - Fluid surface reconstruction
    - Smooth blending of shapes (soft min / union)
    - Distance queries without explicit geometry
*/

/* ---- Primitive SDFs (return signed distance; negative = inside) ---- */

float UpeSDF_Sphere(UpeVec3 p, UpeVec3 center, float radius);
float UpeSDF_Box(UpeVec3 p, UpeVec3 center, UpeVec3 half_extents);
float UpeSDF_Capsule(UpeVec3 p, UpeVec3 a, UpeVec3 b, float radius);
float UpeSDF_Torus(UpeVec3 p, UpeVec3 center, float major_r, float minor_r);
float UpeSDF_Cylinder(UpeVec3 p, UpeVec3 center, float radius, float half_height);
float UpeSDF_Cone(UpeVec3 p, UpeVec3 apex, UpeVec3 axis_dir, float half_angle_rad, float height);
float UpeSDF_Plane(UpeVec3 p, UpeVec3 point_on_plane, UpeVec3 normal);
float UpeSDF_Ellipsoid(UpeVec3 p, UpeVec3 center, UpeVec3 radii);

/* ---- Boolean / blending operations ---- */

float UpeSDF_Union(float d1, float d2);
float UpeSDF_Intersection(float d1, float d2);
float UpeSDF_Subtraction(float d1, float d2);
float UpeSDF_SmoothUnion(float d1, float d2, float k);        /* polynomial smooth union */
float UpeSDF_SmoothSubtraction(float d1, float d2, float k);
float UpeSDF_SmoothIntersection(float d1, float d2, float k);

/* ---- Domain transforms ---- */

float UpeSDF_Onion(float d, float thickness);   /* hollow shell at surface */
float UpeSDF_Round(float d, float radius);       /* rounded offset */
UpeVec3 UpeSDF_RepeatDomain(UpeVec3 p, UpeVec3 period);    /* infinite repeat */

/* ---- Gradient (normal) estimation via central differences ---
   h: step size (typically 0.0001)
   Func and user_data are the SDF function pointer and its data. */
typedef float (*UpeSDF_Func)(UpeVec3 p, void *user_data);
UpeVec3 UpeSDF_EstimateNormal(UpeVec3 p, UpeSDF_Func func, void *user_data, float h);

/* ---- Grid sampler: fill a 3-D float grid from an SDF function ---
   min_corner/max_corner: world bounds.  grid: flat (nx*ny*nz) array (caller allocates). */
void UpeSDF_SampleGrid(float *grid, uint32_t nx, uint32_t ny, uint32_t nz,
                        UpeVec3 min_corner, UpeVec3 max_corner,
                        UpeSDF_Func func, void *user_data);

/* ---- Raymarching: find first surface hit along ray ---
   Returns distance along ray, or -1 if no hit within max_dist.
   max_steps: cap march iterations (default quality: 128). */
float UpeSDF_Raymarch(UpeVec3 ray_origin, UpeVec3 ray_dir,
                       UpeSDF_Func func, void *user_data,
                       float max_dist, uint32_t max_steps);

#ifdef __cplusplus
}
#endif
