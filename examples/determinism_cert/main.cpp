#include <stdio.h>
#include <stdlib.h>
#include "upe/upe_api.h"
#include "upe/upe_determinism.h"
#include "upe/upe_state_hash.h"

static UpeVec3 MakeVec3(float x, float y, float z) {
    UpeVec3 out;
    out.x = x;
    out.y = y;
    out.z = z;
    return out;
}

static void AddBoxShape(UpeWorld *world, uint32_t body_id, UpeVec3 half_extents) {
    UpeShapeDesc shape_desc = {};
    shape_desc.type = UPE_SHAPE_BOX;
    shape_desc.body_id = body_id;
    shape_desc.collision_layer = 1u;
    shape_desc.collision_mask = 0xFFFFFFFFu;
    shape_desc.box.min = MakeVec3(-half_extents.x, -half_extents.y, -half_extents.z);
    shape_desc.box.max = MakeVec3(half_extents.x, half_extents.y, half_extents.z);
    UpeWorld_CreateShape(world, &shape_desc);
}

static uint32_t CreateBody(UpeWorld *world, UpeBodyType type, UpeVec3 position, float mass) {
    UpeBodyDesc desc = {};
    desc.type = type;
    desc.position = position;
    desc.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    desc.mass = mass;
    desc.collision_layer = 1u;
    desc.collision_mask = 0xFFFFFFFFu;
    return UpeWorld_CreateBody(world, &desc);
}

int main(int argc, char **argv) {
    int frames = 10000;
    int sample_interval = 100;
    int grid = 4;

    if (argc > 1) {
        frames = atoi(argv[1]);
        if (frames <= 0) {
            frames = 10000;
        }
    }
    if (argc > 2) {
        sample_interval = atoi(argv[2]);
        if (sample_interval <= 0) {
            sample_interval = 100;
        }
    }
    if (argc > 3) {
        grid = atoi(argv[3]);
        if (grid <= 0) {
            grid = 4;
        }
    }

    UpeWorldDesc world_desc = {};
    world_desc.fixed_dt = 1.0f / 60.0f;
    UpeWorld *world = UpeWorld_Create(&world_desc);
    if (!world) {
        return 1;
    }

    uint32_t ground_id = CreateBody(world, UPE_BODY_STATIC, MakeVec3(0.0f, -1.0f, 0.0f), 0.0f);
    AddBoxShape(world, ground_id, MakeVec3(50.0f, 1.0f, 50.0f));

    int body_count = grid * grid * grid;
    uint32_t *body_ids = (uint32_t *)malloc(sizeof(uint32_t) * (size_t)body_count);
    if (!body_ids) {
        UpeWorld_Destroy(world);
        return 1;
    }

    int index = 0;
    for (int z = 0; z < grid; ++z) {
        for (int y = 0; y < grid; ++y) {
            for (int x = 0; x < grid; ++x) {
                UpeVec3 pos = MakeVec3((float)x * 1.1f, 0.5f + (float)y * 1.1f, (float)z * 1.1f);
                uint32_t body_id = CreateBody(world, UPE_BODY_DYNAMIC, pos, 1.0f);
                AddBoxShape(world, body_id, MakeVec3(0.45f, 0.45f, 0.45f));
                body_ids[index++] = body_id;
            }
        }
    }

    UpeStateHash cert_hash = {};
    UpeStateHash_Begin(&cert_hash, 0xC0FFEEULL);

    for (int frame = 0; frame < frames; ++frame) {
        if ((frame & 7) == 0) {
            for (int i = 0; i < body_count; ++i) {
                float sign_x = (i & 1) ? 1.0f : -1.0f;
                float sign_z = (i & 2) ? 1.0f : -1.0f;
                UpeVec3 force = MakeVec3(sign_x * 2.0f, 0.0f, sign_z * 1.5f);
                UpeWorld_ApplyForce(world, body_ids[i], force);
            }
        }

        UpeWorld_Step(world);

        if ((frame % sample_interval) == 0) {
            UpeStateHash frame_hash = UpeDeterminism_HashState(world);
            UpeStateHash_PushBytes(&cert_hash, &frame_hash, (uint32_t)sizeof(frame_hash));
            printf("FRAME %d HASH 0x%016llX\n",
                   frame,
                   (unsigned long long)frame_hash.state);
        }
    }

    uint64_t final_hash = UpeStateHash_Final(&cert_hash);
    printf("CERT_HASH 0x%llX\n", (unsigned long long)final_hash);

    free(body_ids);
    UpeWorld_Destroy(world);
    return 0;
}
