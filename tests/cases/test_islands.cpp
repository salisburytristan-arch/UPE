#include "test_framework.h"
#include "upe/upe_islands.h"

TEST(TestIslandAddBody) {
    UpeIsland island;
    UpeIsland_Init(&island);

    UpeIsland_AddBody(&island, 10);
    UpeIsland_AddBody(&island, 20);
    REQUIRE(island.body_count == 2);
    REQUIRE(island.awake == 1);

    UpeIsland_Sleep(&island);
    REQUIRE(island.awake == 0);

    UpeIsland_Wake(&island);
    REQUIRE(island.awake == 1);

    UpeIsland_Destroy(&island);
}
