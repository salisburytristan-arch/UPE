#include "test_framework.h"
#include "upe/upe_determinism.h"

TEST(TestReplayEvents) {
    UpeReplayRecorder recorder;
    UpeReplay_Init(&recorder, 16);
    UpeReplay_StartRecording(&recorder);

    float input_data[2] = {1.0f, -2.0f};
    UpeReplay_RecordInput(&recorder, 5, 42, input_data, 2);
    UpeReplay_RecordSpawn(&recorder, 6, 7, input_data, 1);
    UpeReplay_RecordImpulse(&recorder, 7, 99, {0.5f, 1.0f, -0.25f});
    UpeReplay_RecordDestroy(&recorder, 8, 7);

    UpeReplay_StopRecording(&recorder);

    REQUIRE(recorder.event_count == 4);
    REQUIRE(recorder.events[0].type == UPE_EVENT_INPUT);
    REQUIRE(recorder.events[1].type == UPE_EVENT_SPAWN);
    REQUIRE(recorder.events[2].type == UPE_EVENT_IMPULSE);
    REQUIRE(recorder.events[3].type == UPE_EVENT_DESTROY);

    UpeReplay_Destroy(&recorder);
}
