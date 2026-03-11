#pragma once

#include <stdint.h>
#include "upe/upe_replay.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeReplayFile {
    void *handle;
    UpeReplayStream stream;
    uint32_t mode;      /* 0 = closed, 1 = write, 2 = read */
    uint32_t frame_count;
    uint64_t seed;
    uint64_t expected_digest;
} UpeReplayFile;

int UpeReplayFile_BeginWrite(UpeReplayFile *file, const char *path, uint64_t seed);
int UpeReplayFile_WriteStep(UpeReplayFile *file, uint64_t step, const uint32_t *inputs, uint32_t input_count);
int UpeReplayFile_EndWrite(UpeReplayFile *file, uint64_t final_digest);

int UpeReplayFile_BeginRead(UpeReplayFile *file, const char *path, uint64_t *out_seed);
int UpeReplayFile_ReadStep(UpeReplayFile *file, uint64_t *out_step, uint32_t *out_inputs, uint32_t max_inputs, uint32_t *out_input_count);
int UpeReplayFile_EndRead(UpeReplayFile *file, uint64_t *out_final_digest);

void UpeReplayFile_Close(UpeReplayFile *file);

#ifdef __cplusplus
}
#endif
