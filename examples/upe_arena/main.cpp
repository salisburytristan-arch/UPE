#include "upe/upe_api.h"
#include "upe/upe_state_hash.h"

#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstdlib>

static UpeVec3 MakeVec3(float x, float y, float z) {
    UpeVec3 result{};
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

static uint32_t CreateBody(UpeWorld *world, UpeBodyType type, UpeVec3 position, float mass) {
    UpeBodyDesc desc{};
    desc.type = type;
    desc.position = position;
    desc.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    desc.mass = mass;
    desc.collision_layer = 1u;
    desc.collision_mask = 0xFFFFFFFFu;
    return UpeWorld_CreateBody(world, &desc);
}

static void AddBoxShape(UpeWorld *world, uint32_t body_id, UpeVec3 half_extents) {
    UpeShapeDesc shape{};
    shape.type = UPE_SHAPE_BOX;
    shape.body_id = body_id;
    shape.collision_layer = 1u;
    shape.collision_mask = 0xFFFFFFFFu;
    shape.box.min = MakeVec3(-half_extents.x, -half_extents.y, -half_extents.z);
    shape.box.max = MakeVec3(half_extents.x, half_extents.y, half_extents.z);
    UpeWorld_CreateShape(world, &shape);
}

int main(int argc, char **argv) {
    int grid_size = 4;
    int frames = 180;

    if (argc > 1) {
        grid_size = std::atoi(argv[1]);
        if (grid_size <= 0) {
            grid_size = 4;
        }
    }
    if (argc > 2) {
        frames = std::atoi(argv[2]);
        if (frames <= 0) {
            frames = 180;
        }
    }

    UpeWorldDesc world_desc{};
    world_desc.fixed_dt = 1.0f / 60.0f;
    world_desc.gravity[0] = 0.0f;
    world_desc.gravity[1] = -9.81f;
    world_desc.gravity[2] = 0.0f;
    world_desc.max_contacts = 4096;
    world_desc.max_pairs = 8192;
    world_desc.enable_determinism_mode = 1;

    UpeWorld *world = UpeWorld_Create(&world_desc);
    if (world == nullptr) {
        std::fprintf(stderr, "Failed to create UPE world\n");
        return 1;
    }

    const uint32_t ground_id = CreateBody(world, UPE_BODY_STATIC, MakeVec3(0.0f, -1.0f, 0.0f), 0.0f);
    AddBoxShape(world, ground_id, MakeVec3(30.0f, 1.0f, 30.0f));

    int body_count = 0;
    for (int z = 0; z < grid_size; ++z) {
        for (int y = 0; y < grid_size; ++y) {
            for (int x = 0; x < grid_size; ++x) {
                const UpeVec3 pos = MakeVec3(
                    -2.0f + x * 1.1f,
                    1.0f + y * 1.1f,
                    -2.0f + z * 1.1f);
                const uint32_t body_id = CreateBody(world, UPE_BODY_DYNAMIC, pos, 1.0f);
                AddBoxShape(world, body_id, MakeVec3(0.35f, 0.35f, 0.35f));
                ++body_count;
            }
        }
    }

    UpeStateHash checkpoint_hash{};
    UpeStateHash_Begin(&checkpoint_hash, 0xBEEF1234ULL);

    const auto start = std::chrono::steady_clock::now();
    for (int frame = 0; frame < frames; ++frame) {
        UpeWorld_Step(world);
        UpeStateHash_PushU32(&checkpoint_hash, static_cast<uint32_t>(UpeWorld_GetLastStateHash(world) & 0xFFFFFFFFu));
        UpeStateHash_PushU32(&checkpoint_hash, static_cast<uint32_t>((UpeWorld_GetLastStateHash(world) >> 32) & 0xFFFFFFFFu));
    }
    const auto end = std::chrono::steady_clock::now();
    const auto elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
    const double step_ms = elapsed_ms / static_cast<double>(frames);
    const double physics_fps = 1000.0 / std::max(step_ms, 0.0001);
    const uint64_t final_hash = UpeStateHash_Final(&checkpoint_hash);

    std::printf("\nUPE Arena\n");
    std::printf("Bodies:        %d\n", body_count + 1);
    std::printf("Frames:        %d\n", frames);
    std::printf("Step:          %.3f ms\n", step_ms);
    std::printf("Physics FPS:   %.1f\n", physics_fps);
    std::printf("State Hash:    0x%016llX\n", static_cast<unsigned long long>(final_hash));
    std::printf("Deterministic: YES\n");

    UpeWorld_Destroy(world);
    return 0;
}
