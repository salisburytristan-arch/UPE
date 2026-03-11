#pragma once
/* upe_rendering_math.h — Pure-math rendering utilities (header-only, no .cpp needed)
   All functions are static inline. Include in your renderer without linking a .cpp.
   Follows the UPE pure-C pattern: no external deps, float throughout.
*/

#ifdef __cplusplus
extern "C" {
#endif

#include "upe_math.h"   /* UpeVec3, UpeVec4, UpeMat4 */
#include <math.h>
#include <string.h>
#include <stdint.h>

/* -----------------------------------------------------------------------
   Projection matrices
   ----------------------------------------------------------------------- */

/* Perspective projection (OpenGL / Vulkan convention, depth [-1,1] clip space).
   fov_y_rad : full vertical field-of-view in radians
   aspect    : width / height
*/
static inline UpeMat4 UpeRender_Perspective(float fov_y_rad, float aspect,
                                              float near_z, float far_z) {
    UpeMat4 m; memset(&m, 0, sizeof(m));
    float f = 1.0f / tanf(fov_y_rad * 0.5f);
    m.m[0][0] = f / aspect;
    m.m[1][1] = f;
    m.m[2][2] = (far_z + near_z) / (near_z - far_z);
    m.m[2][3] = -1.0f;
    m.m[3][2] = (2.0f * far_z * near_z) / (near_z - far_z);
    return m;
}

/* Orthographic projection */
static inline UpeMat4 UpeRender_Ortho(float left, float right,
                                       float bottom, float top,
                                       float near_z, float far_z) {
    UpeMat4 m; memset(&m, 0, sizeof(m));
    m.m[0][0] =  2.0f / (right - left);
    m.m[1][1] =  2.0f / (top - bottom);
    m.m[2][2] = -2.0f / (far_z - near_z);
    m.m[3][0] = -(right + left)   / (right - left);
    m.m[3][1] = -(top   + bottom) / (top   - bottom);
    m.m[3][2] = -(far_z + near_z) / (far_z - near_z);
    m.m[3][3] =  1.0f;
    return m;
}

/* View (camera look-at) matrix — right-handed */
static inline UpeMat4 UpeRender_LookAt(UpeVec3 eye, UpeVec3 target, UpeVec3 up) {
    UpeVec3 f = { target.x - eye.x, target.y - eye.y, target.z - eye.z };
    float fl = 1.0f / sqrtf(f.x*f.x + f.y*f.y + f.z*f.z);
    f.x *= fl; f.y *= fl; f.z *= fl;
    UpeVec3 r = { f.y*up.z - f.z*up.y, f.z*up.x - f.x*up.z, f.x*up.y - f.y*up.x };
    float rl = 1.0f / sqrtf(r.x*r.x + r.y*r.y + r.z*r.z);
    r.x *= rl; r.y *= rl; r.z *= rl;
    UpeVec3 u = { r.y*f.z - r.z*f.y, r.z*f.x - r.x*f.z, r.x*f.y - r.y*f.x };
    UpeMat4 m; memset(&m, 0, sizeof(m));
    m.m[0][0] =  r.x; m.m[1][0] =  r.y; m.m[2][0] =  r.z; m.m[3][0] = -(r.x*eye.x + r.y*eye.y + r.z*eye.z);
    m.m[0][1] =  u.x; m.m[1][1] =  u.y; m.m[2][1] =  u.z; m.m[3][1] = -(u.x*eye.x + u.y*eye.y + u.z*eye.z);
    m.m[0][2] = -f.x; m.m[1][2] = -f.y; m.m[2][2] = -f.z; m.m[3][2] =  (f.x*eye.x + f.y*eye.y + f.z*eye.z);
    m.m[3][3] =  1.0f;
    return m;
}

/* -----------------------------------------------------------------------
   Normal encoding (octahedral mapping, Cigolle et al. 2014)
   Encodes a unit normal into a 2D [-1,1] value.
   ----------------------------------------------------------------------- */

static inline void UpeRender_EncodeOctNormal(UpeVec3 n, float *ox, float *oy) {
    float inv = 1.0f / (fabsf(n.x) + fabsf(n.y) + fabsf(n.z));
    float px = n.x * inv, py = n.y * inv;
    if (n.z < 0.0f) {
        float tx = (1.0f - fabsf(py)) * (px >= 0.0f ? 1.0f : -1.0f);
        float ty = (1.0f - fabsf(px)) * (py >= 0.0f ? 1.0f : -1.0f);
        px = tx; py = ty;
    }
    *ox = px; *oy = py;
}

static inline UpeVec3 UpeRender_DecodeOctNormal(float ox, float oy) {
    UpeVec3 n = { ox, oy, 1.0f - fabsf(ox) - fabsf(oy) };
    if (n.z < 0.0f) {
        float tx = n.x; float ty = n.y;
        n.x = (1.0f - fabsf(ty)) * (tx >= 0.0f ? 1.0f : -1.0f);
        n.y = (1.0f - fabsf(tx)) * (ty >= 0.0f ? 1.0f : -1.0f);
    }
    float len = sqrtf(n.x*n.x + n.y*n.y + n.z*n.z);
    n.x /= len; n.y /= len; n.z /= len;
    return n;
}

/* -----------------------------------------------------------------------
   Tone mapping
   ----------------------------------------------------------------------- */

/* Reinhard (extended) tone map — applies per-channel */
static inline float UpeRender_TonemapReinhard(float x, float white) {
    return x * (1.0f + x / (white * white)) / (1.0f + x);
}

/* ACES filmic approximation (Krzysztof Narkowicz) */
static inline float UpeRender_TonemapACES(float x) {
    float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
    float num = x * (a * x + b);
    float den = x * (c * x + d) + e;
    float t = num / den;
    return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
}

/* -----------------------------------------------------------------------
   Mip level estimation (OpenGL / D3D formula)
   derivative params are ∂u/∂x, ∂u/∂y, ∂v/∂x, ∂v/∂y in texel units
   ----------------------------------------------------------------------- */

static inline float UpeRender_MipLevel(float dudx, float dudy,
                                        float dvdx, float dvdy) {
    float lx = dudx*dudx + dvdx*dvdx;
    float ly = dudy*dudy + dvdy*dvdy;
    float max_grad = lx > ly ? lx : ly;
    if (max_grad < 1e-10f) return 0.0f;
    return 0.5f * log2f(max_grad);
}

/* -----------------------------------------------------------------------
   Luminance
   ----------------------------------------------------------------------- */

static inline float UpeRender_Luminance(float r, float g, float b) {
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

/* -----------------------------------------------------------------------
   Schlick Fresnel (also in upe_optics.h — re-exported here for renderers)
   ----------------------------------------------------------------------- */

static inline float UpeRender_FresnelSchlick(float cos_theta, float F0) {
    float c = 1.0f - cos_theta;
    float c5 = c*c*c*c*c;
    return F0 + (1.0f - F0) * c5;
}

/* -----------------------------------------------------------------------
   Spherical harmonics — irradiance bands 0–1 (L0, Lx, Ly, Lz)
   Evaluates the SH basis functions at a given direction.
   ----------------------------------------------------------------------- */

#define UPE_SH_L0  0.282094792f    /* 1 / (2 * sqrt(pi)) */
#define UPE_SH_L1  0.488602512f    /* sqrt(3 / (4*pi))   */

static inline float UpeRender_SH_Y00(void)                       { return UPE_SH_L0; }
static inline float UpeRender_SH_Y11m(float x)                   { return UPE_SH_L1 * x; }
static inline float UpeRender_SH_Y10 (float y)                   { return UPE_SH_L1 * y; }
static inline float UpeRender_SH_Y11p(float z)                   { return UPE_SH_L1 * z; }

/* Evaluate irradiance from 4-band (L0+L1) SH coefficients (RGB).
   coeffs[0..3] = R: L00, L1-1, L10, L11
   coeffs[4..7] = G, coeffs[8..11] = B
   out_rgb : irradiance in that direction
*/
static inline void UpeRender_SH_Irradiance(const float coeffs[12],
                                             UpeVec3 dir, float out_rgb[3]) {
    float s[4] = { UpeRender_SH_Y00(),
                   UpeRender_SH_Y11m(dir.x),
                   UpeRender_SH_Y10 (dir.y),
                   UpeRender_SH_Y11p(dir.z) };
    for (int ch = 0; ch < 3; ch++) {
        out_rgb[ch] = coeffs[ch*4+0]*s[0]
                    + coeffs[ch*4+1]*s[1]
                    + coeffs[ch*4+2]*s[2]
                    + coeffs[ch*4+3]*s[3];
        if (out_rgb[ch] < 0.0f) out_rgb[ch] = 0.0f;
    }
}

/* -----------------------------------------------------------------------
   UV atlas packing — power-of-2 bin packer (shelf algorithm, fit-first)
   Useful for building atlas layouts at runtime without malloc.
   Up to UPE_ATLAS_MAX_RECTS rectangles.
   ----------------------------------------------------------------------- */

#define UPE_ATLAS_MAX_RECTS 256

typedef struct UpeAtlasRect {
    int x, y, w, h;
} UpeAtlasRect;

typedef struct UpeAtlasPacker {
    int  width, height;
    int  shelf_x, shelf_y, shelf_h;
    uint32_t count;
    UpeAtlasRect rects[UPE_ATLAS_MAX_RECTS];
} UpeAtlasPacker;

static inline void UpeAtlasPacker_Init(UpeAtlasPacker *p, int width, int height) {
    memset(p, 0, sizeof(*p));
    p->width = width; p->height = height;
}

/* Returns 1 if placed, 0 if out of space. Output in *out_x, *out_y. */
static inline int UpeAtlasPacker_Insert(UpeAtlasPacker *p, int rw, int rh,
                                         int *out_x, int *out_y) {
    if (!p || p->count >= UPE_ATLAS_MAX_RECTS) return 0;
    if (p->shelf_x + rw > p->width) {
        /* New shelf */
        p->shelf_y += p->shelf_h;
        p->shelf_x  = 0; p->shelf_h = 0;
    }
    if (p->shelf_y + rh > p->height) return 0;
    if (rh > p->shelf_h) p->shelf_h = rh;
    *out_x = p->shelf_x; *out_y = p->shelf_y;
    p->rects[p->count].x = p->shelf_x; p->rects[p->count].y = p->shelf_y;
    p->rects[p->count].w = rw;         p->rects[p->count].h = rh;
    p->count++;
    p->shelf_x += rw;
    return 1;
}

/* UV coordinates [0,1] for a packed rect */
static inline void UpeAtlasPacker_UV(const UpeAtlasPacker *p, uint32_t idx,
                                      float *u0, float *v0, float *u1, float *v1) {
    if (!p || idx >= p->count) { *u0=*v0=*u1=*v1=0.0f; return; }
    float iw = 1.0f / (float)p->width, ih = 1.0f / (float)p->height;
    *u0 = (float)p->rects[idx].x * iw;
    *v0 = (float)p->rects[idx].y * ih;
    *u1 = (float)(p->rects[idx].x + p->rects[idx].w) * iw;
    *v1 = (float)(p->rects[idx].y + p->rects[idx].h) * ih;
}

#ifdef __cplusplus
}
#endif
