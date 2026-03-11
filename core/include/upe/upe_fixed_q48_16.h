#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeQ48_16 {
    int64_t raw;
} UpeQ48_16;

enum {
    UPE_Q48_16_FRACTION_BITS = 16,
    UPE_Q48_16_ONE_RAW = 1 << UPE_Q48_16_FRACTION_BITS
};

UpeQ48_16 UpeQ48_16_FromRaw(int64_t raw);
UpeQ48_16 UpeQ48_16_FromI32(int32_t value);
int64_t UpeQ48_16_ToRaw(UpeQ48_16 value);
int32_t UpeQ48_16_ToI32(UpeQ48_16 value);

UpeQ48_16 UpeQ48_16_Add(UpeQ48_16 a, UpeQ48_16 b);
UpeQ48_16 UpeQ48_16_Sub(UpeQ48_16 a, UpeQ48_16 b);
UpeQ48_16 UpeQ48_16_Neg(UpeQ48_16 value);
UpeQ48_16 UpeQ48_16_Mul(UpeQ48_16 a, UpeQ48_16 b);
UpeQ48_16 UpeQ48_16_Div(UpeQ48_16 a, UpeQ48_16 b);
UpeQ48_16 UpeQ48_16_Min(UpeQ48_16 a, UpeQ48_16 b);
UpeQ48_16 UpeQ48_16_Max(UpeQ48_16 a, UpeQ48_16 b);

int UpeQ48_16_Cmp(UpeQ48_16 a, UpeQ48_16 b);

#ifdef __cplusplus
}
#endif
