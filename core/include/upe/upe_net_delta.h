#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeNetDelta {
    uint32_t frame;
    uint8_t *data;
    uint32_t size;
} UpeNetDelta;

UpeNetDelta *UpeNetDelta_Create(uint32_t frame, const void *data, uint32_t size);
UpeNetDelta *UpeNetDelta_CreateDiff(uint32_t frame, const void *base_data, const void *next_data, uint32_t size);
int UpeNetDelta_Apply(const UpeNetDelta *delta, const void *base_data, uint32_t base_size,
                      void *out_data, uint32_t out_size);
uint16_t UpeNetDelta_QuantizeF32(float value, float min_value, float max_value, uint32_t bits);
float UpeNetDelta_DequantizeF32(uint16_t value, float min_value, float max_value, uint32_t bits);
void UpeNetDelta_QuantizeVec3(UpeVec3 value, UpeVec3 min_value, UpeVec3 max_value,
                              uint32_t bits, uint16_t out_q[3]);
UpeVec3 UpeNetDelta_DequantizeVec3(const uint16_t q[3], UpeVec3 min_value, UpeVec3 max_value,
                                   uint32_t bits);
void UpeNetDelta_Destroy(UpeNetDelta *delta);

/* Dead reckoning: extrapolates position forward under constant acceleration */
void UpeNetDelta_DeadReckoning(UpeVec3 pos, UpeVec3 vel, UpeVec3 accel,
                                 float dt, UpeVec3 *out_pos);

/* Ownership authority: optimistic lock-step ownership transfer */
typedef struct UpeOwnershipState {
    uint32_t owner_id;
    uint32_t authority_version;
    float    handoff_timer;
} UpeOwnershipState;

/* Returns 1 if ownership was granted (requester_id != current owner_id) */
int UpeNetDelta_RequestOwnership(UpeOwnershipState *state, uint32_t requester_id);

#ifdef __cplusplus
}
#endif
