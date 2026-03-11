#include "upe/upe_noise.h"
#include <math.h>
#include <string.h>

/* -----------------------------------------------------------------------
   Permutation table (512 entries for easy wrapping)
   ----------------------------------------------------------------------- */

static unsigned char s_perm[512];
static int s_seeded = 0;

/* LCG random for seeding */
static uint32_t lcg(uint32_t *s) { *s = *s * 1664525u + 1013904223u; return *s; }

void UpeNoise_Seed(uint32_t seed) {
    /* Fill perm[0..255] with values 0..255, then Fisher-Yates shuffle */
    unsigned char base[256];
    for (int i = 0; i < 256; i++) base[i] = (unsigned char)i;
    for (int i = 255; i > 0; i--) {
        uint32_t j = lcg(&seed) % ((uint32_t)i + 1);
        unsigned char tmp = base[i]; base[i] = base[j]; base[j] = tmp;
    }
    for (int i = 0; i < 256; i++) s_perm[i] = s_perm[i+256] = base[i];
    s_seeded = 1;
}

static void ensure_seeded(void) { if (!s_seeded) UpeNoise_Seed(0); }

static float fade(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
static float lerp_f(float a, float b, float t) { return a + t * (b - a); }

/* -----------------------------------------------------------------------
   Gradient helpers for Perlin
   ----------------------------------------------------------------------- */

static float grad2(int h, float x, float y) {
    switch (h & 3) {
        case 0:  return  x + y;
        case 1:  return -x + y;
        case 2:  return  x - y;
        default: return -x - y;
    }
}

static float grad3(int h, float x, float y, float z) {
    /* 12 gradients pointing to cube edges */
    int g = h & 15;
    float u = (g < 8) ? x : y;
    float v = (g < 4) ? y : ((g == 12 || g == 14) ? x : z);
    return ((g & 1) ? -u : u) + ((g & 2) ? -v : v);
}

/* -----------------------------------------------------------------------
   Pseudo-random float in [0,1) from 2/3 integer coords via perm table
   ----------------------------------------------------------------------- */

static int perm_hash2(int ix, int iy) {
    return s_perm[(s_perm[ix & 255] + iy) & 255];
}

static int perm_hash3(int ix, int iy, int iz) {
    return s_perm[(s_perm[(s_perm[ix & 255] + iy) & 255] + iz) & 255];
}

static float rand01(uint32_t h) {
    h = h * 1664525u + 1013904223u;
    h ^= h >> 16;
    return (float)(h & 0x7fffff) / (float)0x7fffff;
}

static float rand_neg(int ix, int iy) {
    return rand01((uint32_t)perm_hash2(ix, iy)) * 2.0f - 1.0f;
}
static float rand_neg3(int ix, int iy, int iz) {
    return rand01((uint32_t)perm_hash3(ix, iy, iz)) * 2.0f - 1.0f;
}

/* -----------------------------------------------------------------------
   Value noise
   ----------------------------------------------------------------------- */

float UpeNoise_Value2D(float x, float y) {
    ensure_seeded();
    int xi = (int)floorf(x), yi = (int)floorf(y);
    float xf = x - (float)xi, yf = y - (float)yi;
    float u = fade(xf), v = fade(yf);
    float v00 = rand_neg(xi,   yi);
    float v10 = rand_neg(xi+1, yi);
    float v01 = rand_neg(xi,   yi+1);
    float v11 = rand_neg(xi+1, yi+1);
    return lerp_f(lerp_f(v00, v10, u), lerp_f(v01, v11, u), v);
}

float UpeNoise_Value3D(float x, float y, float z) {
    ensure_seeded();
    int xi = (int)floorf(x), yi = (int)floorf(y), zi = (int)floorf(z);
    float xf = x-(float)xi, yf = y-(float)yi, zf = z-(float)zi;
    float u = fade(xf), v = fade(yf), w = fade(zf);
    float v000 = rand_neg3(xi,   yi,   zi);
    float v100 = rand_neg3(xi+1, yi,   zi);
    float v010 = rand_neg3(xi,   yi+1, zi);
    float v110 = rand_neg3(xi+1, yi+1, zi);
    float v001 = rand_neg3(xi,   yi,   zi+1);
    float v101 = rand_neg3(xi+1, yi,   zi+1);
    float v011 = rand_neg3(xi,   yi+1, zi+1);
    float v111 = rand_neg3(xi+1, yi+1, zi+1);
    float x0 = lerp_f(lerp_f(v000,v100,u), lerp_f(v010,v110,u), v);
    float x1 = lerp_f(lerp_f(v001,v101,u), lerp_f(v011,v111,u), v);
    return lerp_f(x0, x1, w);
}

/* -----------------------------------------------------------------------
   Perlin gradient noise (Improved Perlin, 2002)
   ----------------------------------------------------------------------- */

float UpeNoise_Perlin2D(float x, float y) {
    ensure_seeded();
    int xi = (int)floorf(x) & 255, yi = (int)floorf(y) & 255;
    float xf = x - floorf(x), yf = y - floorf(y);
    float u = fade(xf), v = fade(yf);
    int aa = s_perm[s_perm[xi  ] + yi  ];
    int ba = s_perm[s_perm[xi+1] + yi  ];
    int ab = s_perm[s_perm[xi  ] + yi+1];
    int bb = s_perm[s_perm[xi+1] + yi+1];
    float r = lerp_f(lerp_f(grad2(aa,xf,  yf  ), grad2(ba,xf-1,yf  ), u),
                     lerp_f(grad2(ab,xf,  yf-1 ), grad2(bb,xf-1,yf-1), u), v);
    return r;
}

float UpeNoise_Perlin3D(float x, float y, float z) {
    ensure_seeded();
    int xi = (int)floorf(x) & 255, yi = (int)floorf(y) & 255, zi = (int)floorf(z) & 255;
    float xf = x-floorf(x), yf = y-floorf(y), zf = z-floorf(z);
    float u = fade(xf), v = fade(yf), w = fade(zf);
    int A  = s_perm[xi  ]+yi, AA = s_perm[A]+zi, AB = s_perm[A+1]+zi;
    int B  = s_perm[xi+1]+yi, BA = s_perm[B]+zi, BB = s_perm[B+1]+zi;
    return lerp_f(
        lerp_f(lerp_f(grad3(s_perm[AA  ],xf,  yf,  zf  ), grad3(s_perm[BA  ],xf-1,yf,  zf  ),u),
               lerp_f(grad3(s_perm[AB  ],xf,  yf-1,zf  ), grad3(s_perm[BB  ],xf-1,yf-1,zf  ),u),v),
        lerp_f(lerp_f(grad3(s_perm[AA+1],xf,  yf,  zf-1), grad3(s_perm[BA+1],xf-1,yf,  zf-1),u),
               lerp_f(grad3(s_perm[AB+1],xf,  yf-1,zf-1), grad3(s_perm[BB+1],xf-1,yf-1,zf-1),u),v),
        w);
}

/* -----------------------------------------------------------------------
   Simplex noise 2D (Gustavson)
   ----------------------------------------------------------------------- */

#define UPE_SKEW2   0.36602540378f   /* (sqrt(3)-1)/2 */
#define UPE_UNSKEW2 0.21132486541f   /* (3-sqrt(3))/6 */

static const int s_grad2[8][2] = {{1,1},{-1,1},{1,-1},{-1,-1},{1,0},{-1,0},{0,1},{0,-1}};

float UpeNoise_Simplex2D(float x, float y) {
    ensure_seeded();
    float s = (x + y) * UPE_SKEW2;
    int i = (int)floorf(x + s), j = (int)floorf(y + s);
    float t  = (float)(i + j) * UPE_UNSKEW2;
    float x0 = x - ((float)i - t), y0 = y - ((float)j - t);
    int i1 = (x0 > y0) ? 1 : 0, j1 = (x0 > y0) ? 0 : 1;
    float x1 = x0 - (float)i1 + UPE_UNSKEW2;
    float y1 = y0 - (float)j1 + UPE_UNSKEW2;
    float x2 = x0 - 1.0f + 2.0f * UPE_UNSKEW2;
    float y2 = y0 - 1.0f + 2.0f * UPE_UNSKEW2;
    int gi0 = perm_hash2(i      & 255, j      & 255) % 8;
    int gi1 = perm_hash2((i+i1) & 255, (j+j1) & 255) % 8;
    int gi2 = perm_hash2((i+1)  & 255, (j+1)  & 255) % 8;
    float n0=0,n1=0,n2=0;
    float t0 = 0.5f - x0*x0 - y0*y0; if (t0 > 0) { t0*=t0; n0 = t0*t0*(s_grad2[gi0][0]*x0 + s_grad2[gi0][1]*y0); }
    float t1 = 0.5f - x1*x1 - y1*y1; if (t1 > 0) { t1*=t1; n1 = t1*t1*(s_grad2[gi1][0]*x1 + s_grad2[gi1][1]*y1); }
    float t2 = 0.5f - x2*x2 - y2*y2; if (t2 > 0) { t2*=t2; n2 = t2*t2*(s_grad2[gi2][0]*x2 + s_grad2[gi2][1]*y2); }
    return 70.0f * (n0 + n1 + n2);
}

/* -----------------------------------------------------------------------
   Simplex noise 3D
   ----------------------------------------------------------------------- */

#define UPE_SKEW3   0.33333333f   /* 1/3 */
#define UPE_UNSKEW3 0.16666667f   /* 1/6 */

static const int s_grad3[12][3] = {
    {1,1,0},{-1,1,0},{1,-1,0},{-1,-1,0},
    {1,0,1},{-1,0,1},{1,0,-1},{-1,0,-1},
    {0,1,1},{0,-1,1},{0,1,-1},{0,-1,-1}
};

float UpeNoise_Simplex3D(float x, float y, float z) {
    ensure_seeded();
    float s = (x+y+z) * UPE_SKEW3;
    int i=(int)floorf(x+s), j=(int)floorf(y+s), k=(int)floorf(z+s);
    float t = (float)(i+j+k)*UPE_UNSKEW3;
    float x0=x-((float)i-t), y0=y-((float)j-t), z0=z-((float)k-t);
    int i1,j1,k1,i2,j2,k2;
    if (x0>=y0) { if (y0>=z0){i1=1;j1=0;k1=0;i2=1;j2=1;k2=0;}
    else if (x0>=z0){i1=1;j1=0;k1=0;i2=1;j2=0;k2=1;}
    else{i1=0;j1=0;k1=1;i2=1;j2=0;k2=1;} }
    else { if (y0<z0){i1=0;j1=0;k1=1;i2=0;j2=1;k2=1;}
    else if (x0<z0){i1=0;j1=1;k1=0;i2=0;j2=1;k2=1;}
    else{i1=0;j1=1;k1=0;i2=1;j2=1;k2=0;} }
    float x1=x0-(float)i1+UPE_UNSKEW3, y1=y0-(float)j1+UPE_UNSKEW3, z1=z0-(float)k1+UPE_UNSKEW3;
    float x2=x0-(float)i2+2*UPE_UNSKEW3, y2=y0-(float)j2+2*UPE_UNSKEW3, z2=z0-(float)k2+2*UPE_UNSKEW3;
    float x3=x0-1.0f+3*UPE_UNSKEW3, y3=y0-1.0f+3*UPE_UNSKEW3, z3=z0-1.0f+3*UPE_UNSKEW3;
    int gi0 = perm_hash3(i &255, j &255, k &255)%12;
    int gi1 = perm_hash3((i+i1)&255,(j+j1)&255,(k+k1)&255)%12;
    int gi2 = perm_hash3((i+i2)&255,(j+j2)&255,(k+k2)&255)%12;
    int gi3 = perm_hash3((i+1)&255,(j+1)&255,(k+1)&255)%12;
    float n0=0,n1=0,n2=0,n3=0;
    float t0=0.6f-x0*x0-y0*y0-z0*z0; if(t0>0){t0*=t0;n0=t0*t0*(s_grad3[gi0][0]*x0+s_grad3[gi0][1]*y0+s_grad3[gi0][2]*z0);}
    float t1=0.6f-x1*x1-y1*y1-z1*z1; if(t1>0){t1*=t1;n1=t1*t1*(s_grad3[gi1][0]*x1+s_grad3[gi1][1]*y1+s_grad3[gi1][2]*z1);}
    float t2=0.6f-x2*x2-y2*y2-z2*z2; if(t2>0){t2*=t2;n2=t2*t2*(s_grad3[gi2][0]*x2+s_grad3[gi2][1]*y2+s_grad3[gi2][2]*z2);}
    float t3=0.6f-x3*x3-y3*y3-z3*z3; if(t3>0){t3*=t3;n3=t3*t3*(s_grad3[gi3][0]*x3+s_grad3[gi3][1]*y3+s_grad3[gi3][2]*z3);}
    return 32.0f*(n0+n1+n2+n3);
}

/* -----------------------------------------------------------------------
   Fractional Brownian Motion
   ----------------------------------------------------------------------- */

float UpeNoise_FBM2D(float x, float y, int octaves, float lacunarity, float gain,
                      float (*base_fn)(float, float)) {
    float val = 0.0f, amp = 1.0f, max_amp = 0.0f;
    float fx = x, fy = y;
    for (int i = 0; i < octaves; i++) {
        val += base_fn(fx, fy) * amp;
        max_amp += amp;
        amp *= gain; fx *= lacunarity; fy *= lacunarity;
    }
    return val / max_amp;
}

float UpeNoise_FBM3D(float x, float y, float z, int octaves, float lacunarity, float gain,
                      float (*base_fn)(float, float, float)) {
    float val = 0.0f, amp = 1.0f, max_amp = 0.0f;
    float fx = x, fy = y, fz = z;
    for (int i = 0; i < octaves; i++) {
        val += base_fn(fx, fy, fz) * amp;
        max_amp += amp;
        amp *= gain; fx *= lacunarity; fy *= lacunarity; fz *= lacunarity;
    }
    return val / max_amp;
}

/* -----------------------------------------------------------------------
   Worley (cellular) noise
   ----------------------------------------------------------------------- */

static float worley_cell_random(int ix, int iy, float *ox, float *oy) {
    uint32_t h = (uint32_t)(ix * 1664525 + iy * 22695477 + 1013904223);
    h ^= h >> 16; h *= 0x45d9f3bu; h ^= h >> 16;
    *ox = (float)(h & 0xffff) / 65536.0f;
    h = h * 1664525u + 1013904223u;
    *oy = (float)(h & 0xffff) / 65536.0f;
    return 0.0f;
}

static float worley_cell_random3(int ix, int iy, int iz, float *ox, float *oy, float *oz) {
    uint32_t h = (uint32_t)(ix*73856093 ^ iy*19349663 ^ iz*83492791);
    h ^= h >> 16; h *= 0x45d9f3bu; h ^= h >> 16;
    *ox = (float)(h & 0xffff) / 65536.0f;
    h = h*1664525u+1013904223u; *oy = (float)(h & 0xffff) / 65536.0f;
    h = h*1664525u+1013904223u; *oz = (float)(h & 0xffff) / 65536.0f;
    return 0.0f;
}

float UpeNoise_Worley2D(float x, float y, int mode) {
    int xi = (int)floorf(x), yi = (int)floorf(y);
    float f1 = 1e30f, f2 = 1e30f;
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
            float px, py;
            worley_cell_random(xi+dx, yi+dy, &px, &py);
            float cx = (float)(xi+dx) + px, cy = (float)(yi+dy) + py;
            float d2 = (x-cx)*(x-cx) + (y-cy)*(y-cy);
            if (d2 < f1) { f2 = f1; f1 = d2; }
            else if (d2 < f2) { f2 = d2; }
        }
    f1 = sqrtf(f1); f2 = sqrtf(f2);
    if (mode == 0) return f1;
    if (mode == 1) return f2;
    return f2 - f1;
}

