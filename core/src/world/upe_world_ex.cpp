#include "upe/upe_world_ex.h"
#include "upe/upe_error.h"
#include "upe/upe_descriptor.h"
#include "upe/upe_telemetry.h"
#include "upe/upe_world_body_api.h"
#include "upe/upe_determinism.h"
#include "upe_world.h"
#include <vector>
#include <mutex>

typedef struct {
    struct UpeWorld *world;
    struct UpeTelemetryBuffer *buffer;
} UpeWorldTelemetryBinding;

static std::vector<UpeWorldTelemetryBinding> g_world_telemetry_bindings;
static std::mutex g_world_telemetry_mutex;

static int UpeWorldTelemetry_FindIndex(const struct UpeWorld *world) {
    for (size_t i = 0; i < g_world_telemetry_bindings.size(); ++i) {
        if (g_world_telemetry_bindings[i].world == world) {
            return (int)i;
        }
    }
    return -1;
}

extern "C" void UpeWorldEx_OnWorldDestroyed(const struct UpeWorld *world) {
    if (!world) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_world_telemetry_mutex);
    int idx = UpeWorldTelemetry_FindIndex(world);
    if (idx < 0) {
        return;
    }
    g_world_telemetry_bindings[(size_t)idx] = g_world_telemetry_bindings[g_world_telemetry_bindings.size() - 1];
    g_world_telemetry_bindings.pop_back();
}

struct UpeWorld *UpeWorld_CreateEx(const UpeWorldDesc *desc) {
    /* Validate descriptor */
#if UPE_ENABLE_DESCRIPTOR_UTILS
    UpeError err = UpeWorldDesc_Validate(desc);
    if (err != UPE_OK) {
        UpeError_Record(err, "UpeWorld_CreateEx", "world", __LINE__, 
                       UPE_SEVERITY_ERROR, "Invalid world descriptor");
        return NULL;
    }
#endif

    /* Call original creation */
    struct UpeWorld *world = UpeWorld_Create(desc);
    
    if (world == NULL) {
        UpeError_Record(UPE_ERR_INTERNAL_ALLOC, "UpeWorld_CreateEx", "world", 
                       __LINE__, UPE_SEVERITY_ERROR, 
                       "Failed to allocate world (out of memory?)");
        return NULL;
    }

    {
        std::lock_guard<std::mutex> lock(g_world_telemetry_mutex);
        if (UpeWorldTelemetry_FindIndex(world) < 0) {
            UpeWorldTelemetryBinding binding = {};
            binding.world = world;
            binding.buffer = NULL;
            g_world_telemetry_bindings.push_back(binding);
        }
    }

    return world;
}

uint32_t UpeWorld_CreateBodyEx(struct UpeWorld *world, const UpeBodyDesc_Ex *desc) {
    /* Validate inputs */
    if (world == NULL) {
        UpeError_Record(UPE_ERR_INVALID_ARGUMENT, "UpeWorld_CreateBodyEx", "body", 
                       __LINE__, UPE_SEVERITY_ERROR, "world pointer is NULL");
        return UINT32_MAX;
    }

    if (desc == NULL) {
        UpeError_Record(UPE_ERR_INVALID_ARGUMENT, "UpeWorld_CreateBodyEx", "body", 
                       __LINE__, UPE_SEVERITY_ERROR, "descriptor pointer is NULL");
        return UINT32_MAX;
    }

    /* Validate descriptor */
#if UPE_ENABLE_DESCRIPTOR_UTILS
    UpeError err = UpeBodyDesc_ExValidate(desc);
    if (err != UPE_OK) {
        UpeError_Record(err, "UpeWorld_CreateBodyEx", "body", __LINE__, 
                       UPE_SEVERITY_ERROR, "Invalid body descriptor");
        return UINT32_MAX;
    }
#endif

    UpeBodyDesc runtime_desc = {};
    runtime_desc.type = (desc->body_type <= (int)UPE_BODY_DYNAMIC)
                        ? (UpeBodyType)desc->body_type
                        : UPE_BODY_DYNAMIC;
    runtime_desc.position = {desc->position[0], desc->position[1], desc->position[2]};
    runtime_desc.rotation = {desc->rotation[0], desc->rotation[1], desc->rotation[2], desc->rotation[3]};
    runtime_desc.mass = desc->mass;
    runtime_desc.collision_layer = desc->collision_layer;
    runtime_desc.collision_mask = desc->collision_mask;

    /* Call original creation */
    uint32_t body_id = UpeWorld_CreateBody(world, &runtime_desc);
    
    if (body_id == UINT32_MAX) {
        UpeError_Record(UPE_ERR_RESOURCE_EXHAUSTED, "UpeWorld_CreateBodyEx", "body", 
                       __LINE__, UPE_SEVERITY_ERROR, 
                       "Failed to allocate body (hit capacity limit?)");
        return UINT32_MAX;
    }

    UpeBody *body = UpeWorld_GetBody(world, body_id);
    if (body) {
        body->velocity = {desc->linear_velocity[0], desc->linear_velocity[1], desc->linear_velocity[2]};
        body->angular_velocity = {desc->angular_velocity[0], desc->angular_velocity[1], desc->angular_velocity[2]};
    }

    return body_id;
}

