#include "test_framework.h"
#include "upe/upe_hash.h"

TEST(TestHashDeterminism) {
    UpeHash64 hash_a;
    UpeHash64 hash_b;
    UpeHash64_Init(&hash_a, 0);
    UpeHash64_Init(&hash_b, 0);

    UpeHash64_UpdateU32(&hash_a, 42);
    UpeHash64_UpdateU32(&hash_a, 7);
    UpeHash64_UpdateU32(&hash_b, 42);
    UpeHash64_UpdateU32(&hash_b, 7);

    REQUIRE(UpeHash64_Final(&hash_a) == UpeHash64_Final(&hash_b));
}
