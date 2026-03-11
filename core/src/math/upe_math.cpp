#include "upe/upe_math.h"
#include <math.h>

UpeVec2 UpeVec2_Add(UpeVec2 a, UpeVec2 b) {
    return {a.x + b.x, a.y + b.y};
}

UpeVec2 UpeVec2_Sub(UpeVec2 a, UpeVec2 b) {
    return {a.x - b.x, a.y - b.y};
}

float UpeVec2_Dot(UpeVec2 a, UpeVec2 b) {
    return a.x * b.x + a.y * b.y;
}

UpeVec2 UpeVec2_Scale(UpeVec2 v, float s) {
    return {v.x * s, v.y * s};
}

float UpeVec2_Length(UpeVec2 v) {
    return sqrtf(UpeVec2_Dot(v, v));
}

UpeVec2 UpeVec2_Normalize(UpeVec2 v) {
    float len = UpeVec2_Length(v);
    if (len > 0.0f) {
        return UpeVec2_Scale(v, 1.0f / len);
    }
    return {0.0f, 0.0f};
}

UpeVec2 UpeVec2_Min(UpeVec2 a, UpeVec2 b) {
    return {a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y};
}

UpeVec2 UpeVec2_Max(UpeVec2 a, UpeVec2 b) {
    return {a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y};
}

