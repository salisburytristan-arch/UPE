#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Advanced Optics & Light Transport
  
  - Caustics (focused light patterns)
  - Volumetric rendering (god rays, fog)
  - Refraction and reflection
  - Participating media (scattering)
*/

typedef struct UpePhoton {
    UpeVec3 position;
    UpeVec3 direction;
    float wavelength;            // nm (for dispersion)
    float energy;                // Relative intensity
    uint32_t bounce_count;
} UpePhoton;

typedef struct UpeOpticalMaterial {
    float refractive_index;      // IOR (1.0 = vacuum, 1.33 = water, 1.5 = glass)
    float absorption[3];         // RGB absorption per meter
    float scattering;            // Scattering coefficient
    float roughness;             // Surface roughness (for blurred refraction)
} UpeOpticalMaterial;

typedef struct UpeCausticMap {
    float *intensity;            // Photon density (for caustics)
    uint32_t width;
    uint32_t height;
    UpeVec3 plane_origin;
    UpeVec3 plane_normal;
    float texel_size;
} UpeCausticMap;

typedef struct UpeVolumetricGrid {
    float *density;              // Fog/smoke/dust density
    float *temperature;          // For thermal effects (optional)
    UpeVec3 *velocity;           // For wind/flow (optional)
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    float cell_size;
    UpeVec3 origin;
} UpeVolumetricGrid;

typedef struct UpeOpticsSystem {
    UpePhoton *photons;
    uint32_t photon_count;
    uint32_t photon_capacity;
    
    UpeCausticMap *caustic_maps;
    uint32_t caustic_map_count;
    
    UpeVolumetricGrid *volume;
    
    // Light sources
    UpeVec3 *light_positions;
    float *light_intensities;
    uint32_t light_count;
    
} UpeOpticsSystem;

void UpeOptics_Init(UpeOpticsSystem *optics, uint32_t max_photons);
void UpeOptics_Destroy(UpeOpticsSystem *optics);

// Photon tracing
void UpeOptics_EmitPhotons(UpeOpticsSystem *optics, UpeVec3 source_pos, uint32_t count);
void UpeOptics_TracePhoton(UpePhoton *photon, float distance);
void UpeOptics_RefractPhoton(UpePhoton *photon, UpeVec3 normal, float ior_from, float ior_to);
void UpeOptics_ReflectPhoton(UpePhoton *photon, UpeVec3 normal);

// Caustics
void UpeOptics_BuildCausticMap(UpeOpticsSystem *optics, const UpeCausticMap *map);
float UpeOptics_SampleCaustics(const UpeCausticMap *map, UpeVec3 world_pos);

// Volumetric effects
void UpeOptics_SetVolumetricGrid(UpeOpticsSystem *optics, const UpeVolumetricGrid *volume);
float UpeOptics_ComputeTransmittance(const UpeVolumetricGrid *volume, UpeVec3 start, UpeVec3 end);
UpeVec3 UpeOptics_ComputeInScattering(const UpeVolumetricGrid *volume, UpeVec3 view_pos, UpeVec3 view_dir, float distance);

// Refraction helpers
float UpeOptics_FresnelSchlick(float cos_theta, float ior_from, float ior_to);
UpeVec3 UpeOptics_DispersionRefract(UpeVec3 incident, UpeVec3 normal, float ior, float wavelength);

#ifdef __cplusplus
}
#endif
