#include "test_framework.h"
#include "upe/upe_queries.h"

TEST(TestQueryFilter) {
    UpeQueryFilter filter = UpeQueryFilter_Default();
    REQUIRE(filter.layer_mask == 0xFFFFFFFF);
    REQUIRE(filter.include_triggers == 0);
}
