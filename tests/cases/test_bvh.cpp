#include "test_framework.h"
#include "upe/upe_bvh.h"

TEST(TestBvhQueryAabb) {
    UpeAabb3 bounds[4];
    bounds[0] = UpeAabb3_FromMinMax({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    bounds[1] = UpeAabb3_FromMinMax({2.0f, 0.0f, 0.0f}, {3.0f, 1.0f, 1.0f});
    bounds[2] = UpeAabb3_FromMinMax({0.5f, 0.0f, 0.0f}, {1.5f, 1.0f, 1.0f});
    bounds[3] = UpeAabb3_FromMinMax({4.0f, 0.0f, 0.0f}, {5.0f, 1.0f, 1.0f});

    uint32_t indices[4];
    UpeBvhNode nodes[8];
    uint32_t node_count = UpeBvh_Build(bounds, 4, indices, nodes, 8);
    REQUIRE(node_count > 0);

    uint32_t hits[4];
    UpeAabb3 query = UpeAabb3_FromMinMax({0.25f, -0.1f, -0.1f}, {1.25f, 1.1f, 1.1f});
    uint32_t hit_count = UpeBvh_QueryAabb(nodes, node_count, bounds, indices, query, hits, 4);
    REQUIRE(hit_count == 2);
}
