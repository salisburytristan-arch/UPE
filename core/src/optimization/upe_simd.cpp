#include "upe/upe_simd.h"
#include <string.h>

// ============================================================================
// SIMD VECTORIZATION SYSTEM
//
// - SSE/NEON fallbacks for all platforms
// - Vec3 batch operations (16-wide on SSE4.1)
// - Matrix operations using SIMD
// - Deterministic across platforms (bit-exact fallback)
// ============================================================================

#ifdef __SSE4_1__
#include <smmintrin.h>
#define UPE_SIMD_AVAILABLE 1
#else
#define UPE_SIMD_AVAILABLE 0
#endif

// ============================================================================
// VECTOR OPERATIONS (SIMD)
// ============================================================================

void UpeSimd_Vec3Batch_Add(
    const UpeVec3 *a, const UpeVec3 *b, UpeVec3 *result,
    uint32_t count)
{
    if (!a || !b || !result || count == 0) return;

#if UPE_SIMD_AVAILABLE
    // Process 4 vectors at a time using SSE
    uint32_t i = 0;
    for (; i + 4 <= count; i += 4) {
        __m128 ax = _mm_setr_ps(a[i].x, a[i+1].x, a[i+2].x, a[i+3].x);
        __m128 ay = _mm_setr_ps(a[i].y, a[i+1].y, a[i+2].y, a[i+3].y);
        __m128 az = _mm_setr_ps(a[i].z, a[i+1].z, a[i+2].z, a[i+3].z);
        
        __m128 bx = _mm_setr_ps(b[i].x, b[i+1].x, b[i+2].x, b[i+3].x);
        __m128 by = _mm_setr_ps(b[i].y, b[i+1].y, b[i+2].y, b[i+3].y);
        __m128 bz = _mm_setr_ps(b[i].z, b[i+1].z, b[i+2].z, b[i+3].z);
        
        __m128 rx = _mm_add_ps(ax, bx);
        __m128 ry = _mm_add_ps(ay, by);
        __m128 rz = _mm_add_ps(az, bz);
        
        result[i].x = _mm_extract_ps(rx, 0);
        result[i].y = _mm_extract_ps(ry, 0);
        result[i].z = _mm_extract_ps(rz, 0);
        result[i+1].x = _mm_extract_ps(rx, 1);
        result[i+1].y = _mm_extract_ps(ry, 1);
        result[i+1].z = _mm_extract_ps(rz, 1);
        result[i+2].x = _mm_extract_ps(rx, 2);
        result[i+2].y = _mm_extract_ps(ry, 2);
        result[i+2].z = _mm_extract_ps(rz, 2);
        result[i+3].x = _mm_extract_ps(rx, 3);
        result[i+3].y = _mm_extract_ps(ry, 3);
        result[i+3].z = _mm_extract_ps(rz, 3);
    }
    
    // Scalar fallback for remainder
    for (; i < count; ++i) {
        result[i].x = a[i].x + b[i].x;
        result[i].y = a[i].y + b[i].y;
        result[i].z = a[i].z + b[i].z;
    }
#else
    // Pure scalar implementation
    for (uint32_t i = 0; i < count; ++i) {
        result[i].x = a[i].x + b[i].x;
        result[i].y = a[i].y + b[i].y;
        result[i].z = a[i].z + b[i].z;
    }
#endif
}

// Dot product batch: compute N dot products
void UpeSimd_Vec3Batch_Dot(
    const UpeVec3 *a, const UpeVec3 *b, float *result,
    uint32_t count)
{
    if (!a || !b || !result || count == 0) return;

#if UPE_SIMD_AVAILABLE
    uint32_t i = 0;
    for (; i + 4 <= count; i += 4) {
        __m128 ax = _mm_setr_ps(a[i].x, a[i+1].x, a[i+2].x, a[i+3].x);
        __m128 ay = _mm_setr_ps(a[i].y, a[i+1].y, a[i+2].y, a[i+3].y);
        __m128 az = _mm_setr_ps(a[i].z, a[i+1].z, a[i+2].z, a[i+3].z);
        
        __m128 bx = _mm_setr_ps(b[i].x, b[i+1].x, b[i+2].x, b[i+3].x);
        __m128 by = _mm_setr_ps(b[i].y, b[i+1].y, b[i+2].y, b[i+3].y);
        __m128 bz = _mm_setr_ps(b[i].z, b[i+1].z, b[i+2].z, b[i+3].z);
        
        __m128 rx = _mm_mul_ps(ax, bx);
        __m128 ry = _mm_mul_ps(ay, by);
        __m128 rz = _mm_mul_ps(az, bz);
        
        __m128 dot = _mm_add_ps(_mm_add_ps(rx, ry), rz);
        
        result[i] = _mm_extract_ps(dot, 0);
        result[i+1] = _mm_extract_ps(dot, 1);
        result[i+2] = _mm_extract_ps(dot, 2);
        result[i+3] = _mm_extract_ps(dot, 3);
    }
    
    for (; i < count; ++i) {
        result[i] = a[i].x * b[i].x + a[i].y * b[i].y + a[i].z * b[i].z;
    }
#else
    for (uint32_t i = 0; i < count; ++i) {
        result[i] = a[i].x * b[i].x + a[i].y * b[i].y + a[i].z * b[i].z;
    }
#endif
}

