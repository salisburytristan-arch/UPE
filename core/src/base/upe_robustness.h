#ifndef UPE_ROBUSTNESS_H
#define UPE_ROBUSTNESS_H

#include "upe/upe_math.h"
#include <stdint.h>
#include <float.h>

#ifdef __cplusplus
extern "C" {
#endif

// NaN/Inf detection and handling
int UpeIsFinite_F32(float value);
int UpeIsFinite_Vec3(UpeVec3 v);
int UpeIsFinite_Quat(UpeQuat q);

// Safe math operations with fallback
float UpeSafeSqrt(float x);
float UpeSafeReciprocal(float x);
UpeVec3 UpeSafeNormalize(UpeVec3 v, UpeVec3 fallback);
UpeQuat UpeSafeQuatNormalize(UpeQuat q);

// Clamp and sanitize
float UpeClamp(float value, float min_val, float max_val);
UpeVec3 UpeVec3_Clamp(UpeVec3 v, float min_val, float max_val);

// Degenerate case detection
int UpeIsZeroVec3(UpeVec3 v, float epsilon);
int UpeIsParallel(UpeVec3 a, UpeVec3 b, float tolerance);

// Error recovery
typedef struct {
    uint32_t nan_count;
    uint32_t inf_count;
    uint32_t degenerate_count;
    uint32_t recoveries;
} UpeRobustnessStats;

void UpeRobustness_Reset(UpeRobustnessStats* stats);
void UpeRobustness_LogNaN(UpeRobustnessStats* stats);
void UpeRobustness_LogInf(UpeRobustnessStats* stats);

#ifdef __cplusplus
}
#endif

#endif // UPE_ROBUSTNESS_H
