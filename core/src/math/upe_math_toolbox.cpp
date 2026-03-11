#include "upe/upe_math_toolbox.h"
#include <math.h>

UpeVec3 UpeMath_BezierCubic(UpeVec3 p0, UpeVec3 p1, UpeVec3 p2, UpeVec3 p3, float t) {
    float u = 1.0f - t;
    float b0 = u*u*u;
    float b1 = 3.0f*u*u*t;
    float b2 = 3.0f*u*t*t;
    float b3 = t*t*t;
    return {
        b0*p0.x + b1*p1.x + b2*p2.x + b3*p3.x,
        b0*p0.y + b1*p1.y + b2*p2.y + b3*p3.y,
        b0*p0.z + b1*p1.z + b2*p2.z + b3*p3.z
    };
}

UpeVec3 UpeMath_BezierCubicTangent(UpeVec3 p0, UpeVec3 p1, UpeVec3 p2, UpeVec3 p3, float t) {
    float u = 1.0f - t;
    return {
        3.0f*u*u*(p1.x-p0.x) + 6.0f*u*t*(p2.x-p1.x) + 3.0f*t*t*(p3.x-p2.x),
        3.0f*u*u*(p1.y-p0.y) + 6.0f*u*t*(p2.y-p1.y) + 3.0f*t*t*(p3.y-p2.y),
        3.0f*u*u*(p1.z-p0.z) + 6.0f*u*t*(p2.z-p1.z) + 3.0f*t*t*(p3.z-p2.z)
    };
}

float UpeMath_FractalBoxDimension(float count_small, float count_large, float scale_ratio) {
    if (count_small <= 0.0f || count_large <= 0.0f || scale_ratio <= 0.0f || scale_ratio == 1.0f) return 0.0f;
    return logf(count_large / count_small) / logf(scale_ratio);
}

static float wrap_pi2(float a) {
    const float PI2 = 6.28318530718f;
    while (a < 0.0f) a += PI2;
    while (a >= PI2) a -= PI2;
    return a;
}

float UpeMath_LUTSin(float angle_rad) {
    const int N = 256;
    static int init = 0;
    static float lut[N];
    if (!init) {
        for (int i = 0; i < N; i++) lut[i] = sinf((6.28318530718f * i) / (float)N);
        init = 1;
    }
    float a = wrap_pi2(angle_rad);
    float f = a * (N / 6.28318530718f);
    int i0 = (int)f & (N - 1);
    int i1 = (i0 + 1) & (N - 1);
    float t = f - (float)((int)f);
    return lut[i0] * (1.0f - t) + lut[i1] * t;
}

float UpeMath_LUTCos(float angle_rad) {
    return UpeMath_LUTSin(angle_rad + 1.57079632679f);
}

float UpeMath_SquareCubeLaw(float linear_scale) {
    if (linear_scale <= 0.0f) return 0.0f;
    /* (s^2)/(s^3) relative to baseline => 1/s */
    return 1.0f / linear_scale;
}