uint32_t UpeWorld_CreateShapeEx(struct UpeWorld *world, const struct UpeShapeDesc *desc) {
    /* Validate inputs */
    if (world == NULL) {
        UpeError_Record(UPE_ERR_INVALID_ARGUMENT, "UpeWorld_CreateShapeEx", "shape", 
                       __LINE__, UPE_SEVERITY_ERROR, "world pointer is NULL");
        return UINT32_MAX;
    }

    if (desc == NULL) {
        UpeError_Record(UPE_ERR_INVALID_ARGUMENT, "UpeWorld_CreateShapeEx", "shape", 
                       __LINE__, UPE_SEVERITY_ERROR, "descriptor pointer is NULL");
        return UINT32_MAX;
    }

    /* Call original creation */
    uint32_t shape_id = UpeWorld_CreateShape(world, desc);
    
    if (shape_id == UINT32_MAX) {
        UpeError_Record(UPE_ERR_RESOURCE_EXHAUSTED, "UpeWorld_CreateShapeEx", "shape", 
                       __LINE__, UPE_SEVERITY_ERROR, 
                       "Failed to allocate shape (hit capacity limit?)");
        return UINT32_MAX;
    }

    return shape_id;
}

void UpeWorld_SetTelemetryBuffer(struct UpeWorld *world, struct UpeTelemetryBuffer *buffer) {
    if (world == NULL) {
        UpeError_Record(UPE_ERR_INVALID_ARGUMENT, "UpeWorld_SetTelemetryBuffer", "telemetry", 
                       __LINE__, UPE_SEVERITY_WARNING, "world pointer is NULL");
        return;
    }
    
    std::lock_guard<std::mutex> lock(g_world_telemetry_mutex);
    int idx = UpeWorldTelemetry_FindIndex(world);
    if (idx >= 0) {
        g_world_telemetry_bindings[(size_t)idx].buffer = buffer;
        return;
    }

    UpeWorldTelemetryBinding binding = {};
    binding.world = world;
    binding.buffer = buffer;
    g_world_telemetry_bindings.push_back(binding);
}

struct UpeTelemetryBuffer *UpeWorld_GetTelemetryBuffer(const struct UpeWorld *world) {
    if (world == NULL) {
        UpeError_Record(UPE_ERR_INVALID_ARGUMENT, "UpeWorld_GetTelemetryBuffer", "telemetry", 
                       __LINE__, UPE_SEVERITY_WARNING, "world pointer is NULL");
        return NULL;
    }
    
    std::lock_guard<std::mutex> lock(g_world_telemetry_mutex);
    int idx = UpeWorldTelemetry_FindIndex(world);
    if (idx < 0) {
        return NULL;
    }
    return g_world_telemetry_bindings[(size_t)idx].buffer;
}

