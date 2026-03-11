#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeThermalNode {
    float temperature;
    float heat_capacity;
    float thermal_mass;
} UpeThermalNode;

typedef struct UpeThermalGrid {
    UpeThermalNode *nodes;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    float conductivity;
} UpeThermalGrid;

void UpeThermalGrid_Init(UpeThermalGrid *grid, uint32_t w, uint32_t h, uint32_t d);
void UpeThermalGrid_Step(UpeThermalGrid *grid, float dt);
void UpeThermalGrid_Destroy(UpeThermalGrid *grid);

/* Convective heat flux  q = h_conv * (T_surface - T_fluid)  [W/m^2] */
float UpeThermal_ConvectiveHeatFlux(float h_conv, float T_surface, float T_fluid);

/* Linear thermal expansion  delta_L = alpha * L0 * delta_T */
float UpeThermal_ThermalExpansion(float length, float alpha_cte, float delta_T);

#ifdef __cplusplus
}
#endif
