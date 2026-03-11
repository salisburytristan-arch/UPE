#ifndef UPE_SIMD_H
#define UPE_SIMD_H

#include "upe_math.h"
#include <stdint.h>

// ============================================================================
// SIMD VECTORIZATION HEADER
//
// - SSE/NEON vectorized math operations
// - Batch processing for Vec3, Mat3 operations
// - Deterministic cross-platform guarantees
// - Platform detection and fallback
// ============================================================================

typedef struct UpeSimdInfo {
    uint32_t available;
    uint32_t supports_sse;
    uint32_t supports_neon;
    uint32_t simd_width;
    char simd_name[32];
} UpeSimdInfo;

// Batch operations (4-wide on SSE)
void UpeSimd_Vec3Batch_Add(const UpeVec3 *a, const UpeVec3 *b, UpeVec3 *result, uint32_t count);
void UpeSimd_Vec3Batch_Dot(const UpeVec3 *a, const UpeVec3 *b, float *result, uint32_t count);
void UpeSimd_Vec3Batch_Scale(const UpeVec3 *a, float scale, UpeVec3 *result, uint32_t count);

// Matrix operations
void UpeSimd_Mat3Vec3Batch(const UpeMatrix3 *M, const UpeVec3 *v, UpeVec3 *result, uint32_t count);

// Platform info
uint32_t UpeSimd_IsAvailable(void);
void UpeSimd_GetInfo(UpeSimdInfo *info);

// Determinism verification
uint32_t UpeSimd_VerifyDeterminism(const float *results_a, const float *results_b, uint32_t count, float epsilon);

#endif // UPE_SIMD_H
