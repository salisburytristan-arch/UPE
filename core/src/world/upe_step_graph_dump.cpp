#include "upe/upe_step_graph_dump.h"

#include "upe/upe_api.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static uint32_t UpeDump_StrLen(const char *text) {
    if (!text) {
        return 0;
    }
    return (uint32_t)strlen(text);
}

static uint32_t UpeDump_AppendText(char *out_text, uint32_t max_len, uint32_t pos, const char *text) {
    if (!out_text || max_len == 0 || !text) {
        return pos;
    }
    uint32_t remaining = (pos < max_len) ? (max_len - pos) : 0;
    if (remaining == 0) {
        return pos;
    }
    uint32_t text_len = UpeDump_StrLen(text);
    uint32_t write_len = (text_len < (remaining - 1)) ? text_len : (remaining - 1);
    if (write_len > 0) {
        memcpy(out_text + pos, text, write_len);
    }
    pos += write_len;
    out_text[pos] = '\0';
    return pos;
}

static uint32_t UpeDump_AppendFormat(char *out_text, uint32_t max_len, uint32_t pos, const char *fmt, ...) {
    if (!out_text || max_len == 0 || !fmt) {
        return pos;
    }
    if (pos >= max_len) {
        return pos;
    }

    va_list args;
    va_start(args, fmt);
    int written = vsnprintf(out_text + pos, max_len - pos, fmt, args);
    va_end(args);

    if (written <= 0) {
        return pos;
    }

    uint32_t available = max_len - pos;
    if ((uint32_t)written >= available) {
        return max_len - 1;
    }
    return pos + (uint32_t)written;
}

static const char *UpeDump_StageNameConst(UpeStepStage stage) {
    switch (stage) {
        case UPE_STEP_STAGE_PRE_STEP: return "PreStep";
        case UPE_STEP_STAGE_BROADPHASE: return "Broadphase";
        case UPE_STEP_STAGE_NARROWPHASE: return "Narrowphase";
        case UPE_STEP_STAGE_ISLANDS: return "Islands";
        case UPE_STEP_STAGE_SOLVER: return "Solver";
        case UPE_STEP_STAGE_INTEGRATION: return "Integration";
        case UPE_STEP_STAGE_POST_STEP: return "PostStep";
        default: return "Unknown";
    }
}

uint32_t UpeStepGraph_StageName(UpeStepStage stage, char *out_name, uint32_t max_len) {
    if (!out_name || max_len == 0) {
        return 0;
    }
    const char *name = UpeDump_StageNameConst(stage);
    uint32_t name_len = UpeDump_StrLen(name);
    uint32_t write_len = (name_len < (max_len - 1)) ? name_len : (max_len - 1);
    memcpy(out_name, name, write_len);
    out_name[write_len] = '\0';
    return write_len;
}

uint32_t UpeWorld_FormatLastStepStatsJson(const UpeWorld *world, char *out_text, uint32_t max_len) {
    if (!out_text || max_len == 0) {
        return 0;
    }

    out_text[0] = '\0';
    UpeStepStageStat stats[UPE_STEP_STAGE_COUNT];
    uint32_t count = UpeWorld_GetLastStepStageStats(world, stats, UPE_STEP_STAGE_COUNT);

    uint32_t pos = 0;
    pos = UpeDump_AppendText(out_text, max_len, pos, "{\n");
    if (count > 0) {
        pos = UpeDump_AppendFormat(out_text, max_len, pos, "  \"frame\": %llu,\n", (unsigned long long)stats[0].frame_index);
    } else {
        pos = UpeDump_AppendText(out_text, max_len, pos, "  \"frame\": 0,\n");
    }
    pos = UpeDump_AppendFormat(out_text, max_len, pos, "  \"stageCount\": %u,\n", count);
    pos = UpeDump_AppendText(out_text, max_len, pos, "  \"stages\": [\n");

    for (uint32_t i = 0; i < count; ++i) {
        const UpeStepStageStat &s = stats[i];
        char name[32];
        UpeStepGraph_StageName((UpeStepStage)s.stage, name, (uint32_t)sizeof(name));

        pos = UpeDump_AppendText(out_text, max_len, pos, "    {\n");
        pos = UpeDump_AppendFormat(out_text, max_len, pos, "      \"index\": %u,\n", i);
        pos = UpeDump_AppendFormat(out_text, max_len, pos, "      \"stage\": %u,\n", s.stage);
        pos = UpeDump_AppendFormat(out_text, max_len, pos, "      \"name\": \"%s\",\n", name);
        pos = UpeDump_AppendFormat(out_text, max_len, pos, "      \"itemIn\": %u,\n", s.item_count_in);
        pos = UpeDump_AppendFormat(out_text, max_len, pos, "      \"itemOut\": %u,\n", s.item_count_out);
        pos = UpeDump_AppendFormat(out_text, max_len, pos, "      \"beginUs\": %llu,\n", (unsigned long long)s.begin_us);
        pos = UpeDump_AppendFormat(out_text, max_len, pos, "      \"endUs\": %llu,\n", (unsigned long long)s.end_us);
        pos = UpeDump_AppendFormat(out_text, max_len, pos, "      \"durationUs\": %llu\n", (unsigned long long)s.duration_us);
        if (i + 1u < count) {
            pos = UpeDump_AppendText(out_text, max_len, pos, "    },\n");
        } else {
            pos = UpeDump_AppendText(out_text, max_len, pos, "    }\n");
        }
    }

    pos = UpeDump_AppendText(out_text, max_len, pos, "  ]\n");
    pos = UpeDump_AppendText(out_text, max_len, pos, "}\n");
    return pos;
}

uint32_t UpeWorld_FormatLastStepStatsCsv(const UpeWorld *world, char *out_text, uint32_t max_len) {
    if (!out_text || max_len == 0) {
        return 0;
    }

    out_text[0] = '\0';
    UpeStepStageStat stats[UPE_STEP_STAGE_COUNT];
    uint32_t count = UpeWorld_GetLastStepStageStats(world, stats, UPE_STEP_STAGE_COUNT);

    uint32_t pos = 0;
    pos = UpeDump_AppendText(out_text, max_len, pos,
        "frame,stageIndex,stageId,stageName,itemIn,itemOut,beginUs,endUs,durationUs\n");

    for (uint32_t i = 0; i < count; ++i) {
        const UpeStepStageStat &s = stats[i];
        char name[32];
        UpeStepGraph_StageName((UpeStepStage)s.stage, name, (uint32_t)sizeof(name));

        pos = UpeDump_AppendFormat(out_text,
                                   max_len,
                                   pos,
                                   "%llu,%u,%u,%s,%u,%u,%llu,%llu,%llu\n",
                                   (unsigned long long)s.frame_index,
                                   i,
                                   s.stage,
                                   name,
                                   s.item_count_in,
                                   s.item_count_out,
                                   (unsigned long long)s.begin_us,
                                   (unsigned long long)s.end_us,
                                   (unsigned long long)s.duration_us);
    }

    return pos;
}
