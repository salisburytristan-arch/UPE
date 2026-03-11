#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeBreakable {
    uint32_t body_id;
    float fracture_threshold;
    float stress;
    int fractured;
} UpeBreakable;

void UpeBreakable_Init(UpeBreakable *breakable, uint32_t body_id, float threshold);
void UpeBreakable_ApplyStress(UpeBreakable *breakable, float stress);
int UpeBreakable_CheckFracture(UpeBreakable *breakable);

#ifdef __cplusplus
}
#endif
