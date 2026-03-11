#include "test_framework.h"
#include "upe/upe_epsilon.h"

TEST(TestEpsilonEqual) {
    REQUIRE(UpeEpsilon_EqualDefault(1.0f, 1.0f + 1.0e-6f));
    REQUIRE(!UpeEpsilon_EqualDefault(1.0f, 1.0f + 1.0e-2f));
}
