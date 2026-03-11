#pragma once

#include <stdint.h>
#include "upe/upe_hash.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeReplayStream {
    UpeHash64 hash;
    uint64_t step;
} UpeReplayStream;

void UpeReplay_Begin(UpeReplayStream *stream, uint64_t seed);
void UpeReplay_NextStep(UpeReplayStream *stream);
void UpeReplay_PushU32(UpeReplayStream *stream, uint32_t value);
void UpeReplay_PushF32(UpeReplayStream *stream, float value);
uint64_t UpeReplay_Digest(const UpeReplayStream *stream);

#ifdef __cplusplus
}
#endif
