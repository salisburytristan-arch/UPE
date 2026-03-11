#include "test_framework.h"
#include "upe/upe_api.h"
#include "upe/upe_determinism.h"

TEST(TestReplayApplyWorld) {
    UpeReplayRecorder recorder;
    UpeReplay_Init(&recorder, 8);
    UpeReplay_StartRecording(&recorder);

    float spawn_data[8] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f};
    UpeReplay_RecordSpawn(&recorder, 0, 0, spawn_data, 8);

    float force_data[4] = {1.0f, 0.0f, 0.0f, 0.0f};
    UpeReplay_RecordInput(&recorder, 0, 0, force_data, 4);

    UpeReplay_RecordImpulse(&recorder, 0, 0, {0.0f, 2.0f, 0.0f});
    UpeReplay_StopRecording(&recorder);

    UpeWorldDesc desc = {};
    desc.fixed_dt = 1.0f / 60.0f;
    UpeWorld *world = UpeWorld_Create(&desc);
    REQUIRE(world != nullptr);

    UpeReplay_ApplyToWorld(world, &recorder, 0);

    UpeBody *body = UpeWorld_GetBody(world, 0);
    REQUIRE(body != nullptr);
    REQUIRE(body->force.x == 1.0f);
    REQUIRE(body->velocity.y > 1.9f);

    UpeWorld_Destroy(world);
    UpeReplay_Destroy(&recorder);
}
