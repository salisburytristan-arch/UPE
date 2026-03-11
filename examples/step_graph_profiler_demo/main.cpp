#include "upe/upe_api.h"
#include "upe/upe_world_body_api.h"

#include <stdio.h>
#include <string.h>

struct StageAggregate {
    uint64_t frames;
    uint64_t duration_sum[UPE_STEP_STAGE_COUNT];
    uint64_t duration_max[UPE_STEP_STAGE_COUNT];
    uint32_t item_in_max[UPE_STEP_STAGE_COUNT];
    uint32_t item_out_max[UPE_STEP_STAGE_COUNT];
};

static const char *StageName(UpeStepStage stage) {
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

static void StepStageHook(const UpeStepStageStat *stat, void *user_data) {
    if (!stat || !user_data) {
        return;
    }
    StageAggregate *agg = (StageAggregate*)user_data;
    uint32_t idx = stat->stage;
    if (idx >= UPE_STEP_STAGE_COUNT) {
        return;
    }
    agg->duration_sum[idx] += stat->duration_us;
    if (stat->duration_us > agg->duration_max[idx]) {
        agg->duration_max[idx] = stat->duration_us;
    }
    if (stat->item_count_in > agg->item_in_max[idx]) {
        agg->item_in_max[idx] = stat->item_count_in;
    }
    if (stat->item_count_out > agg->item_out_max[idx]) {
        agg->item_out_max[idx] = stat->item_count_out;
    }
}

static UpeBodyDesc MakeBody(UpeBodyType type, float x, float y, float z, float mass) {
    UpeBodyDesc desc = {};
    desc.type = type;
    desc.position = {x, y, z};
    desc.rotation = UpeQuat_Identity();
    desc.mass = mass;
    desc.collision_layer = 1;
    desc.collision_mask = 0xFFFFFFFFu;
    return desc;
}

static UpeShapeDesc MakeSphere(uint32_t body_id, float radius) {
    UpeShapeDesc desc = {};
    desc.type = UPE_SHAPE_SPHERE;
    desc.body_id = body_id;
    desc.collision_layer = 0;
    desc.collision_mask = 0;
    desc.friction = 0.6f;
    desc.restitution = 0.1f;
    desc.sphere.center = {0.0f, 0.0f, 0.0f};
    desc.sphere.radius = radius;
    return desc;
}

static UpeShapeDesc MakeGroundBox(uint32_t body_id, float half_x, float half_y, float half_z) {
    UpeShapeDesc desc = {};
    desc.type = UPE_SHAPE_BOX;
    desc.body_id = body_id;
    desc.collision_layer = 0;
    desc.collision_mask = 0;
    desc.friction = 0.9f;
    desc.restitution = 0.0f;
    desc.box.min = {-half_x, -half_y, -half_z};
    desc.box.max = {half_x, half_y, half_z};
    return desc;
}

static void SpawnStack(UpeWorld *world, uint32_t rows, uint32_t cols, float spacing, float radius, float y_base) {
    if (!world) {
        return;
    }

    for (uint32_t r = 0; r < rows; ++r) {
        for (uint32_t c = 0; c < cols; ++c) {
            float x = ((float)c - (float)(cols - 1) * 0.5f) * spacing;
            float y = y_base + (float)r * spacing;
            float z = (((((r + c) & 1u) != 0u) ? 1.0f : 0.0f) - 0.5f) * radius * 0.3f;

            UpeBodyDesc bd = MakeBody(UPE_BODY_DYNAMIC, x, y, z, 1.0f);
            uint32_t body = UpeWorld_CreateBody(world, &bd);
            UpeShapeDesc sd = MakeSphere(body, radius);
            UpeWorld_CreateShape(world, &sd);

            UpeBody *b = UpeWorld_GetBody(world, body);
            if (b) {
                b->velocity.x = (float)(c % 3) * 0.02f;
                b->velocity.z = (float)(r % 2) * 0.02f;
            }
        }
    }
}

static void PrintPerFrameStats(const UpeWorld *world, uint32_t frame) {
    UpeStepStageStat stats[UPE_STEP_STAGE_COUNT] = {};
    uint32_t count = UpeWorld_GetLastStepStageStats(world, stats, UPE_STEP_STAGE_COUNT);
    printf("Frame %u | stageCount=%u\n", frame, count);
    for (uint32_t i = 0; i < count; ++i) {
        const UpeStepStageStat &s = stats[i];
        printf("  %-11s in=%6u out=%6u duration=%8llu us\n",
               StageName((UpeStepStage)s.stage),
               s.item_count_in,
               s.item_count_out,
               (unsigned long long)s.duration_us);
    }
}

static void PrintAggregate(const StageAggregate *agg) {
    if (!agg || agg->frames == 0) {
        return;
    }
    printf("\n=== Stage Aggregate (%llu frames) ===\n", (unsigned long long)agg->frames);
    for (uint32_t i = 0; i < UPE_STEP_STAGE_COUNT; ++i) {
        uint64_t avg = agg->duration_sum[i] / agg->frames;
        printf("  %-11s avg=%8llu us  max=%8llu us  maxIn=%6u  maxOut=%6u\n",
               StageName((UpeStepStage)i),
               (unsigned long long)avg,
               (unsigned long long)agg->duration_max[i],
               agg->item_in_max[i],
               agg->item_out_max[i]);
    }
}

int main() {
    printf("=== Step Graph Profiler Demo ===\n\n");

    UpeWorldDesc desc = {};
    desc.fixed_dt = 1.0f / 60.0f;
    desc.gravity[0] = 0.0f;
    desc.gravity[1] = -9.81f;
    desc.gravity[2] = 0.0f;
    desc.max_pairs = 32768;
    desc.max_contacts = 32768;
    desc.solver_iterations = 12;
    desc.enable_sleeping = 1;
    desc.enable_warm_starting = 1;
    desc.enable_ccd = 1;
    desc.ccd_threshold = 8.0f;

    UpeWorld *world = UpeWorld_Create(&desc);
    if (!world) {
        printf("Failed to create world.\n");
        return 1;
    }

    StageAggregate agg = {};
    UpeWorld_SetStepStageCallback(world, StepStageHook, &agg);

    UpeBodyDesc ground_bd = MakeBody(UPE_BODY_STATIC, 0.0f, -1.0f, 0.0f, 0.0f);
    uint32_t ground = UpeWorld_CreateBody(world, &ground_bd);
    UpeShapeDesc ground_sd = MakeGroundBox(ground, 30.0f, 1.0f, 30.0f);
    UpeWorld_CreateShape(world, &ground_sd);

    SpawnStack(world, 10, 16, 0.9f, 0.35f, 0.5f);

    const uint32_t total_frames = 240;
    for (uint32_t frame = 0; frame < total_frames; ++frame) {
        UpeWorld_Step(world);
        agg.frames++;

        if (frame < 5 || (frame % 60 == 0)) {
            PrintPerFrameStats(world, frame + 1);
        }
    }

    PrintAggregate(&agg);

    char json_text[8192];
    char csv_text[8192];
    uint32_t json_len = UpeWorld_FormatLastStepStatsJson(world, json_text, (uint32_t)sizeof(json_text));
    uint32_t csv_len = UpeWorld_FormatLastStepStatsCsv(world, csv_text, (uint32_t)sizeof(csv_text));

    printf("\n=== Last Step JSON (%u bytes) ===\n", json_len);
    printf("%s\n", json_text);

    printf("=== Last Step CSV (%u bytes) ===\n", csv_len);
    printf("%s\n", csv_text);

    uint64_t frame_index = UpeWorld_GetFrameIndex(world);
    uint64_t last_hash = UpeWorld_GetLastStateHash(world);
    printf("Final frame index: %llu\n", (unsigned long long)frame_index);
    printf("Final state hash:  0x%llx\n", (unsigned long long)last_hash);

    UpeWorld_Destroy(world);
    printf("\nDemo complete.\n");
    return 0;
}
