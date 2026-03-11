#include "test_framework.h"
#include "upe/upe_contact.h"

TEST(TestContactManifoldClear) {
    UpeContactManifold manifold;
    manifold.count = 2;
    UpeContactManifold_Clear(&manifold);
    REQUIRE(manifold.count == 0);
}
