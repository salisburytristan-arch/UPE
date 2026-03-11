#include "test_framework.h"
#include "upe/upe_fixed_q48_16.h"

TEST(TestQ48_16_FromAndToInt) {
    UpeQ48_16 value = UpeQ48_16_FromI32(42);
    REQUIRE(UpeQ48_16_ToI32(value) == 42);
    REQUIRE(UpeQ48_16_ToRaw(value) == (42LL << UPE_Q48_16_FRACTION_BITS));
}

TEST(TestQ48_16_AddSub) {
    UpeQ48_16 a = UpeQ48_16_FromI32(10);
    UpeQ48_16 b = UpeQ48_16_FromI32(3);

    UpeQ48_16 sum = UpeQ48_16_Add(a, b);
    UpeQ48_16 diff = UpeQ48_16_Sub(a, b);

    REQUIRE(UpeQ48_16_ToI32(sum) == 13);
    REQUIRE(UpeQ48_16_ToI32(diff) == 7);
}

TEST(TestQ48_16_Mul) {
    UpeQ48_16 a = UpeQ48_16_FromI32(7);
    UpeQ48_16 b = UpeQ48_16_FromI32(-4);
    UpeQ48_16 product = UpeQ48_16_Mul(a, b);

    REQUIRE(UpeQ48_16_ToI32(product) == -28);
}

TEST(TestQ48_16_Div) {
    UpeQ48_16 a = UpeQ48_16_FromI32(21);
    UpeQ48_16 b = UpeQ48_16_FromI32(3);
    UpeQ48_16 c = UpeQ48_16_Div(a, b);
    REQUIRE(UpeQ48_16_ToI32(c) == 7);
}

TEST(TestQ48_16_CompareMinMax) {
    UpeQ48_16 low = UpeQ48_16_FromI32(-2);
    UpeQ48_16 high = UpeQ48_16_FromI32(5);

    REQUIRE(UpeQ48_16_Cmp(low, high) < 0);
    REQUIRE(UpeQ48_16_Cmp(high, low) > 0);
    REQUIRE(UpeQ48_16_Cmp(low, low) == 0);
    REQUIRE(UpeQ48_16_ToI32(UpeQ48_16_Min(low, high)) == -2);
    REQUIRE(UpeQ48_16_ToI32(UpeQ48_16_Max(low, high)) == 5);
}
