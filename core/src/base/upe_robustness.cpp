#include "upe_robustness.h"
#include <math.h>

int UpeIsFinite_F32(float value) {
    return isfinite(value);
}

int UpeIsFinite_Vec3(UpeVec3 v) {
    return isfinite(v.x) && isfinite(v.y) && isfinite(v.z);
}

int UpeIsFinite_Quat(UpeQuat q) {
    return isfinite(q.x) && isfinite(q.y) && isfinite(q.z) && isfinite(q.w);
}

float UpeSafeSqrt(float x) {
    if (x < 0.0f) return 0.0f;
    if (!isfinite(x)) return 0.0f;
    float result = sqrtf(x);
    return isfinite(result) ? result : 0.0f;
}

float UpeSafeReciprocal(float x) {
    if (fabsf(x) < 1e-12f) return 0.0f;
    if (!isfinite(x)) return 0.0f;
    float result = 1.0f / x;
    return isfinite(result) ? result : 0.0f;
}

UpeVec3 UpeSafeNormalize(UpeVec3 v, UpeVec3 fallback) {
    if (!UpeIsFinite_Vec3(v)) return fallback;
    
    float len_sq = v.x * v.x + v.y * v.y + v.z * v.z;
    if (len_sq < 1e-12f) return fallback;
    
    float inv_len = 1.0f / sqrtf(len_sq);
    if (!isfinite(inv_len)) return fallback;
    
    UpeVec3 result = {v.x * inv_len, v.y * inv_len, v.z * inv_len};
    return UpeIsFinite_Vec3(result) ? result : fallback;
}

UpeQuat UpeSafeQuatNormalize(UpeQuat q) {
    if (!UpeIsFinite_Quat(q)) {
        UpeQuat identity = {0, 0, 0, 1};
        return identity;
    }
    
    float len_sq = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
    if (len_sq < 1e-12f) {
        UpeQuat identity = {0, 0, 0, 1};
        return identity;
    }
    
    float inv_len = 1.0f / sqrtf(len_sq);
    if (!isfinite(inv_len)) {
        UpeQuat identity = {0, 0, 0, 1};
        return identity;
    }
    
    UpeQuat result = {q.x * inv_len, q.y * inv_len, q.z * inv_len, q.w * inv_len};
    return result;
}

float UpeClamp(float value, float min_val, float max_val) {
    if (!isfinite(value)) return min_val;
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

UpeVec3 UpeVec3_Clamp(UpeVec3 v, float min_val, float max_val) {
    UpeVec3 result;
    result.x = UpeClamp(v.x, min_val, max_val);
    result.y = UpeClamp(v.y, min_val, max_val);
    result.z = UpeClamp(v.z, min_val, max_val);
    return result;
}

int UpeIsZeroVec3(UpeVec3 v, float epsilon) {
    return fabsf(v.x) < epsilon && fabsf(v.y) < epsilon && fabsf(v.z) < epsilon;
}

int UpeIsParallel(UpeVec3 a, UpeVec3 b, float tolerance) {
    // Check if cross product is near zero
    UpeVec3 cross;
    cross.x = a.y * b.z - a.z * b.y;
    cross.y = a.z * b.x - a.x * b.z;
    cross.z = a.x * b.y - a.y * b.x;
    
    float cross_len_sq = cross.x*cross.x + cross.y*cross.y + cross.z*cross.z;
    return cross_len_sq < (tolerance * tolerance);
}

void UpeRobustness_Reset(UpeRobustnessStats* stats) {
    stats->nan_count = 0;
    stats->inf_count = 0;
    stats->degenerate_count = 0;
    stats->recoveries = 0;
}

void UpeRobustness_LogNaN(UpeRobustnessStats* stats) {
    stats->nan_count++;
}

void UpeRobustness_LogInf(UpeRobustnessStats* stats) {
    stats->inf_count++;
}
