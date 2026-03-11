#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Terrain Erosion
  
  - Hydraulic erosion (water flow)
  - Thermal erosion (weathering)
  - Sediment transport
  - Chemical weathering
*/

typedef struct UpeErosionHeightfield {
    float *heights;              // Terrain heights
    float *water;                // Water depth
    float *sediment;             // Suspended sediment
    float *hardness;             // Material resistance to erosion
    uint32_t width;
    uint32_t height;
    float cell_size;
} UpeErosionHeightfield;

typedef struct UpeErosionParameters {
    float rain_rate;             // m/s (precipitation)
    float evaporation_rate;      // m/s
    float flow_speed;            // Water flow velocity multiplier
    float erosion_rate;          // How fast water erodes terrain
    float deposition_rate;       // How fast sediment settles
    float sediment_capacity;     // Max sediment per unit water
    float thermal_erosion_angle; // Angle of repose (radians)
    float thermal_erosion_rate;
} UpeErosionParameters;

typedef struct UpeErosionSystem {
    UpeErosionHeightfield *heightfield;
    UpeErosionParameters params;
    
    // Velocity field (for water flow)
    UpeVec2 *velocity_field;
    
    // Temporary buffers
    float *height_delta;
    float *water_delta;
    
} UpeErosionSystem;

void UpeErosion_Init(UpeErosionSystem *erosion, uint32_t width, uint32_t height, float cell_size);
void UpeErosion_Destroy(UpeErosionSystem *erosion);

// Simulation
void UpeErosion_Step(UpeErosionSystem *erosion, float dt);

// Hydraulic erosion (water-driven)
void UpeErosion_HydraulicStep(UpeErosionSystem *erosion, float dt);

// Thermal erosion (gravity-driven talus formation)
void UpeErosion_ThermalStep(UpeErosionSystem *erosion, float dt);

// Sediment transport
void UpeErosion_TransportSediment(UpeErosionSystem *erosion, float dt);

// Add water source (rain, river)
void UpeErosion_AddWater(UpeErosionSystem *erosion, uint32_t x, uint32_t z, float amount);

// Query
float UpeErosion_GetHeight(const UpeErosionSystem *erosion, uint32_t x, uint32_t z);
float UpeErosion_GetWaterDepth(const UpeErosionSystem *erosion, uint32_t x, uint32_t z);
UpeVec3 UpeErosion_GetNormal(const UpeErosionSystem *erosion, uint32_t x, uint32_t z);

#ifdef __cplusplus
}
#endif
