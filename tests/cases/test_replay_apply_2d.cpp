#include "test_framework.h"
#include "upe/upe_2d_physics.h"
#include "upe/upe_determinism.h"

TEST(TestReplayApplyWorld2D) {
    UpeReplayRecorder recorder;
    UpeReplay_Init(&recorder, 8);
    UpeReplay_StartRecording(&recorder);

    float spawn_data[5] = {0.0f, 0.0f, 0.0f, (float)UPE_2D_BODY_DYNAMIC, 1.0f};
    UpeReplay_RecordSpawn(&recorder, 0, 0, spawn_data, 5);

    float force_data[4] = {2.0f, 0.0f, 0.0f, 0.0f};
    UpeReplay_RecordInput(&recorder, 0, 0, force_data, 4);

    UpeReplay_StopRecording(&recorder);

    Upe2DWorld world = {};
    Upe2DWorld_Init(&world, {0.0f, -9.8f});

    UpeReplay_ApplyToWorld2D(&world, &recorder, 0);

    Upe2DBody *body = Upe2DWorld_GetBody(&world, 0);
    REQUIRE(body != nullptr);
    REQUIRE(body->force.x == 2.0f);

    Upe2DWorld_Destroy(&world);
    UpeReplay_Destroy(&recorder);
}