UpeVec3 UpeVec3_Add(UpeVec3 a, UpeVec3 b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

UpeVec3 UpeVec3_Sub(UpeVec3 a, UpeVec3 b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

float UpeVec3_Dot(UpeVec3 a, UpeVec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

UpeVec3 UpeVec3_Scale(UpeVec3 v, float s) {
    return {v.x * s, v.y * s, v.z * s};
}

float UpeVec3_Length(UpeVec3 v) {
    return sqrtf(UpeVec3_Dot(v, v));
}

UpeVec3 UpeVec3_Normalize(UpeVec3 v) {
    float len = UpeVec3_Length(v);
    if (len > 0.0f) {
        return UpeVec3_Scale(v, 1.0f / len);
    }
    return {0.0f, 0.0f, 0.0f};
}

UpeVec3 UpeVec3_Cross(UpeVec3 a, UpeVec3 b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

UpeVec3 UpeVec3_Min(UpeVec3 a, UpeVec3 b) {
    return {
        a.x < b.x ? a.x : b.x,
        a.y < b.y ? a.y : b.y,
        a.z < b.z ? a.z : b.z,
    };
}

UpeVec3 UpeVec3_Max(UpeVec3 a, UpeVec3 b) {
    return {
        a.x > b.x ? a.x : b.x,
        a.y > b.y ? a.y : b.y,
        a.z > b.z ? a.z : b.z,
    };
}

UpeVec4 UpeVec4_Add(UpeVec4 a, UpeVec4 b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

UpeVec4 UpeVec4_Sub(UpeVec4 a, UpeVec4 b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}

float UpeVec4_Dot(UpeVec4 a, UpeVec4 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

UpeMat2 UpeMat2_Identity() {
    UpeMat2 m = {};
    m.m[0][0] = 1.0f;
    m.m[1][1] = 1.0f;
    return m;
}

UpeVec2 UpeMat2_MulVec2(UpeMat2 m, UpeVec2 v) {
    UpeVec2 out;
    out.x = m.m[0][0] * v.x + m.m[0][1] * v.y;
    out.y = m.m[1][0] * v.x + m.m[1][1] * v.y;
    return out;
}

UpeMat3 UpeMat3_Identity() {
    UpeMat3 m = {};
    m.m[0][0] = 1.0f;
    m.m[1][1] = 1.0f;
    m.m[2][2] = 1.0f;
    return m;
}

UpeVec3 UpeMat3_MulVec3(UpeMat3 m, UpeVec3 v) {
    UpeVec3 out;
    out.x = m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z;
    out.y = m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z;
    out.z = m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z;
    return out;
}

UpeMat4 UpeMat4_Identity() {
    UpeMat4 m = {};
    m.m[0][0] = 1.0f;
    m.m[1][1] = 1.0f;
    m.m[2][2] = 1.0f;
    m.m[3][3] = 1.0f;
    return m;
}

UpeVec4 UpeMat4_MulVec4(UpeMat4 m, UpeVec4 v) {
    UpeVec4 out;
    out.x = m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z + m.m[0][3] * v.w;
    out.y = m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z + m.m[1][3] * v.w;
    out.z = m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z + m.m[2][3] * v.w;
    out.w = m.m[3][0] * v.x + m.m[3][1] * v.y + m.m[3][2] * v.z + m.m[3][3] * v.w;
    return out;
}

UpeQuat UpeQuat_Identity() {
    return {0.0f, 0.0f, 0.0f, 1.0f};
}

UpeQuat UpeQuat_Mul(UpeQuat a, UpeQuat b) {
    return {
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
    };
}

/* Alias for compatibility */
UpeQuat UpeQuat_Multiply(UpeQuat a, UpeQuat b) {
    return UpeQuat_Mul(a, b);
}

/* Spherical linear interpolation */
UpeQuat UpeQuat_Slerp(UpeQuat a, UpeQuat b, float t) {
    /* Clamp t to [0, 1] */
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    
    /* Calculate dot product */
    float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    
    /* If dot < 0, negate one quaternion to take shorter path */
    if (dot < 0.0f) {
        b.x = -b.x;
        b.y = -b.y;
        b.z = -b.z;
        b.w = -b.w;
        dot = -dot;
    }
    
    /* Clamp dot to prevent numerical issues in acos */
    if (dot > 1.0f) dot = 1.0f;
    if (dot < -1.0f) dot = -1.0f;
    
    /* Calculate angle between quaternions */
    float theta = acosf(dot);
    float sin_theta = sinf(theta);
    
    /* If angle is very small, use linear interpolation */
    if (sin_theta < 1e-6f) {
        return {
            a.x * (1.0f - t) + b.x * t,
            a.y * (1.0f - t) + b.y * t,
            a.z * (1.0f - t) + b.z * t,
            a.w * (1.0f - t) + b.w * t
        };
    }
    
    /* Standard spherical linear interpolation */
    float w1 = sinf((1.0f - t) * theta) / sin_theta;
    float w2 = sinf(t * theta) / sin_theta;
    
    return {
        a.x * w1 + b.x * w2,
        a.y * w1 + b.y * w2,
        a.z * w1 + b.z * w2,
        a.w * w1 + b.w * w2
    };
}

UpeVec3 UpeQuat_RotateVec3(UpeQuat q, UpeVec3 v) {
    UpeQuat vq = {v.x, v.y, v.z, 0.0f};
    UpeQuat qinv = {-q.x, -q.y, -q.z, q.w};
    UpeQuat rq = UpeQuat_Mul(UpeQuat_Mul(q, vq), qinv);
    return {rq.x, rq.y, rq.z};
}

float UpeVec3_ScalarTriple(UpeVec3 a, UpeVec3 b, UpeVec3 c) {
    return UpeVec3_Dot(a, UpeVec3_Cross(b, c));
}

UpeMat3 UpeMat3_Skew(UpeVec3 v) {
    UpeMat3 out = {{
        {0.0f, -v.z,  v.y},
        { v.z, 0.0f, -v.x},
        {-v.y, v.x, 0.0f}
    }};
    return out;
}

/* Jacobi eigen decomposition for symmetric 3x3 matrices */
void UpeMat3_EigenSymmetric(UpeMat3 m, float eigenvalues[3], UpeVec3 eigenvectors[3]) {
    if (!eigenvalues || !eigenvectors) return;
    float a[3][3] = {
        {m.m[0][0], m.m[0][1], m.m[0][2]},
        {m.m[1][0], m.m[1][1], m.m[1][2]},
        {m.m[2][0], m.m[2][1], m.m[2][2]}
    };
    float v[3][3] = {
        {1.0f,0.0f,0.0f},
        {0.0f,1.0f,0.0f},
        {0.0f,0.0f,1.0f}
    };
    for (int iter = 0; iter < 32; ++iter) {
        int p = 0, q = 1;
        float max = fabsf(a[0][1]);
        if (fabsf(a[0][2]) > max) { max = fabsf(a[0][2]); p = 0; q = 2; }
        if (fabsf(a[1][2]) > max) { max = fabsf(a[1][2]); p = 1; q = 2; }
        if (max < 1e-6f) break;
        float app = a[p][p], aqq = a[q][q], apq = a[p][q];
        float phi = 0.5f * atan2f(2.0f * apq, (aqq - app));
        float c = cosf(phi), s = sinf(phi);
        for (int k = 0; k < 3; ++k) {
            float aip = a[k][p], aiq = a[k][q];
            a[k][p] = c * aip - s * aiq;
            a[k][q] = s * aip + c * aiq;
        }
        for (int k = 0; k < 3; ++k) {
            float apk = a[p][k], aqk = a[q][k];
            a[p][k] = c * apk - s * aqk;
            a[q][k] = s * apk + c * aqk;
        }
        for (int k = 0; k < 3; ++k) {
            float vip = v[k][p], viq = v[k][q];
            v[k][p] = c * vip - s * viq;
            v[k][q] = s * vip + c * viq;
        }
    }
    eigenvalues[0] = a[0][0]; eigenvalues[1] = a[1][1]; eigenvalues[2] = a[2][2];
    eigenvectors[0] = {v[0][0], v[1][0], v[2][0]};
    eigenvectors[1] = {v[0][1], v[1][1], v[2][1]};
    eigenvectors[2] = {v[0][2], v[1][2], v[2][2]};
}

UpeMat4 UpeMat4_Perspective(float fovY_rad, float aspect, float near_z, float far_z) {
    float f = 1.0f / tanf(fovY_rad * 0.5f);
    return {
        f / aspect, 0.0f, 0.0f, 0.0f,
        0.0f, f, 0.0f, 0.0f,
        0.0f, 0.0f, (far_z + near_z) / (near_z - far_z), -1.0f,
        0.0f, 0.0f, (2.0f * far_z * near_z) / (near_z - far_z), 0.0f
    };
}

UpeMat4 UpeMat4_LookAt(UpeVec3 eye, UpeVec3 target, UpeVec3 up) {
    UpeVec3 f = UpeVec3_Normalize(UpeVec3_Sub(target, eye));
    UpeVec3 r = UpeVec3_Normalize(UpeVec3_Cross(f, up));
    UpeVec3 u = UpeVec3_Cross(r, f);
    return {
         r.x,  u.x, -f.x, 0.0f,
         r.y,  u.y, -f.y, 0.0f,
         r.z,  u.z, -f.z, 0.0f,
        -UpeVec3_Dot(r, eye), -UpeVec3_Dot(u, eye), UpeVec3_Dot(f, eye), 1.0f
    };
}
