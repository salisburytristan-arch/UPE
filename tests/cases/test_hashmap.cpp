#include "test_framework.h"
#include "upe/upe_hashmap.h"

TEST(TestHashMapSetGetRemove) {
    UpeHashMap64 map;
    UpeHashMap64_Init(&map, 8, nullptr);

    REQUIRE(UpeHashMap64_Set(&map, 10, 100));
    REQUIRE(UpeHashMap64_Set(&map, 20, 200));

    uint64_t value = 0;
    REQUIRE(UpeHashMap64_Get(&map, 10, &value));
    REQUIRE(value == 100);
    REQUIRE(UpeHashMap64_Get(&map, 20, &value));
    REQUIRE(value == 200);

    REQUIRE(UpeHashMap64_Remove(&map, 10));
    REQUIRE(!UpeHashMap64_Get(&map, 10, &value));

    UpeHashMap64_Destroy(&map);
}
