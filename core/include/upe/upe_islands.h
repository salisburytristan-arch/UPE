#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeIsland {
    uint32_t *bodies;
    uint32_t body_count;
    int awake;
    float sleep_time;
    float kinetic_energy;
} UpeIsland;

void UpeIsland_Init(UpeIsland *island);
void UpeIsland_Destroy(UpeIsland *island);
void UpeIsland_AddBody(UpeIsland *island, uint32_t body_id);
void UpeIsland_Sleep(UpeIsland *island);
void UpeIsland_Wake(UpeIsland *island);

#ifdef __cplusplus
}
#endif
