#include "test_framework.h"
#include "upe/upe_replay.h"
#include "upe/upe_rng.h"

TEST(TestReplayDigest) {
    UpeReplayStream stream;
    UpeRng rng;
    UpeRng_Seed(&rng, 99);

    UpeReplay_Begin(&stream, 0xBEEF);
    for (int step = 0; step < 8; ++step) {
        UpeReplay_NextStep(&stream);
        for (int i = 0; i < 4; ++i) {
            UpeReplay_PushU32(&stream, UpeRng_NextU32(&rng));
        }
    }

    uint64_t digest = UpeReplay_Digest(&stream);
    REQUIRE(digest != 0ULL);
}
