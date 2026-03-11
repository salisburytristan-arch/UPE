#include "test_framework.h"
#include "upe/upe_rng.h"

TEST(TestRngDeterminism) {
    UpeRng rng_a;
    UpeRng rng_b;
    UpeRng_Seed(&rng_a, 1234);
    UpeRng_Seed(&rng_b, 1234);

    for (int i = 0; i < 16; ++i) {
        REQUIRE(UpeRng_NextU32(&rng_a) == UpeRng_NextU32(&rng_b));
    }
}
