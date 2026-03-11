#include "test_framework.h"
#include "../../core/src/base/upe_robustness.h"
#include <math.h>

TEST(TestRobustness_NaNDetection) {
    float nan_val = NAN;
    REQUIRE(UpeIsFinite_F32(nan_val) == 0);
    REQUIRE(UpeIsFinite_F32(1.0f) == 1);
}

TEST(TestRobustness_SafeSqrt) {
    REQUIRE(UpeSafeSqrt(-1.0f) == 0.0f);
    REQUIRE(UpeSafeSqrt(4.0f) == 2.0f);
    REQUIRE(UpeSafeSqrt(NAN) == 0.0f);
}

TEST(TestRobustness_SafeNormalize) {
    UpeVec3 zero = {0, 0, 0};
    UpeVec3 fallback = {1, 0, 0};
    UpeVec3 result = UpeSafeNormalize(zero, fallback);
    REQUIRE(result.x == 1.0f);
}

TEST(TestRobustness_Clamp) {
    REQUIRE(UpeClamp(5.0f, 0.0f, 10.0f) == 5.0f);
    REQUIRE(UpeClamp(-5.0f, 0.0f, 10.0f) == 0.0f);
    REQUIRE(UpeClamp(15.0f, 0.0f, 10.0f) == 10.0f);
}
