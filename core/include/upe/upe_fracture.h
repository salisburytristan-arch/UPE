#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Rigid Body Destruction & Fracture
  
  - Voronoi fracture patterns
  - Stress-based fracture triggers
  - Progressive damage accumulation
  - Fragment hierarchies (chunk → shard → dust)
*/

#define UPE_FRACTURE_MAX_FRAGMENTS 512
#define UPE_FRACTURE_MAX_VORONOI_SITES 128

typedef enum UpeFractureMode {
    UPE_FRACTURE_VORONOI,        // Voronoi tessellation (most common)
    UPE_FRACTURE_RADIAL,         // Radial cracks from impact point
    UPE_FRACTURE_PLANAR,         // Planar cuts (slicing)
    UPE_FRACTURE_SHATTER          // Complete disintegration
} UpeFractureMode;

typedef struct UpeFracturePattern {
    UpeFractureMode mode;
    uint32_t num_fragments;      // Target fragment count
    UpeVec3 *voronoi_sites;      // Fracture cell centers
    uint32_t site_count;
    UpeVec3 impact_point;        // For radial fracture
    UpeVec3 impact_direction;
    float impact_force;
    uint32_t random_seed;
} UpeFracturePattern;

typedef struct UpeFractureFragment {
    uint32_t body_id;            // Resulting rigid body
    UpeVec3 centroid;
    float volume;
    float mass;
    UpeVec3 *vertices;           // Fragment mesh vertices
    uint32_t vertex_count;
    uint32_t *indices;           // Triangle indices
    uint32_t index_count;
    int is_debris;               // Small fragments can be debris
} UpeFractureFragment;

typedef struct UpeDamageModel {
    float max_stress;            // Yield strength (Pa)
    float fracture_toughness;    // Resistance to crack propagation
    float accumulated_damage;    // 0 = intact, 1 = fractured
    float damage_rate;           // How fast damage accumulates
    float healing_rate;          // Some materials can self-heal
    
    // Brittle vs ductile
    float brittleness;           // 0 = ductile (bends), 1 = brittle (shatters)
    float plastic_strain;        // Permanent deformation before fracture
} UpeDamageModel;

// Fracture execution
typedef struct UpeFractureSystem {
    UpeFractureFragment *fragments;
    uint32_t fragment_count;
    uint32_t fragment_capacity;
    
    // Fracture queue (bodies awaiting fracture)
    uint32_t *fracture_queue;
    uint32_t queue_count;
    uint32_t queue_capacity;
} UpeFractureSystem;

void UpeFracture_Init(UpeFractureSystem *sys, uint32_t max_fragments);
void UpeFracture_Destroy(UpeFractureSystem *sys);

// Fracture a body
void UpeFracture_Execute(UpeFractureSystem *sys, 
                        uint32_t body_id,
                        const UpeFracturePattern *pattern,
                        UpeFractureFragment *out_fragments,
                        uint32_t *out_count);

// Generate fracture patterns
void UpeFracture_GenerateVoronoi(UpeFracturePattern *pattern, 
                                UpeVec3 bounds_min, UpeVec3 bounds_max,
                                uint32_t num_cells, uint32_t seed);

void UpeFracture_GenerateRadial(UpeFracturePattern *pattern,
                               UpeVec3 impact_point, UpeVec3 impact_dir,
                               uint32_t num_rays);

// Damage accumulation
void UpeDamage_ApplyImpact(UpeDamageModel *damage, float impact_force, float threshold);
int UpeDamage_ShouldFracture(const UpeDamageModel *damage);

// Stress calculation (from collisions, forces)
float UpeFracture_ComputeStress(UpeVec3 force, float area, UpeVec3 normal);

#ifdef __cplusplus
}
#endif
