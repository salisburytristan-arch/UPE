#pragma once

#include <stdint.h>
#include "upe/upe_shape.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeCompoundDesc {
    uint32_t shape_count;
    UpeShape *shapes;
} UpeCompoundDesc;

typedef struct UpeCompound {
    uint32_t shape_count;
    UpeShape *shapes;
} UpeCompound;

void UpeCompound_Init(UpeCompound *compound, const UpeCompoundDesc *desc);
void UpeCompound_Destroy(UpeCompound *compound);

#ifdef __cplusplus
}
#endif
