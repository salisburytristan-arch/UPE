#pragma once
/* upe_noise.h — Procedural noise functions.
   Implementations in core/src/math/upe_noise.cpp.
   All functions are pure C, no external dependencies.
*/

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* -----------------------------------------------------------------------
   Low-level permutation table (seeded)
   Call UpeNoise_Seed before using any noise function. Default seed = 0.
   ----------------------------------------------------------------------- */

void UpeNoise_Seed(uint32_t seed);

/* -----------------------------------------------------------------------
   Value noise — trilinear interpolation of pseudo-random lattice values
   Returns [-1, 1]
   ----------------------------------------------------------------------- */

float UpeNoise_Value2D(float x, float y);
float UpeNoise_Value3D(float x, float y, float z);

/* -----------------------------------------------------------------------
   Perlin gradient noise (Ken Perlin 2002 "Improved" permutation)
   Returns approximately [-1, 1]
   ----------------------------------------------------------------------- */

float UpeNoise_Perlin2D(float x, float y);
float UpeNoise_Perlin3D(float x, float y, float z);

/* -----------------------------------------------------------------------
   Simplex noise (Gustavson algorithm, no patent issues)
   Returns approximately [-1, 1]
   ----------------------------------------------------------------------- */

float UpeNoise_Simplex2D(float x, float y);
float UpeNoise_Simplex3D(float x, float y, float z);

/* -----------------------------------------------------------------------
   Fractional Brownian Motion — sums octaves of a base noise function.
   octaves   : number of frequency layers (typically 4–8)
   lacunarity: frequency multiplier per octave (typically 2.0)
   gain      : amplitude multiplier per octave (typically 0.5)

   Function pointer allows any of the 2D/3D noise functions above to be used
   as the base noise.
   ----------------------------------------------------------------------- */

float UpeNoise_FBM2D(float x, float y, int octaves, float lacunarity, float gain,
                      float (*base_fn)(float, float));
float UpeNoise_FBM3D(float x, float y, float z, int octaves, float lacunarity, float gain,
                      float (*base_fn)(float, float, float));

/* -----------------------------------------------------------------------
   Worley (cellular / Voronoi) noise
   mode:
     0 = F1   (nearest feature point distance)
     1 = F2   (second nearest)
     2 = F2-F1
   Returns [0, ~1]
   ----------------------------------------------------------------------- */

float UpeNoise_Worley2D(float x, float y, int mode);
float UpeNoise_Worley3D(float x, float y, float z, int mode);

/* -----------------------------------------------------------------------
   Curl noise (divergence-free vector field derived from Perlin gradient)
   Only 3D version (2D returns XZ components with Y=0).
   ----------------------------------------------------------------------- */

void UpeNoise_Curl3D(float x, float y, float z, float *out_cx, float *out_cy, float *out_cz);
void UpeNoise_Curl2D(float x, float z, float *out_cx, float *out_cz);

/* -----------------------------------------------------------------------
   Domain-warped noise  q = f(p + s * f(p))
   Applies one level of domain warping using Perlin3D as the base.
   warp_strength : typical range 0.5–4.0
   ----------------------------------------------------------------------- */

float UpeNoise_DomainWarp3D(float x, float y, float z, float warp_strength);

#ifdef __cplusplus
}
#endif