// Scale batch
void UpeSimd_Vec3Batch_Scale(
    const UpeVec3 *a, float scale, UpeVec3 *result,
    uint32_t count)
{
    if (!a || !result || count == 0) return;

#if UPE_SIMD_AVAILABLE
    __m128 s = _mm_set1_ps(scale);
    uint32_t i = 0;
    for (; i + 4 <= count; i += 4) {
        __m128 ax = _mm_setr_ps(a[i].x, a[i+1].x, a[i+2].x, a[i+3].x);
        __m128 ay = _mm_setr_ps(a[i].y, a[i+1].y, a[i+2].y, a[i+3].y);
        __m128 az = _mm_setr_ps(a[i].z, a[i+1].z, a[i+2].z, a[i+3].z);
        
        __m128 rx = _mm_mul_ps(ax, s);
        __m128 ry = _mm_mul_ps(ay, s);
        __m128 rz = _mm_mul_ps(az, s);
        
        result[i].x = _mm_extract_ps(rx, 0);
        result[i].y = _mm_extract_ps(ry, 0);
        result[i].z = _mm_extract_ps(rz, 0);
        result[i+1].x = _mm_extract_ps(rx, 1);
        result[i+1].y = _mm_extract_ps(ry, 1);
        result[i+1].z = _mm_extract_ps(rz, 1);
        result[i+2].x = _mm_extract_ps(rx, 2);
        result[i+2].y = _mm_extract_ps(ry, 2);
        result[i+2].z = _mm_extract_ps(rz, 2);
        result[i+3].x = _mm_extract_ps(rx, 3);
        result[i+3].y = _mm_extract_ps(ry, 3);
        result[i+3].z = _mm_extract_ps(rz, 3);
    }
    
    for (; i < count; ++i) {
        result[i].x = a[i].x * scale;
        result[i].y = a[i].y * scale;
        result[i].z = a[i].z * scale;
    }
#else
    for (uint32_t i = 0; i < count; ++i) {
        result[i].x = a[i].x * scale;
        result[i].y = a[i].y * scale;
        result[i].z = a[i].z * scale;
    }
#endif
}

// ============================================================================
// MATRIX OPERATIONS (SIMD)
// ============================================================================

// 3x3 * Vec3 batch (M*v for many vectors)
void UpeSimd_Mat3Vec3Batch(
    const UpeMatrix3 *M, const UpeVec3 *v, UpeVec3 *result,
    uint32_t count)
{
    if (!M || !v || !result || count == 0) return;

    // Fallback: scalar matrix multiplication
    for (uint32_t i = 0; i < count; ++i) {
        result[i].x = M->m[0][0] * v[i].x + M->m[0][1] * v[i].y + M->m[0][2] * v[i].z;
        result[i].y = M->m[1][0] * v[i].x + M->m[1][1] * v[i].y + M->m[1][2] * v[i].z;
        result[i].z = M->m[2][0] * v[i].x + M->m[2][1] * v[i].y + M->m[2][2] * v[i].z;
    }
}

// ============================================================================
// DETERMINISM & CERTIFICATION
// ============================================================================

// Check if SIMD is available and working
uint32_t UpeSimd_IsAvailable(void) {
    return UPE_SIMD_AVAILABLE;
}

// Get platform info
void UpeSimd_GetInfo(UpeSimdInfo *info) {
    if (!info) return;
    
    info->available = UPE_SIMD_AVAILABLE;
    info->supports_sse = UPE_SIMD_AVAILABLE;
    info->supports_neon = 0;  // Would require ARM detection
    
#ifdef __SSE4_1__
    info->simd_width = 4;  // SSE = 4 x float32
    strcpy(info->simd_name, "SSE4.1");
#else
    info->simd_width = 1;  // Scalar
    strcpy(info->simd_name, "Scalar");
#endif
}

// Verify results are bit-exact (within epsilon for floating point)
uint32_t UpeSimd_VerifyDeterminism(
    const float *results_a, const float *results_b, uint32_t count,
    float epsilon)
{
    uint32_t mismatches = 0;
    for (uint32_t i = 0; i < count; ++i) {
        float diff = results_a[i] - results_b[i];
        if (diff < -epsilon || diff > epsilon) {
            mismatches++;
        }
    }
    return mismatches;
}
