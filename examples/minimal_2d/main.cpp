#include "upe/upe_api.h"
#include "upe/upe_replay.h"
#include "upe/upe_state_hash.h"
#include <stdio.h>

int main() {
    UpeWorldDesc desc{};
    desc.fixed_dt = 1.0f / 60.0f;
    UpeWorld *world = UpeWorld_Create(&desc);
    UpeReplayStream replay;
    UpeReplay_Begin(&replay, 0x1234);

    UpeStateHash state_hash;
    UpeStateHash_Begin(&state_hash, 0xBEEF);

    for (int i = 0; i < 60; ++i) {
        UpeWorld_Step(world);
        UpeReplay_NextStep(&replay);
        UpeReplay_PushF32(&replay, UpeWorld_Time(world));
        UpeStateHash_PushF32(&state_hash, UpeWorld_Time(world));
    }
    printf("World time: %.3f\n", UpeWorld_Time(world));
    printf("Replay digest: 0x%llx\n", (unsigned long long)UpeReplay_Digest(&replay));
    printf("State hash: 0x%llx\n", (unsigned long long)UpeStateHash_Final(&state_hash));
    UpeWorld_Destroy(world);
    return 0;
}
