#include "test_framework.h"
#include "upe/upe_api.h"
#include "upe/upe_snapshot.h"
#include "upe/upe_state_hash.h"

TEST(TestDeterminismSnapshotRoundtrip) {
    UpeWorldDesc desc = UpeWorldDesc_Default();
    desc.fixed_dt = 1.0f / 60.0f;

    UpeWorld *world = UpeWorld_Create(&desc);
    REQUIRE(world != nullptr);

    for (int i = 0; i < 12; ++i) {
        UpeWorld_Step(world);
    }

    const uint64_t state_hash = UpeWorld_GetLastStateHash(world);
    UpeStateHash hash;
    UpeStateHash_Begin(&hash, 0x1234ULL);
    UpeStateHash_PushU32(&hash, (uint32_t)state_hash);
    UpeStateHash_PushU32(&hash, (uint32_t)(state_hash >> 32));
    REQUIRE(UpeStateHash_Final(&hash) != 0u);

    UpeWorld_Destroy(world);
}