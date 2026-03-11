#pragma once

#include <stdint.h>

#ifdef UPE_EXPERIMENTAL_Q48_16
#include "upe/upe_fixed_q48_16.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef UPE_EXPERIMENTAL_Q48_16

typedef UpeQ48_16 UpeScalar;

static inline UpeScalar UpeScalar_FromI32(int32_t value) {
    return UpeQ48_16_FromI32(value);
}

static inline int32_t UpeScalar_ToI32(UpeScalar value) {
    return UpeQ48_16_ToI32(value);
}

static inline UpeScalar UpeScalar_Add(UpeScalar a, UpeScalar b) {
    return UpeQ48_16_Add(a, b);
}

static inline UpeScalar UpeScalar_Sub(UpeScalar a, UpeScalar b) {
    return UpeQ48_16_Sub(a, b);
}

static inline UpeScalar UpeScalar_Mul(UpeScalar a, UpeScalar b) {
    return UpeQ48_16_Mul(a, b);
}

static inline UpeScalar UpeScalar_Div(UpeScalar a, UpeScalar b) {
    return UpeQ48_16_Div(a, b);
}

static inline UpeScalar UpeScalar_Neg(UpeScalar value) {
    return UpeQ48_16_Neg(value);
}

static inline UpeScalar UpeScalar_Min(UpeScalar a, UpeScalar b) {
    return UpeQ48_16_Min(a, b);
}

static inline UpeScalar UpeScalar_Max(UpeScalar a, UpeScalar b) {
    return UpeQ48_16_Max(a, b);
}

static inline int UpeScalar_Cmp(UpeScalar a, UpeScalar b) {
    return UpeQ48_16_Cmp(a, b);
}

static inline UpeScalar UpeScalar_Half(void) {
    return UpeQ48_16_FromRaw(UPE_Q48_16_ONE_RAW / 2);
}

#else

typedef float UpeScalar;

static inline UpeScalar UpeScalar_FromI32(int32_t value) {
    return (float)value;
}

static inline int32_t UpeScalar_ToI32(UpeScalar value) {
    return (int32_t)value;
}

static inline UpeScalar UpeScalar_Add(UpeScalar a, UpeScalar b) {
    return a + b;
}

static inline UpeScalar UpeScalar_Sub(UpeScalar a, UpeScalar b) {
    return a - b;
}

static inline UpeScalar UpeScalar_Mul(UpeScalar a, UpeScalar b) {
    return a * b;
}

static inline UpeScalar UpeScalar_Div(UpeScalar a, UpeScalar b) {
    return a / b;
}

static inline UpeScalar UpeScalar_Neg(UpeScalar value) {
    return -value;
}

static inline UpeScalar UpeScalar_Min(UpeScalar a, UpeScalar b) {
    return a < b ? a : b;
}

static inline UpeScalar UpeScalar_Max(UpeScalar a, UpeScalar b) {
    return a > b ? a : b;
}

static inline int UpeScalar_Cmp(UpeScalar a, UpeScalar b) {
    if (a < b) {
        return -1;
    }
    if (a > b) {
        return 1;
    }
    return 0;
}

static inline UpeScalar UpeScalar_Half(void) {
    return 0.5f;
}

#endif

#ifdef __cplusplus
}
#endif
