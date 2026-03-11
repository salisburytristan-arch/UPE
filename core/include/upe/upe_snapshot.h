#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeSnapshot {
    void *data;
    uint32_t size;
    uint32_t frame;
} UpeSnapshot;

UpeSnapshot *UpeSnapshot_Create(const void *world_state, uint32_t size, uint32_t frame);
void UpeSnapshot_Destroy(UpeSnapshot *snapshot);
int UpeSnapshot_Restore(UpeSnapshot *snapshot, void *world_state, uint32_t max_size);

#ifdef __cplusplus
}
#endif
