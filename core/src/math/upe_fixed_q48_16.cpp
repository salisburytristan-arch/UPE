#include "upe/upe_fixed_q48_16.h"

#include <limits.h>

#if defined(_MSC_VER) && defined(_M_X64)
#include <intrin.h>
#endif

UpeQ48_16 UpeQ48_16_FromRaw(int64_t raw) {
    UpeQ48_16 out = {raw};
    return out;
}

UpeQ48_16 UpeQ48_16_FromI32(int32_t value) {
    UpeQ48_16 out = {(int64_t)value << UPE_Q48_16_FRACTION_BITS};
    return out;
}

int64_t UpeQ48_16_ToRaw(UpeQ48_16 value) {
    return value.raw;
}

int32_t UpeQ48_16_ToI32(UpeQ48_16 value) {
    return (int32_t)(value.raw >> UPE_Q48_16_FRACTION_BITS);
}

static int64_t sat_add64(int64_t a, int64_t b) {
    int64_t out = a + b;
    if (((a ^ b) >= 0) && ((a ^ out) < 0)) {
        return a < 0 ? INT64_MIN : INT64_MAX;
    }
    return out;
}

static int64_t sat_sub64(int64_t a, int64_t b) {
    int64_t out = a - b;
    if (((a ^ b) < 0) && ((a ^ out) < 0)) {
        return a < 0 ? INT64_MIN : INT64_MAX;
    }
    return out;
}

#if defined(_MSC_VER) && defined(_M_X64)
static int64_t q48_16_div_msvc_x64(int64_t num_raw, int64_t den_raw) {
    const int negative = ((num_raw < 0) ^ (den_raw < 0)) ? 1 : 0;
    uint64_t abs_num = (num_raw < 0) ? (uint64_t)(-(num_raw + 1)) + 1u : (uint64_t)num_raw;
    uint64_t abs_den = (den_raw < 0) ? (uint64_t)(-(den_raw + 1)) + 1u : (uint64_t)den_raw;

    uint64_t high = abs_num >> (64 - UPE_Q48_16_FRACTION_BITS);
    uint64_t low = abs_num << UPE_Q48_16_FRACTION_BITS;

    if (high >= abs_den) {
        return negative ? INT64_MIN : INT64_MAX;
    }

    uint64_t remainder = 0;
    uint64_t quotient = _udiv128(high, low, abs_den, &remainder);
    (void)remainder;

    if (!negative) {
        if (quotient > (uint64_t)INT64_MAX) {
            return INT64_MAX;
        }
        return (int64_t)quotient;
    }

    if (quotient > 0x8000000000000000ull) {
        return INT64_MIN;
    }
    if (quotient == 0x8000000000000000ull) {
        return INT64_MIN;
    }
    return -(int64_t)quotient;
}
#endif

UpeQ48_16 UpeQ48_16_Add(UpeQ48_16 a, UpeQ48_16 b) {
    return UpeQ48_16_FromRaw(sat_add64(a.raw, b.raw));
}

UpeQ48_16 UpeQ48_16_Sub(UpeQ48_16 a, UpeQ48_16 b) {
    return UpeQ48_16_FromRaw(sat_sub64(a.raw, b.raw));
}

UpeQ48_16 UpeQ48_16_Neg(UpeQ48_16 value) {
    if (value.raw == INT64_MIN) {
        return UpeQ48_16_FromRaw(INT64_MAX);
    }
    return UpeQ48_16_FromRaw(-value.raw);
}

UpeQ48_16 UpeQ48_16_Mul(UpeQ48_16 a, UpeQ48_16 b) {
#if defined(_MSC_VER) && defined(_M_X64)
    int64_t high = 0;
    int64_t low = _mul128(a.raw, b.raw, &high);
    uint64_t hi_bits = (uint64_t)high;
    uint64_t lo_bits = (uint64_t)low;
    int64_t shifted = (int64_t)((hi_bits << (64 - UPE_Q48_16_FRACTION_BITS)) |
                                (lo_bits >> UPE_Q48_16_FRACTION_BITS));
    return UpeQ48_16_FromRaw(shifted);
#elif defined(__SIZEOF_INT128__)
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
    __int128 product = (__int128)a.raw * (__int128)b.raw;
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
    return UpeQ48_16_FromRaw((int64_t)(product >> UPE_Q48_16_FRACTION_BITS));
#else
    const int64_t a_hi = a.raw >> 32;
    const int64_t a_lo = (int64_t)(uint32_t)a.raw;
    const int64_t b_hi = b.raw >> 32;
    const int64_t b_lo = (int64_t)(uint32_t)b.raw;

    int64_t p0 = a_lo * b_lo;
    int64_t p1 = a_lo * b_hi;
    int64_t p2 = a_hi * b_lo;
    int64_t p3 = a_hi * b_hi;

    int64_t mid = p1 + p2;
    int64_t high = p3 + (mid >> 32);
    uint64_t low = (uint64_t)(uint32_t)p0 + ((uint64_t)(uint32_t)mid << 32);

    if (low < (uint64_t)(uint32_t)p0) {
        high += 1;
    }

    int64_t shifted = (int64_t)(((uint64_t)high << (64 - UPE_Q48_16_FRACTION_BITS)) |
                                (low >> UPE_Q48_16_FRACTION_BITS));
    return UpeQ48_16_FromRaw(shifted);
#endif
}

UpeQ48_16 UpeQ48_16_Div(UpeQ48_16 a, UpeQ48_16 b) {
    if (b.raw == 0) {
        return UpeQ48_16_FromRaw(a.raw >= 0 ? INT64_MAX : INT64_MIN);
    }
#if defined(_MSC_VER) && defined(_M_X64)
    return UpeQ48_16_FromRaw(q48_16_div_msvc_x64(a.raw, b.raw));
#elif defined(__SIZEOF_INT128__)
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
    __int128 numerator = (__int128)a.raw << UPE_Q48_16_FRACTION_BITS;
    __int128 quotient = numerator / (__int128)b.raw;
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
    if (quotient > INT64_MAX) {
        return UpeQ48_16_FromRaw(INT64_MAX);
    }
    if (quotient < INT64_MIN) {
        return UpeQ48_16_FromRaw(INT64_MIN);
    }
    return UpeQ48_16_FromRaw((int64_t)quotient);
#else
    const long double scale = (long double)(1u << UPE_Q48_16_FRACTION_BITS);
    long double quotient = ((long double)a.raw * scale) / (long double)b.raw;
    if (quotient > (long double)INT64_MAX) {
        return UpeQ48_16_FromRaw(INT64_MAX);
    }
    if (quotient < (long double)INT64_MIN) {
        return UpeQ48_16_FromRaw(INT64_MIN);
    }
    return UpeQ48_16_FromRaw((int64_t)quotient);
#endif
}

UpeQ48_16 UpeQ48_16_Min(UpeQ48_16 a, UpeQ48_16 b) {
    return (a.raw <= b.raw) ? a : b;
}

UpeQ48_16 UpeQ48_16_Max(UpeQ48_16 a, UpeQ48_16 b) {
    return (a.raw >= b.raw) ? a : b;
}

int UpeQ48_16_Cmp(UpeQ48_16 a, UpeQ48_16 b) {
    if (a.raw < b.raw) {
        return -1;
    }
    if (a.raw > b.raw) {
        return 1;
    }
    return 0;
}
