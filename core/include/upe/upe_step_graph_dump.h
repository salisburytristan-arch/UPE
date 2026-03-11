#pragma once

#include <stdint.h>
#include "upe/upe_step_graph.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t UpeStepGraph_StageName(UpeStepStage stage, char *out_name, uint32_t max_len);
uint32_t UpeWorld_FormatLastStepStatsJson(const UpeWorld *world, char *out_text, uint32_t max_len);
uint32_t UpeWorld_FormatLastStepStatsCsv(const UpeWorld *world, char *out_text, uint32_t max_len);

#ifdef __cplusplus
}
#endif
