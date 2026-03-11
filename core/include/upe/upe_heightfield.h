#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeHeightfield {
    float *heights;
    uint32_t rows;
    uint32_t cols;
    float cell_size;
} UpeHeightfield;

void UpeHeightfield_Init(UpeHeightfield *hf);
void UpeHeightfield_Destroy(UpeHeightfield *hf);
int UpeHeightfield_Allocate(UpeHeightfield *hf, uint32_t rows, uint32_t cols, float cell_size);
float UpeHeightfield_SampleHeight(const UpeHeightfield *hf, float x, float z);
UpeVec3 UpeHeightfield_GetNormal(const UpeHeightfield *hf, float x, float z);
int UpeHeightfield_RaycastClosestPoint(const UpeHeightfield *hf, UpeVec3 ray_origin, UpeVec3 ray_direction,
                                       float max_distance, float *out_t, UpeVec3 *out_point, UpeVec3 *out_normal);

#ifdef __cplusplus
}
#endif
