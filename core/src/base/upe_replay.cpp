#include "upe/upe_replay.h"

void UpeReplay_Begin(UpeReplayStream *stream, uint64_t seed) {
    if (!stream) {
        return;
    }
    UpeHash64_Init(&stream->hash, seed);
    stream->step = 0;
}

void UpeReplay_NextStep(UpeReplayStream *stream) {
    if (!stream) {
        return;
    }
    stream->step++;
    UpeHash64_UpdateU32(&stream->hash, (uint32_t)stream->step);
    UpeHash64_UpdateU32(&stream->hash, (uint32_t)(stream->step >> 32));
}

void UpeReplay_PushU32(UpeReplayStream *stream, uint32_t value) {
    if (!stream) {
        return;
    }
    UpeHash64_UpdateU32(&stream->hash, value);
}

void UpeReplay_PushF32(UpeReplayStream *stream, float value) {
    if (!stream) {
        return;
    }
    UpeHash64_UpdateF32(&stream->hash, value);
}

uint64_t UpeReplay_Digest(const UpeReplayStream *stream) {
    return stream ? UpeHash64_Final(&stream->hash) : 0ULL;
}
