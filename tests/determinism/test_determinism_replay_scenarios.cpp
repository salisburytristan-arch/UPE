#include "test_framework.h"
#include "upe/upe_api.h"
#include "upe/upe_determinism.h"
#include "upe/upe_state_hash.h"

static UpeWorld *MakeDeterministicWorld() {
    UpeWorldDesc desc = UpeWorldDesc_Default();
    desc.fixed_dt = 1.0f / 60.0f;
    desc.enable_determinism_mode = 1;
    return UpeWorld_Create(&desc);
}

TEST(TestDeterminismReplayWorldHash) {
    UpeWorld *world = MakeDeterministicWorld();
    REQUIRE(world != nullptr);

    for (int i = 0; i < 60; ++i) {
        UpeWorld_Step(world);
    }

    UpeStateHash hash;
    UpeStateHash_Begin(&hash, 0xD15EA5EULL);
    UpeStateHash_PushU32(&hash, (uint32_t)UpeWorld_GetFrameIndex(world));
    UpeStateHash_PushU32(&hash, (uint32_t)(UpeWorld_GetLastStateHash(world) & 0xFFFFFFFFu));
    UpeStateHash_PushU32(&hash, (uint32_t)(UpeWorld_GetLastStateHash(world) >> 32));

    REQUIRE(UpeStateHash_Final(&hash) != 0u);
    UpeWorld_Destroy(world);
}

TEST(TestDeterminismReplayRecorderHooks) {
    UpeWorld *world = MakeDeterministicWorld();
    REQUIRE(world != nullptr);

    UpeReplayRecorder recorder;
    UpeReplay_Init(&recorder, 16);
    UpeReplay_StartRecording(&recorder);
    UpeWorld_AttachReplayRecorder(world, &recorder, 1);

    for (int step = 0; step < 8; ++step) {
        UpeWorld_Step(world);
    }

    UpeWorld_AttachReplayRecorder(world, nullptr, 0);
    UpeReplay_StopRecording(&recorder);
    REQUIRE(recorder.event_count > 0u);
    REQUIRE(UpeReplay_GetEvent(&recorder, 0) != nullptr);
    UpeReplay_Destroy(&recorder);
    UpeWorld_Destroy(world);
}