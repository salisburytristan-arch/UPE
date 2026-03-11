#pragma once

#include <stdint.h>
#include "upe/upe_math.h"
#include "upe/upe_state_hash.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Determinism Framework
  
  Ensures bitwise-identical simulation results across:
  - Different runs on same machine
  - Different machines (x86, ARM, etc.)
  - Different compilers
  - Network multiplayer (rollback netcode)
*/

// Floating-point control
typedef enum UpeFpMode {
    UPE_FP_PRECISE,              // Full IEEE-754 precision
    UPE_FP_FAST,                 // Allow FMA, reordering (NON-deterministic)
    UPE_FP_DETERMINISTIC         // Strict ordering, no FMA, flush denormals
} UpeFpMode;

void UpeDeterminism_SetFpMode(UpeFpMode mode);
void UpeDeterminism_FlushDenormals(int enable);

// State hashing (for rollback verification)
// Note: UpeStateHash now defined in upe_state_hash.h
UpeStateHash UpeDeterminism_HashState(const void *world);
int UpeDeterminism_CompareHash(UpeStateHash a, UpeStateHash b);

// Deterministic random number generator
typedef struct UpeDeterministicRng {
    uint64_t state;
    uint64_t increment;
} UpeDeterministicRng;

void UpeRng_Init(UpeDeterministicRng *rng, uint64_t seed);
uint32_t UpeRng_Next(UpeDeterministicRng *rng);
float UpeRng_Float(UpeDeterministicRng *rng); // [0, 1)
float UpeRng_FloatRange(UpeDeterministicRng *rng, float min, float max);

// Frame counter (synchronized time across network)
typedef struct UpeFrameClock {
    uint64_t frame;              // Monotonic frame counter
    float fixed_dt;              // Fixed timestep
    double accumulated_time;     // For interpolation
} UpeFrameClock;

void UpeFrameClock_Init(UpeFrameClock *clock, float fixed_dt);
void UpeFrameClock_Advance(UpeFrameClock *clock);
uint64_t UpeFrameClock_GetFrame(const UpeFrameClock *clock);
float UpeFrameClock_GetInterpolation(const UpeFrameClock *clock);

// Replay recording (event stream)
typedef enum UpeEventType {
    UPE_EVENT_INPUT,
    UPE_EVENT_SPAWN,
    UPE_EVENT_DESTROY,
    UPE_EVENT_IMPULSE,
    UPE_EVENT_CUSTOM
} UpeEventType;

typedef struct UpeEvent {
    uint64_t frame;
    UpeEventType type;
    uint32_t entity_id;
    float data[8];               // Event-specific data
} UpeEvent;

typedef struct UpeReplayRecorder {
    UpeEvent *events;
    uint32_t event_count;
    uint32_t event_capacity;
    int recording;
} UpeReplayRecorder;

struct UpeWorld;
struct Upe2DWorld;

void UpeReplay_Init(UpeReplayRecorder *replay, uint32_t max_events);
void UpeReplay_Destroy(UpeReplayRecorder *replay);
void UpeReplay_StartRecording(UpeReplayRecorder *replay);
void UpeReplay_StopRecording(UpeReplayRecorder *replay);
void UpeReplay_RecordEvent(UpeReplayRecorder *replay, const UpeEvent *event);
const UpeEvent* UpeReplay_GetEvent(const UpeReplayRecorder *replay, uint32_t index);

void UpeReplay_RecordInput(UpeReplayRecorder *replay, uint64_t frame, uint32_t entity_id,
                           const float *data, uint32_t data_count);
void UpeReplay_RecordSpawn(UpeReplayRecorder *replay, uint64_t frame, uint32_t entity_id,
                           const float *data, uint32_t data_count);
void UpeReplay_RecordDestroy(UpeReplayRecorder *replay, uint64_t frame, uint32_t entity_id);
void UpeReplay_RecordImpulse(UpeReplayRecorder *replay, uint64_t frame, uint32_t entity_id,
                             UpeVec3 impulse);
void UpeReplay_ApplyToWorld(struct UpeWorld *world, const UpeReplayRecorder *replay, uint64_t frame);
void UpeReplay_ApplyToWorld2D(struct Upe2DWorld *world, const UpeReplayRecorder *replay, uint64_t frame);

// Cross-platform verification
int UpeDeterminism_VerifyPlatform(void);  // Returns 1 if platform is deterministic

#ifdef __cplusplus
}
#endif
