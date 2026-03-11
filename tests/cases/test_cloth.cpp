#include "test_framework.h"
#include "upe/upe_cloth.h"

TEST(TestClothInit) {
    UpeCloth cloth;
    UpeCloth_Init(&cloth, 5, 5, 1.0f);
    REQUIRE(cloth.vertex_count == 25);
    REQUIRE(cloth.constraint_count > 0);
    REQUIRE(cloth.vertices[0].pinned == 1);
    UpeCloth_Destroy(&cloth);
}
