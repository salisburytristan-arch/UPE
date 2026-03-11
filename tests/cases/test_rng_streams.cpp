#include "test_framework.h"
#include "upe/upe_rng.h"

TEST(TestRngStreamsDeterminism) {
    UpeRngSet set_a;
    UpeRngSet set_b;

    UpeRngSet_Init(&set_a, 777u);
    UpeRngSet_Init(&set_b, 777u);

    for (int i = 0; i < 16; ++i) {
        REQUIRE(UpeRngSet_NextU32(&set_a, UPE_RNG_STREAM_SOLVER) ==
                UpeRngSet_NextU32(&set_b, UPE_RNG_STREAM_SOLVER));
    }

    uint32_t sample_a = UpeRngSet_NextU32(&set_a, UPE_RNG_STREAM_PARTICLES);
    uint32_t sample_b = UpeRngSet_NextU32(&set_a, UPE_RNG_STREAM_CLOTH);
    REQUIRE(sample_a != sample_b);
}
