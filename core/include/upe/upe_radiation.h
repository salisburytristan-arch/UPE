#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeRadiationNode {
    float temperature;
    float emissivity;
    float absorbed_energy;
} UpeRadiationNode;

typedef struct UpeRadiationGrid {
    UpeRadiationNode *nodes;
    uint32_t count;
    float stefan_boltzmann;
} UpeRadiationGrid;

void UpeRadiationGrid_Init(UpeRadiationGrid *grid, uint32_t node_count);
void UpeRadiationGrid_Step(UpeRadiationGrid *grid, float dt);
void UpeRadiationGrid_Destroy(UpeRadiationGrid *grid);

#ifdef __cplusplus
}
#endif
