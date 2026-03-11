#include "test_framework.h"
#include "upe/upe_net_delta.h"
#include <math.h>

TEST(TestNetDeltaCreate) {
    struct TestData {
        float x, y, z;
    };
    TestData data = {1.0f, 2.0f, 3.0f};

    UpeNetDelta *delta = UpeNetDelta_Create(42, &data, sizeof(TestData));
    REQUIRE(delta != nullptr);
    REQUIRE(delta->frame == 42);
    REQUIRE(delta->size == sizeof(TestData));

    UpeNetDelta_Destroy(delta);
}

TEST(TestNetDeltaDiffApply) {
    struct TestData {
        float x, y, z;
        uint32_t id;
    };

    TestData base = {1.0f, 2.0f, 3.0f, 7};
    TestData next = {1.5f, 2.0f, 5.0f, 7};
    TestData out = {};

    UpeNetDelta *delta = UpeNetDelta_CreateDiff(10, &base, &next, sizeof(TestData));
    REQUIRE(delta != nullptr);
    REQUIRE(UpeNetDelta_Apply(delta, &base, sizeof(TestData), &out, sizeof(TestData)) == 1);
    REQUIRE(out.x == next.x);
    REQUIRE(out.y == next.y);
    REQUIRE(out.z == next.z);
    REQUIRE(out.id == next.id);
    UpeNetDelta_Destroy(delta);
}

TEST(TestNetDeltaQuantizeVec3) {
    UpeVec3 min_v = {-10.0f, -10.0f, -10.0f};
    UpeVec3 max_v = {10.0f, 10.0f, 10.0f};
    UpeVec3 v = {3.25f, -7.5f, 9.0f};
    uint16_t q[3] = {};

    UpeNetDelta_QuantizeVec3(v, min_v, max_v, 16, q);
    UpeVec3 d = UpeNetDelta_DequantizeVec3(q, min_v, max_v, 16);

    REQUIRE(fabsf(v.x - d.x) < 0.01f);
    REQUIRE(fabsf(v.y - d.y) < 0.01f);
    REQUIRE(fabsf(v.z - d.z) < 0.01f);
}

TEST(TestNetDeltaSizeBudget) {
    struct BodyState {
        float px, py, pz;
        float vx, vy, vz;
        uint32_t id;
    };

    const uint32_t body_count = 1000;
    BodyState base[body_count] = {};
    BodyState next[body_count] = {};

    for (uint32_t i = 0; i < body_count; ++i) {
        base[i].id = i;
        next[i].id = i;
    }

    for (uint32_t i = 0; i < 10; ++i) {
        next[i].px = (float)i * 0.1f;
        next[i].py = (float)i * 0.2f;
        next[i].pz = (float)i * -0.05f;
        next[i].vx = (float)i * 0.01f;
    }

    UpeNetDelta *delta = UpeNetDelta_CreateDiff(7, base, next, sizeof(base));
    REQUIRE(delta != nullptr);
    REQUIRE(delta->size < 5u * 1024u);
    UpeNetDelta_Destroy(delta);
}
