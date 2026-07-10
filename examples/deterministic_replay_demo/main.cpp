#include "upe/upe_api.h"
#include "upe/upe_determinism.h"
#include "upe/upe_replay.h"
#include "upe/upe_state_hash.h"
#include <cstdio>

int main() {
    UpeWorldDesc desc = UpeWorldDesc_Default();
    desc.fixed_dt = 1.0f / 60.0f;
    desc.enable_determinism_mode = 1;

    UpeWorld *world = UpeWorld_Create(&desc);
    if (!world) {
        std::printf("Failed to create world\n");
        return 1;
    }

    UpeReplayRecorder recorder;
    UpeReplay_Init(&recorder, 64);
    UpeReplay_StartRecording(&recorder);
    UpeWorld_AttachReplayRecorder(world, &recorder, 1);

    UpeReplayStream stream;
    UpeReplay_Begin(&stream, 0xD1A5EEDULL);

    for (int i = 0; i < 120; ++i) {
        UpeWorld_Step(world);
        UpeReplay_NextStep(&stream);
        UpeReplay_PushU32(&stream, (uint32_t)UpeWorld_GetFrameIndex(world));
        uint64_t world_hash = UpeWorld_GetLastStateHash(world);
        UpeReplay_PushU32(&stream, (uint32_t)(world_hash & 0xFFFFFFFFu));
        UpeReplay_PushU32(&stream, (uint32_t)(world_hash >> 32));
    }

    UpeWorld_AttachReplayRecorder(world, nullptr, 0);
    UpeReplay_StopRecording(&recorder);

    UpeStateHash hash;
    UpeStateHash_Begin(&hash, 0xD1A5EEDULL);
    UpeStateHash_PushU32(&hash, (uint32_t)UpeWorld_GetFrameIndex(world));
    UpeStateHash_PushU32(&hash, (uint32_t)(UpeWorld_GetLastStateHash(world) & 0xFFFFFFFFu));
    UpeStateHash_PushU32(&hash, (uint32_t)(UpeWorld_GetLastStateHash(world) >> 32));

    std::printf("Frame: %llu\n", (unsigned long long)UpeWorld_GetFrameIndex(world));
    std::printf("Replay events: %u\n", recorder.event_count);
    std::printf("Replay digest: 0x%llX\n", (unsigned long long)UpeReplay_Digest(&stream));
    std::printf("Final-state transform hash: 0x%llX\n", (unsigned long long)UpeStateHash_Final(&hash));

    UpeReplay_Destroy(&recorder);
    UpeWorld_Destroy(world);
    return 0;
}