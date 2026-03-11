#include "test_framework.h"
#include "upe/upe_api.h"
#include "upe/upe_determinism.h"

TEST(TestReplayWorldInputs) {
    UpeWorldDesc desc = {};
    desc.fixed_dt = 1.0f / 60.0f;

    UpeWorld *world = UpeWorld_Create(&desc);
    REQUIRE(world != nullptr);

    UpeReplayRecorder recorder;
    UpeReplay_Init(&recorder, 8);
    UpeReplay_StartRecording(&recorder);
    UpeWorld_SetReplayRecorder(world, &recorder);

    UpeBodyDesc body_desc = {};
    body_desc.type = UPE_BODY_DYNAMIC;
    body_desc.position = {0.0f, 0.0f, 0.0f};
    body_desc.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    body_desc.mass = 1.0f;
    body_desc.collision_layer = 1u;
    body_desc.collision_mask = 0xFFFFFFFFu;

    uint32_t body_id = UpeWorld_CreateBody(world, &body_desc);
    REQUIRE(body_id != 0xFFFFFFFFu);

    UpeWorld_ApplyForce(world, body_id, {1.0f, 0.0f, 0.0f});
    UpeWorld_ApplyImpulse(world, body_id, {0.0f, 2.0f, 0.0f});

    UpeReplay_StopRecording(&recorder);

    REQUIRE(recorder.event_count == 3);
    REQUIRE(recorder.events[0].type == UPE_EVENT_SPAWN);
    REQUIRE(recorder.events[1].type == UPE_EVENT_INPUT);
    REQUIRE(recorder.events[2].type == UPE_EVENT_IMPULSE);

    UpeWorld_Destroy(world);
    UpeReplay_Destroy(&recorder);
}