float UpeNoise_Worley3D(float x, float y, float z, int mode) {
    int xi=(int)floorf(x), yi=(int)floorf(y), zi=(int)floorf(z);
    float f1=1e30f, f2=1e30f;
    for (int dz=-1;dz<=1;dz++)
        for (int dy=-1;dy<=1;dy++)
            for (int dx=-1;dx<=1;dx++) {
                float px,py,pz;
                worley_cell_random3(xi+dx,yi+dy,zi+dz,&px,&py,&pz);
                float cx=(float)(xi+dx)+px, cy=(float)(yi+dy)+py, cz=(float)(zi+dz)+pz;
                float d2=(x-cx)*(x-cx)+(y-cy)*(y-cy)+(z-cz)*(z-cz);
                if(d2<f1){f2=f1;f1=d2;} else if(d2<f2){f2=d2;}
            }
    f1=sqrtf(f1); f2=sqrtf(f2);
    if (mode==0) return f1;
    if (mode==1) return f2;
    return f2-f1;
}

/* -----------------------------------------------------------------------
   Curl noise (divergence-free field from Perlin gradient)
   Approximates curl via central differences of Perlin3D.
   ----------------------------------------------------------------------- */

#define UPE_CURL_EPS 0.0001f

void UpeNoise_Curl3D(float x, float y, float z,
                      float *out_cx, float *out_cy, float *out_cz) {
    /* curl F = (dFz/dy - dFy/dz, dFx/dz - dFz/dx, dFy/dx - dFx/dy)
       Use 3 offset noise fields (phase shifts) to ensure non-zero curl. */
    float e = UPE_CURL_EPS;
    float off1 = 0.0f;       /* Fx field offset */
    float off2 = 1.7342f;    /* Fy field */
    float off3 = 3.4561f;    /* Fz field */

    float dFzdy = (UpeNoise_Perlin3D(x,y+e,z+off3) - UpeNoise_Perlin3D(x,y-e,z+off3)) / (2*e);
    float dFydz = (UpeNoise_Perlin3D(x,y,z+e+off2) - UpeNoise_Perlin3D(x,y,z-e+off2)) / (2*e);
    float dFxdz = (UpeNoise_Perlin3D(x,y,z+e+off1) - UpeNoise_Perlin3D(x,y,z-e+off1)) / (2*e);
    float dFzdx = (UpeNoise_Perlin3D(x+e,y,z+off3) - UpeNoise_Perlin3D(x-e,y,z+off3)) / (2*e);
    float dFydx = (UpeNoise_Perlin3D(x+e,y,z+off2) - UpeNoise_Perlin3D(x-e,y,z+off2)) / (2*e);
    float dFxdy = (UpeNoise_Perlin3D(x,y+e,z+off1) - UpeNoise_Perlin3D(x,y-e,z+off1)) / (2*e);

    *out_cx = dFzdy - dFydz;
    *out_cy = dFxdz - dFzdx;
    *out_cz = dFydx - dFxdy;
}

void UpeNoise_Curl2D(float x, float z, float *out_cx, float *out_cz) {
    float cy_unused;
    UpeNoise_Curl3D(x, 0.0f, z, out_cx, &cy_unused, out_cz);
}

/* -----------------------------------------------------------------------
   Domain warp
   ----------------------------------------------------------------------- */

float UpeNoise_DomainWarp3D(float x, float y, float z, float warp_strength) {
    float wx = UpeNoise_Perlin3D(x      , y+0.0f , z+0.0f);
    float wy = UpeNoise_Perlin3D(x+5.2f , y+1.3f , z+0.0f);
    float wz = UpeNoise_Perlin3D(x+9.2f , y+4.1f , z+2.8f);
    return UpeNoise_Perlin3D(x + warp_strength*wx,
                              y + warp_strength*wy,
                              z + warp_strength*wz);
}
