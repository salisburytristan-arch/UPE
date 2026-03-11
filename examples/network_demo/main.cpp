#include <stdio.h>
#include <string.h>
#include "upe/upe_api.h"
#include "upe/upe_determinism.h"

typedef struct InputFrame {
    UpeVec3 predicted_force;
    UpeVec3 authoritative_force;
    int has_authoritative;
} InputFrame;

static UpeVec3 MakeForce(float x, float y, float z) {
    UpeVec3 out;
    out.x = x;
    out.y = y;
    out.z = z;
    return out;
}

static int ForceDiffers(UpeVec3 a, UpeVec3 b, float eps) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return (dx < -eps || dx > eps || dy < -eps || dy > eps || dz < -eps || dz > eps);
}

static UpeVec3 GetLocalInput(uint32_t frame) {
    return (frame < 30u) ? MakeForce(1.0f, 0.0f, 0.0f) : MakeForce(-1.0f, 0.0f, 0.0f);
}

static UpeVec3 GetAuthoritativeInput(uint32_t frame) {
    return (frame < 30u) ? MakeForce(1.0f, 0.0f, 0.0f) : MakeForce(0.5f, 0.0f, 0.0f);
}

static uint32_t CreateWorldWithBody(UpeWorld **out_world, float fixed_dt) {
    UpeWorldDesc desc = {};
    desc.fixed_dt = fixed_dt;

    UpeWorld *world = UpeWorld_Create(&desc);
    if (!world) {
        *out_world = NULL;
        return 0xFFFFFFFFu;
    }

    UpeBodyDesc body_desc = {};
    body_desc.type = UPE_BODY_DYNAMIC;
    body_desc.position = {0.0f, 0.0f, 0.0f};
    body_desc.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    body_desc.mass = 1.0f;
    body_desc.collision_layer = 1u;
    body_desc.collision_mask = 0xFFFFFFFFu;

    uint32_t body_id = UpeWorld_CreateBody(world, &body_desc);
    *out_world = world;
    return body_id;
}

static void SimulateToFrame(UpeWorld *world, uint32_t body_id, const InputFrame *inputs,
                            uint32_t frame_count) {
    for (uint32_t frame = 0; frame < frame_count; ++frame) {
        UpeVec3 force = inputs[frame].has_authoritative
            ? inputs[frame].authoritative_force
            : inputs[frame].predicted_force;
        UpeWorld_ApplyForce(world, body_id, force);
        UpeWorld_Step(world);
    }
}

static void SimulateAuthoritativeToFrame(UpeWorld *world, uint32_t body_id, uint32_t frame_count) {
    for (uint32_t frame = 0; frame < frame_count; ++frame) {
        UpeVec3 force = GetAuthoritativeInput(frame);
        UpeWorld_ApplyForce(world, body_id, force);
        UpeWorld_Step(world);
    }
}

int main() {
    const float fixed_dt = 1.0f / 60.0f;
    const uint32_t total_frames = 120;
    const uint32_t input_latency = 6;
    const float diff_epsilon = 0.0001f;

    InputFrame inputs[256];
    memset(inputs, 0, sizeof(inputs));

    UpeWorld *client_world = NULL;
    UpeWorld *server_world = NULL;
    uint32_t client_body = CreateWorldWithBody(&client_world, fixed_dt);
    uint32_t server_body = CreateWorldWithBody(&server_world, fixed_dt);
    if (!client_world || !server_world) {
        if (client_world) {
            UpeWorld_Destroy(client_world);
        }
        if (server_world) {
            UpeWorld_Destroy(server_world);
        }
        return 1;
    }

    uint32_t rollback_count = 0;
    for (uint32_t frame = 0; frame < total_frames; ++frame) {
        UpeVec3 predicted = GetLocalInput(frame);
        inputs[frame].predicted_force = predicted;

        UpeWorld_ApplyForce(client_world, client_body, predicted);
        UpeWorld_Step(client_world);

        UpeVec3 authoritative = GetAuthoritativeInput(frame);
        UpeWorld_ApplyForce(server_world, server_body, authoritative);
        UpeWorld_Step(server_world);

        if (frame >= input_latency) {
            uint32_t auth_frame = frame - input_latency;
            UpeVec3 auth_force = GetAuthoritativeInput(auth_frame);
            if (!inputs[auth_frame].has_authoritative) {
                inputs[auth_frame].authoritative_force = auth_force;
                inputs[auth_frame].has_authoritative = 1;

                if (ForceDiffers(inputs[auth_frame].predicted_force, auth_force, diff_epsilon)) {
                    rollback_count++;
                    UpeWorld_Destroy(client_world);
                    client_body = CreateWorldWithBody(&client_world, fixed_dt);
                    if (!client_world) {
                        break;
                    }
                    SimulateToFrame(client_world, client_body, inputs, frame + 1);
                }
            }
        }
    }

    UpeBody *client = UpeWorld_GetBody(client_world, client_body);
    UpeBody *server = UpeWorld_GetBody(server_world, server_body);

    uint32_t fully_authoritative_frames = total_frames;
    if (input_latency < total_frames) {
        fully_authoritative_frames = total_frames - input_latency;
    }

    UpeWorld *client_authoritative_world = NULL;
    UpeWorld *server_authoritative_world = NULL;
    uint32_t client_authoritative_body = CreateWorldWithBody(&client_authoritative_world, fixed_dt);
    uint32_t server_authoritative_body = CreateWorldWithBody(&server_authoritative_world, fixed_dt);

    int authoritative_hash_match = 0;
    if (client_authoritative_world && server_authoritative_world) {
        SimulateAuthoritativeToFrame(
            client_authoritative_world,
            client_authoritative_body,
            fully_authoritative_frames
        );
        SimulateAuthoritativeToFrame(
            server_authoritative_world,
            server_authoritative_body,
            fully_authoritative_frames
        );

        UpeStateHash client_authoritative_hash = UpeDeterminism_HashState(client_authoritative_world);
        UpeStateHash server_authoritative_hash = UpeDeterminism_HashState(server_authoritative_world);
        authoritative_hash_match = UpeDeterminism_CompareHash(
            client_authoritative_hash,
            server_authoritative_hash
        );
    }

    if (client && server) {
         printf("Rollback demo: rollbacks=%u hash_match_at_authoritative_frame=%s authoritative_frames=%u\n",
             rollback_count,
             authoritative_hash_match ? "yes" : "no",
             fully_authoritative_frames);
        printf("Final position: client=(%.3f, %.3f, %.3f) server=(%.3f, %.3f, %.3f)\n",
               client->position.x, client->position.y, client->position.z,
               server->position.x, server->position.y, server->position.z);
         if (fully_authoritative_frames < total_frames) {
             printf("Note: final frame comparison omits %u predicted-latency frame(s).\n",
                 total_frames - fully_authoritative_frames);
         }
    }

        if (client_authoritative_world) {
            UpeWorld_Destroy(client_authoritative_world);
        }
        if (server_authoritative_world) {
            UpeWorld_Destroy(server_authoritative_world);
        }
    UpeWorld_Destroy(client_world);
    UpeWorld_Destroy(server_world);
    return 0;
}
