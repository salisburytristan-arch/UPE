#include "test_framework.h"
#include "upe/upe_array.h"

TEST(TestArrayPush) {
    UpeArray array;
    UpeArray_Init(&array, sizeof(int), nullptr);

    int a = 10;
    int b = 20;
    REQUIRE(UpeArray_Push(&array, &a));
    REQUIRE(UpeArray_Push(&array, &b));
    REQUIRE(array.count == 2);

    int *first = (int *)UpeArray_At(&array, 0);
    int *second = (int *)UpeArray_At(&array, 1);
    REQUIRE(first && *first == 10);
    REQUIRE(second && *second == 20);

    UpeArray_Destroy(&array);
}
