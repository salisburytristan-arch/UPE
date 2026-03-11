#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Finite Element Method (FEM) Soft Bodies
  
  Tetrahedral mesh-based deformable objects with:
  - Linear/corotational FEM
  - Hyperelastic materials
  - Plasticity and fracture
*/

#define UPE_FEM_MAX_NODES 16384
#define UPE_FEM_MAX_ELEMENTS 65536

typedef struct UpeFemNode {
    UpeVec3 position;            // Current position
    UpeVec3 velocity;
    UpeVec3 force;               // Accumulated force
    float mass;
    float inv_mass;
    uint32_t flags;              // Pinned, etc.
} UpeFemNode;

typedef struct UpeFemTetrahedron {
    uint32_t nodes[4];           // Indices into node array
    
    // Rest state
    float rest_volume;
    float rest_matrix[9];        // Inverse of rest-state shape matrix
    
    // Material properties (per-element)
    float youngs_modulus;        // E (stiffness)
    float poisson_ratio;         // ν (lateral strain)
    float density;
    
    // Plasticity
    float yield_stress;
    float plastic_strain;
    
    // Damage/fracture
    float damage;                // 0 = intact, 1 = fully fractured
} UpeFemTetrahedron;

typedef enum UpeFemMaterialModel {
    UPE_FEM_LINEAR,              // Linear elastic (fast, small deformations)
    UPE_FEM_COROTATIONAL,        // Corotational (handles rotation, medium deformations)
    UPE_FEM_NEOHOOKEAN,          // Hyperelastic (large deformations)
    UPE_FEM_STVK                 // St. Venant-Kirchhoff
} UpeFemMaterialModel;

typedef struct UpeFemSystem {
    UpeFemNode *nodes;
    uint32_t node_count;
    uint32_t node_capacity;
    
    UpeFemTetrahedron *elements;
    uint32_t element_count;
    uint32_t element_capacity;
    
    UpeFemMaterialModel material_model;
    
    // Simulation
    UpeVec3 gravity;
    float damping;
    float dt;
    
    // Boundaries
    UpeVec3 bounds_min;
    UpeVec3 bounds_max;
    float ground_friction;
} UpeFemSystem;

// System management
void UpeFem_Init(UpeFemSystem *fem, uint32_t max_nodes, uint32_t max_elements);
void UpeFem_Destroy(UpeFemSystem *fem);

// Mesh construction
uint32_t UpeFem_AddNode(UpeFemSystem *fem, UpeVec3 position, float mass);
uint32_t UpeFem_AddTetrahedron(UpeFemSystem *fem, uint32_t n0, uint32_t n1, uint32_t n2, uint32_t n3, float youngs, float poisson);

// Simulation
void UpeFem_Step(UpeFemSystem *fem, float dt);

// Utilities
void UpeFem_CreateBox(UpeFemSystem *fem, UpeVec3 center, UpeVec3 extents, 
                     uint32_t divisions, float density, float youngs, float poisson);
void UpeFem_PinNode(UpeFemSystem *fem, uint32_t node_id, int pinned);

// Material presets
void UpeFem_SetMaterialRubber(UpeFemSystem *fem);
void UpeFem_SetMaterialJello(UpeFemSystem *fem);
void UpeFem_SetMaterialMetal(UpeFemSystem *fem);

/* Anisotropic material (fiber-reinforced) */
typedef struct UpeFemAnisotropy {
    UpeVec3 fiber_direction;  /* Unit vector along fiber axis */
    float   E_axial;          /* Young's modulus along fiber */
    float   E_perp;           /* Young's modulus transverse */
} UpeFemAnisotropy;
void UpeFem_SetAnisotropy(UpeFemSystem *fem, uint32_t elem, const UpeFemAnisotropy *a);

/* Ductile tearing check: returns 1 if damage >= 1.0 (element has torn) */
int  UpeFem_IsTorn(const UpeFemTetrahedron *tet);

#ifdef __cplusplus
}
#endif
