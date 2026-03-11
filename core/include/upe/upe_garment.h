#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Garment (Clothing) Simulation
  
  - Pattern-based cloth construction
  - Seams and stitching
  - Self-collision with continuous collision detection
  - Wrinkle formation
*/

typedef struct UpeGarmentVertex {
    UpeVec3 position;
    UpeVec3 velocity;
    UpeVec2 uv;                  // Texture coordinates
    float mass;
    float friction;
    uint32_t flags;              // PINNED, SEAM_VERTEX, etc.
} UpeGarmentVertex;

typedef struct UpeGarmentPatch {
    UpeGarmentVertex *vertices;
    uint32_t *indices;           // Triangle indices
    uint32_t vertex_count;
    uint32_t triangle_count;
    
    // Material properties
    float stretch_stiffness;
    float shear_stiffness;
    float bend_stiffness;
    float thickness;
    
} UpeGarmentPatch;

typedef struct UpeGarmentSeam {
    uint32_t patch_a;
    uint32_t patch_b;
    uint32_t *vertex_pairs;      // Pairs of vertex indices to stitch
    uint32_t pair_count;
    float stitch_stiffness;
} UpeGarmentSeam;

typedef struct UpeGarmentSystem {
    UpeGarmentPatch *patches;
    uint32_t patch_count;
    uint32_t patch_capacity;
    
    UpeGarmentSeam *seams;
    uint32_t seam_count;
    uint32_t seam_capacity;
    
    UpeVec3 gravity;
    float air_drag;
    
    // Body collision (capsules)
    UpeVec3 *body_capsule_a;
    UpeVec3 *body_capsule_b;
    float *body_capsule_radius;
    uint32_t body_capsule_count;
    
    // Self-collision broadphase
    uint32_t *spatial_hash;
    uint32_t hash_size;
    
} UpeGarmentSystem;

void UpeGarment_Init(UpeGarmentSystem *garment, uint32_t max_patches, uint32_t max_seams);
void UpeGarment_Destroy(UpeGarmentSystem *garment);

// Patch creation
uint32_t UpeGarment_AddPatch(UpeGarmentSystem *garment, const UpeGarmentPatch *patch);
void UpeGarment_AddSeam(UpeGarmentSystem *garment, const UpeGarmentSeam *seam);

// Attach to body
void UpeGarment_PinVertex(UpeGarmentSystem *garment, uint32_t patch_id, uint32_t vertex_id, UpeVec3 position);
void UpeGarment_AddBodyCapsule(UpeGarmentSystem *garment, UpeVec3 start, UpeVec3 end, float radius);

// Simulation
void UpeGarment_Step(UpeGarmentSystem *garment, float dt);
void UpeGarment_SolveConstraints(UpeGarmentSystem *garment, uint32_t iterations);
void UpeGarment_SolveSeams(UpeGarmentSystem *garment);
void UpeGarment_SelfCollision(UpeGarmentSystem *garment);
void UpeGarment_BodyCollision(UpeGarmentSystem *garment);

// Utilities
void UpeGarment_CreateShirt(UpeGarmentSystem *garment);  // Predefined patterns
void UpeGarment_CreatePants(UpeGarmentSystem *garment);
void UpeGarment_CreateCape(UpeGarmentSystem *garment);

#ifdef __cplusplus
}
#endif
