#include "upe/upe_radiation.h"
#include <stdlib.h>
#include <math.h>

void UpeRadiationGrid_Init(UpeRadiationGrid *grid, uint32_t node_count) {
    if (!grid) {
        return;
    }
    grid->count = node_count;
    grid->stefan_boltzmann = 5.67e-8f;
    grid->nodes = (UpeRadiationNode *)malloc(node_count * sizeof(UpeRadiationNode));
    for (uint32_t i = 0; i < node_count; ++i) {
        grid->nodes[i].temperature = 300.0f;
        grid->nodes[i].emissivity = 0.9f;
        grid->nodes[i].absorbed_energy = 0.0f;
    }
}

void UpeRadiationGrid_Step(UpeRadiationGrid *grid, float dt) {
    if (!grid) {
        return;
    }
    
    for (uint32_t i = 0; i < grid->count; ++i) {
        float T4 = grid->nodes[i].temperature * grid->nodes[i].temperature * 
                   grid->nodes[i].temperature * grid->nodes[i].temperature;
        float radiated_power = grid->stefan_boltzmann * grid->nodes[i].emissivity * T4;
        grid->nodes[i].absorbed_energy -= radiated_power * dt;
    }
    
    for (uint32_t i = 0; i < grid->count; ++i) {
        grid->nodes[i].temperature += grid->nodes[i].absorbed_energy * dt;
        grid->nodes[i].absorbed_energy = 0.0f;
    }
}

void UpeRadiationGrid_Destroy(UpeRadiationGrid *grid) {
    if (!grid) {
        return;
    }
    free(grid->nodes);
}
