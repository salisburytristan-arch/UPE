#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Unified Field System Core (2.0.0.0)
  
  Single infrastructure for temperature, pressure, density, wind, humidity, EM, radiation, etc.
  Multiple discretizations: grid, sparse grid, mesh, particles, signed-distance volumes.
  
  Every physics domain reads/writes fields through this unified interface.
*/

// Field types
typedef enum UpeFieldType {
    UPE_FIELD_SCALAR = 0,        // Temperature, pressure, density, humidity, pH
    UPE_FIELD_VECTOR = 1,        // Wind, velocity, electric field, magnetic field
    UPE_FIELD_TENSOR = 2         // Stress/strain tensors
} UpeFieldType;

// Storage modes
typedef enum UpeFieldStorage {
    UPE_FIELD_DENSE_GRID = 0,    // Regular 3D grid (fast, memory heavy)
    UPE_FIELD_SPARSE_GRID = 1,   // Only active cells (moderate)
    UPE_FIELD_PARTICLES = 2,     // Scattered particles with kernel interpolation
    UPE_FIELD_SDF = 3,           // Signed distance field (for boundaries)
    UPE_FIELD_MESH = 4           // Per-vertex or per-element data
} UpeFieldStorage;

// Interpolation modes
typedef enum UpeFieldInterp {
    UPE_FIELD_NEAREST = 0,
    UPE_FIELD_LINEAR = 1,
    UPE_FIELD_CUBIC = 2
} UpeFieldInterp;

// Field descriptor
typedef struct UpeFieldDescriptor {
    char name[64];
    UpeFieldType type;
    UpeFieldStorage storage;
    UpeFieldInterp interp;
    
    // Domain
    UpeVec3 origin;
    UpeVec3 extent;              // Size of domain
    uint32_t resolution[3];      // Grid resolution (for grid storage)
    float cell_size;
    
    // Properties
    float diffusion_rate;        // m²/s (for diffusion-advection)
    float dissipation_rate;      // 1/s (decay rate)
    
    // Boundary conditions
    uint8_t boundary_mode;       // 0=clamp, 1=wrap, 2=mirror, 3=zero
    
    // Write permissions (for coupling contracts)
    uint32_t writer_domain_mask; // Bitfield: which domains can write
    uint32_t reader_domain_mask; // Bitfield: which domains can read
    
} UpeFieldDescriptor;

// Scalar field
typedef struct UpeFieldScalar {
    UpeFieldDescriptor desc;
    
    // Dense grid storage
    float *values;               // Size = resolution[0] * resolution[1] * resolution[2]
    
    // Sparse grid storage (if used)
    uint32_t *active_cells;      // Indices of non-empty cells
    float *sparse_values;
    uint32_t active_count;
    
    // Particle storage (if used)
    UpeVec3 *particle_positions;
    float *particle_values;
    float *particle_radii;       // Kernel radius
    uint32_t particle_count;
    
} UpeFieldScalar;

// Vector field
typedef struct UpeFieldVector {
    UpeFieldDescriptor desc;
    
    // Dense grid storage (staggered MAC grid option)
    float *values_x;
    float *values_y;
    float *values_z;
    
    // Or interleaved
    UpeVec3 *values;
    
    // Sparse/particle storage similar to scalar
    uint32_t *active_cells;
    UpeVec3 *sparse_values;
    uint32_t active_count;
    
} UpeFieldVector;

// Tensor field (3x3 matrices)
typedef struct UpeFieldTensor {
    UpeFieldDescriptor desc;
    
    UpeMatrix3 *values;          // Per grid cell or per element
    
    uint32_t *active_cells;
    UpeMatrix3 *sparse_values;
    uint32_t active_count;
    
} UpeFieldTensor;

// Field system (manages all fields)
typedef struct UpeFieldSystem {
    UpeFieldScalar *scalar_fields;
    uint32_t scalar_count;
    uint32_t scalar_capacity;
    
    UpeFieldVector *vector_fields;
    uint32_t vector_count;
    uint32_t vector_capacity;
    
    UpeFieldTensor *tensor_fields;
    uint32_t tensor_count;
    uint32_t tensor_capacity;
    
    // Spatial hashing for particle fields
    uint32_t *spatial_hash;
    uint32_t hash_size;
    
} UpeFieldSystem;

// Initialization
void UpeField_InitSystem(UpeFieldSystem *sys);
void UpeField_DestroySystem(UpeFieldSystem *sys);

