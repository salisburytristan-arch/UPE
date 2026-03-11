#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Non-Euclidean Physics Approximations (Exotic/Game-specific — Phase 9)

  APPROXIMATION NOTE: True non-Euclidean geometry requires a curved metric tensor.
  This module provides *game-practical* approximations by wrapping/remapping
  coordinate systems, suitable for puzzle games (Antichamber-style spaces),
  portals, and infinite corridors.

  Mathematical basis:
    - Spherical geometry: geodesic distance via haversine, angle sum > π
    - Hyperbolic geometry: Poincaré disk model, ideal polygons
    - Coordinate wrap (torus topology) for "planet" worlds
    - Curved gravity direction (radially inward on a sphere surface)
    - Perception illusion: distance in remapped space vs. real engine space
*/

/* ---- Space topology descriptor ---- */
typedef enum UpeTopology {
    UPE_TOPOLOGY_FLAT      = 0,  /* Standard Euclidean (default) */
    UPE_TOPOLOGY_TORUS     = 1,  /* Periodic boundary in X/Z */
    UPE_TOPOLOGY_SPHERE    = 2,  /* Map to sphere surface, gravity inward */
    UPE_TOPOLOGY_HYPERBOLIC= 3,  /* Poincaré disk approximation */
    UPE_TOPOLOGY_KLEIN     = 4   /* Klein bottle (one-sided wrapping) */
} UpeTopology;

typedef struct UpeNonEuclidSpace {
    UpeTopology topology;
    float       scale_factor;   /* radius for sphere, disk radius for hyperbolic */
    UpeVec3     wrap_bounds;    /* half-extents for torus wrapping */
    float       curvature_K;    /* Gaussian curvature (K>0 spherical, K<0 hyperbolic) */
} UpeNonEuclidSpace;

/* --- Geodesic distance in the chosen geometry ---
   For flat: Euclidean distance.
   For torus: minimum image convention.
   For sphere: great-circle arc length. */
float UpeNonEuclid_GeodesicDistance(const UpeNonEuclidSpace *space,
                                     UpeVec3 a, UpeVec3 b);

/* --- Wrap position back into fundamental domain (torus) --- */
UpeVec3 UpeNonEuclid_WrapPosition(const UpeNonEuclidSpace *space, UpeVec3 pos);

/* --- Gravity direction in curved space ---
   For sphere topology: points toward center of sphere.
   For flat / torus: returns down vector unchanged. */
UpeVec3 UpeNonEuclid_GravityDirection(const UpeNonEuclidSpace *space,
                                       UpeVec3 object_pos);

/* --- Poincaré disk: Möbius transport of a point a by translation vector t ---
   Both in unit disk coordinates.  Returns new position in disk. */
UpeVec2 UpeNonEuclid_PoincareTranslate(UpeVec2 a_disk, UpeVec2 t_disk);

/* --- Sphere surface project: map from 3-D game world coords to sphere surface ---
   Useful when the "planet" has radius R and the player stands on it. */
UpeVec3 UpeNonEuclid_ProjectToSphere(UpeVec3 world_pos, float sphere_radius);

/* --- Angle sum of a spherical triangle (rad) ---
   Inputs: three interior angles (rad).  Returns excess = sum - π */
float UpeNonEuclid_SphericalExcess(float alpha, float beta, float gamma);

/* --- Angular size of an arc on a sphere ---
   chord_length / sphere_radius in small-angle limit → full formula. */
float UpeNonEuclid_SphericalAngularSize(float chord_m, float sphere_radius_m);

#ifdef __cplusplus
}
#endif
