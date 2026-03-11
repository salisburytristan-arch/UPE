#include "test_framework.h"
#include "upe/upe_vector.h"

TEST(TestVectorPushPop) {
    UpeVector vec;
    UpeVector_Init(&vec, sizeof(int), nullptr);

    int a = 3;
    int b = 7;
    REQUIRE(UpeVector_Push(&vec, &a));
    REQUIRE(UpeVector_Push(&vec, &b));
    REQUIRE(vec.count == 2);

    int out = 0;
    REQUIRE(UpeVector_Pop(&vec, &out));
    REQUIRE(out == 7);
    REQUIRE(vec.count == 1);

    UpeVector_Destroy(&vec);
}