// Field creation
uint32_t UpeField_CreateScalar(UpeFieldSystem *sys, const UpeFieldDescriptor *desc);
uint32_t UpeField_CreateVector(UpeFieldSystem *sys, const UpeFieldDescriptor *desc);
uint32_t UpeField_CreateTensor(UpeFieldSystem *sys, const UpeFieldDescriptor *desc);

// Sampling (read with interpolation)
float UpeField_SampleScalar(const UpeFieldScalar *field, UpeVec3 position);
UpeVec3 UpeField_SampleVector(const UpeFieldVector *field, UpeVec3 position);
UpeMatrix3 UpeField_SampleTensor(const UpeFieldTensor *field, UpeVec3 position);

// Writing (with permission checks)
int UpeField_WriteScalar(UpeFieldScalar *field, UpeVec3 position, float value, uint32_t writer_domain);
int UpeField_WriteVector(UpeFieldVector *field, UpeVec3 position, UpeVec3 value, uint32_t writer_domain);
int UpeField_WriteAdd(UpeFieldScalar *field, UpeVec3 position, float delta, uint32_t writer_domain);

// Grid access (direct, fast)
float UpeField_GetGridScalar(const UpeFieldScalar *field, uint32_t x, uint32_t y, uint32_t z);
void UpeField_SetGridScalar(UpeFieldScalar *field, uint32_t x, uint32_t y, uint32_t z, float value);

// Advection-diffusion-reaction
void UpeField_Advect(UpeFieldScalar *field, const UpeFieldVector *velocity, float dt);
void UpeField_Diffuse(UpeFieldScalar *field, float dt);
void UpeField_Dissipate(UpeFieldScalar *field, float dt);

// Gradient / divergence / curl
UpeVec3 UpeField_Gradient(const UpeFieldScalar *field, UpeVec3 position);
float UpeField_Divergence(const UpeFieldVector *field, UpeVec3 position);
UpeVec3 UpeField_Curl(const UpeFieldVector *field, UpeVec3 position);

// Debugging
void UpeField_ClearAll(UpeFieldSystem *sys);
void UpeField_PrintStats(const UpeFieldSystem *sys);

// Common field presets
uint32_t UpeField_CreateTemperature(UpeFieldSystem *sys, UpeVec3 origin, UpeVec3 extent, uint32_t res);
uint32_t UpeField_CreatePressure(UpeFieldSystem *sys, UpeVec3 origin, UpeVec3 extent, uint32_t res);
uint32_t UpeField_CreateDensity(UpeFieldSystem *sys, UpeVec3 origin, UpeVec3 extent, uint32_t res);
uint32_t UpeField_CreateWind(UpeFieldSystem *sys, UpeVec3 origin, UpeVec3 extent, uint32_t res);
uint32_t UpeField_CreateHumidity(UpeFieldSystem *sys, UpeVec3 origin, UpeVec3 extent, uint32_t res);

// ===== Wind Field Specialization =====
// A simpler grid-based wind field for cloth/particle interaction

typedef struct UpeWindField {
    UpeVec3 *velocity;            // Grid of velocity vectors
    float *turbulence;            // Per-cell turbulence factor
    float *temperature;           // Per-cell temperature (for buoyancy)
    
    uint32_t width, height, depth;
    UpeVec3 cell_size;
    uint32_t total_cells;
    
    UpeVec3 base_wind;            // Base wind direction and speed
    float wind_strength;          // Overall wind strength multiplier
    float turbulence_scale;       // Turbulence contribution factor
    float gust_frequency;         // Hz for gust oscillation
    float gust_strength;          // 0-1 gust amplitude
    float time_accumulator;       // For gust animation
} UpeWindField;

// Wind field functions
void UpeWindField_Init(UpeWindField *field, uint32_t width, uint32_t height, uint32_t depth, UpeVec3 cell_size);
void UpeWindField_Destroy(UpeWindField *field);

// Grid query
uint32_t UpeWindField_GetCellIndex(UpeWindField *field, UpeVec3 position);
UpeVec3 UpeWindField_SampleVelocity(UpeWindField *field, UpeVec3 position);

// Wind control
void UpeWindField_SetBaseWind(UpeWindField *field, UpeVec3 wind);
void UpeWindField_AddGust(UpeWindField *field, UpeVec3 wind_impulse);
void UpeWindField_UpdateGustTurbulence(UpeWindField *field, float dt);

// Apply wind to objects
void UpeWindField_AffectClothParticle(UpeWindField *field, UpeVec3 position, float mass, float drag_coeff, UpeVec3 *out_force);

// Simulation
void UpeWindField_Step(UpeWindField *field, float dt);

#ifdef __cplusplus
}
#endif
