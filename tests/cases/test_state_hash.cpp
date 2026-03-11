#include "test_framework.h"
#include "upe/upe_state_hash.h"

TEST(TestStateHashDeterminism) {
    UpeStateHash a;
    UpeStateHash b;
    UpeStateHash_Begin(&a, 1);
    UpeStateHash_Begin(&b, 1);

    UpeStateHash_PushU32(&a, 10);
    UpeStateHash_PushU32(&a, 20);
    UpeStateHash_PushU32(&b, 10);
    UpeStateHash_PushU32(&b, 20);

    REQUIRE(UpeStateHash_Final(&a) == UpeStateHash_Final(&b));
}
