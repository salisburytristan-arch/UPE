#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeWaveGrid {
    float *wave_height;
    float *velocity;
    uint32_t width;
    uint32_t height;
    float wave_speed;
    float damping;
} UpeWaveGrid;

void UpeWaveGrid_Init(UpeWaveGrid *grid, uint32_t w, uint32_t h);
void UpeWaveGrid_Step(UpeWaveGrid *grid, float dt);
void UpeWaveGrid_Disturb(UpeWaveGrid *grid, uint32_t x, uint32_t y, float magnitude);
void UpeWaveGrid_Destroy(UpeWaveGrid *grid);

#ifdef __cplusplus
}
#endif
