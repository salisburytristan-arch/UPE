#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeWorld UpeWorld;

typedef enum UpeStepStage {
    UPE_STEP_STAGE_PRE_STEP = 0,
    UPE_STEP_STAGE_BROADPHASE = 1,
    UPE_STEP_STAGE_NARROWPHASE = 2,
    UPE_STEP_STAGE_ISLANDS = 3,
    UPE_STEP_STAGE_SOLVER = 4,
    UPE_STEP_STAGE_INTEGRATION = 5,
    UPE_STEP_STAGE_POST_STEP = 6,
    UPE_STEP_STAGE_COUNT = 7
} UpeStepStage;

typedef struct UpeStepStageStat {
    uint64_t frame_index;
    uint32_t stage;
    uint32_t item_count_in;
    uint32_t item_count_out;
    uint64_t begin_us;
    uint64_t end_us;
    uint64_t duration_us;
} UpeStepStageStat;

typedef void (*UpeStepStageCallback)(const UpeStepStageStat *stat, void *user_data);

void UpeWorld_SetStepStageCallback(UpeWorld *world, UpeStepStageCallback callback, void *user_data);
uint32_t UpeWorld_GetLastStepStageStats(const UpeWorld *world, UpeStepStageStat *out_stats, uint32_t max_stats);

#ifdef __cplusplus
}
#endif
