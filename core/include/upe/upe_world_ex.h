#pragma once

#include <stdint.h>
#include "upe/upe_descriptor.h"

#ifdef __cplusplus
extern "C" {
#endif

struct UpeReplayRecorder;

/**
 * Error-aware world creation (replaces UpeWorld_Create)
 * Returns world pointer or NULL on failure
 * Error details available via UpeError_GetLastError()
 */
struct UpeWorld *UpeWorld_CreateEx(const UpeWorldDesc *desc);

/**
 * Error-aware body creation (replaces UpeWorld_CreateBody)
 * Returns body ID or UINT32_MAX on failure
 */
uint32_t UpeWorld_CreateBodyEx(struct UpeWorld *world, const UpeBodyDesc_Ex *desc);

/**
 * Error-aware shape creation (replaces UpeWorld_CreateShape)
 */
uint32_t UpeWorld_CreateShapeEx(struct UpeWorld *world, const struct UpeShapeDesc *desc);

/**
 * Register telemetry buffer with a world instance
 * Telemetry events are recorded throughout the step
 * If buffer is NULL, disables telemetry for this world
 */
void UpeWorld_SetTelemetryBuffer(struct UpeWorld *world, struct UpeTelemetryBuffer *buffer);

/**
 * Get telemetry buffer associated with a world
 * Returns NULL if none registered
 */
struct UpeTelemetryBuffer *UpeWorld_GetTelemetryBuffer(const struct UpeWorld *world);

/**
 * Attach/detach replay recorder for deterministic input playback and frame hash logging.
 * When enable_apply != 0, recorded events for current frame are applied before each world step.
 */
void UpeWorld_AttachReplayRecorder(struct UpeWorld *world, struct UpeReplayRecorder *replay, int enable_apply);

/**
 * Get replay recorder currently attached to world (may be NULL).
 */
struct UpeReplayRecorder *UpeWorld_GetReplayRecorder(const struct UpeWorld *world);

/**
 * Get current world frame index.
 */
uint64_t UpeWorld_GetFrameIndex(const struct UpeWorld *world);

/**
 * Get last deterministic state hash captured at end of most recent step.
 */
uint64_t UpeWorld_GetLastStateHash(const struct UpeWorld *world);

#ifdef __cplusplus
}
#endif
