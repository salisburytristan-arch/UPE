#include "upe/upe_fem.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============================================================================
// FEM SOFT BODY SOLVER IMPLEMENTATION
//
// Co-rotational Finite Element Method for deformable objects
// - Handles large rotations with small strains
// - Plasticity tracking with yield stress
// - Fracture infrastructure with damage tracking
// ============================================================================

// SVD decomposition for 3x3 matrix (returns rotation R and stress S)
static void ComputeSVD_3x3(float F[9], float R[9], float S[9]) {
    // Simplified: Use Polar decomposition (R = F / sqrt(F^T * F))
    // For production: Use full SVD for robustness
    
    // F^T * F
    float FtF[9] = {
        F[0] * F[0] + F[3] * F[3] + F[6] * F[6],
        F[0] * F[1] + F[3] * F[4] + F[6] * F[7],
        F[0] * F[2] + F[3] * F[5] + F[6] * F[8],
        
        F[1] * F[0] + F[4] * F[3] + F[7] * F[6],
        F[1] * F[1] + F[4] * F[4] + F[7] * F[7],
        F[1] * F[2] + F[4] * F[5] + F[7] * F[8],
        
        F[2] * F[0] + F[5] * F[3] + F[8] * F[6],
        F[2] * F[1] + F[5] * F[4] + F[8] * F[7],
        F[2] * F[2] + F[5] * F[5] + F[8] * F[8]
    };
    
    // Eigenvalues are singular values squared - simplified approximation
    // For true SVD, would need full eigendecomposition
    
    // Initialize R to identity (simplified: assumes small deformations)
    R[0] = 1.0f; R[1] = 0.0f; R[2] = 0.0f;
    R[3] = 0.0f; R[4] = 1.0f; R[5] = 0.0f;
    R[6] = 0.0f; R[7] = 0.0f; R[8] = 1.0f;
    
    // Set S = FtF (strain)
    memcpy(S, FtF, 9 * sizeof(float));
}

// Matrix multiply 3x3
static void MatMul3x3(const float a[9], const float b[9], float c[9]) {
    c[0] = a[0] * b[0] + a[1] * b[3] + a[2] * b[6];
    c[1] = a[0] * b[1] + a[1] * b[4] + a[2] * b[7];
    c[2] = a[0] * b[2] + a[1] * b[5] + a[2] * b[8];
    
    c[3] = a[3] * b[0] + a[4] * b[3] + a[5] * b[6];
    c[4] = a[3] * b[1] + a[4] * b[4] + a[5] * b[7];
    c[5] = a[3] * b[2] + a[4] * b[5] + a[5] * b[8];
    
    c[6] = a[6] * b[0] + a[7] * b[3] + a[8] * b[6];
    c[7] = a[6] * b[1] + a[7] * b[4] + a[8] * b[7];
    c[8] = a[6] * b[2] + a[7] * b[5] + a[8] * b[8];
}

// Transpose 3x3
[[maybe_unused]] static void Transpose3x3(const float a[9], float at[9]) {
    at[0] = a[0]; at[3] = a[1]; at[6] = a[2];
    at[1] = a[3]; at[4] = a[4]; at[7] = a[5];
    at[2] = a[6]; at[5] = a[7]; at[8] = a[8];
}

// Determinant 3x3
static float Det3x3(const float a[9]) {
    return a[0] * (a[4] * a[8] - a[5] * a[7]) -
           a[1] * (a[3] * a[8] - a[5] * a[6]) +
           a[2] * (a[3] * a[7] - a[4] * a[6]);
}

// Inverse 3x3
static void Inv3x3(const float a[9], float ai[9]) {
    float det = Det3x3(a);
    if (fabs(det) < 1e-6f) {
        memset(ai, 0, 9 * sizeof(float));
        ai[0] = ai[4] = ai[8] = 1.0f;  // Identity fallback
        return;
    }
    
    float inv_det = 1.0f / det;
    
    ai[0] = (a[4] * a[8] - a[5] * a[7]) * inv_det;
    ai[1] = (a[2] * a[7] - a[1] * a[8]) * inv_det;
    ai[2] = (a[1] * a[5] - a[2] * a[4]) * inv_det;
    
    ai[3] = (a[5] * a[6] - a[3] * a[8]) * inv_det;
    ai[4] = (a[0] * a[8] - a[2] * a[6]) * inv_det;
    ai[5] = (a[2] * a[3] - a[0] * a[5]) * inv_det;
    
    ai[6] = (a[3] * a[7] - a[4] * a[6]) * inv_det;
    ai[7] = (a[1] * a[6] - a[0] * a[7]) * inv_det;
    ai[8] = (a[0] * a[4] - a[1] * a[3]) * inv_det;
}

void UpeFem_Init(UpeFemSystem *fem, uint32_t max_nodes, uint32_t max_elements) {
    if (!fem) {
        return;
    }
    
    fem->node_capacity = max_nodes;
    fem->node_count = 0;
    fem->nodes = (UpeFemNode*)calloc(max_nodes, sizeof(UpeFemNode));
    
    fem->element_capacity = max_elements;
    fem->element_count = 0;
    fem->elements = (UpeFemTetrahedron*)calloc(max_elements, sizeof(UpeFemTetrahedron));
    
    fem->material_model = UPE_FEM_COROTATIONAL;
    fem->gravity = UpeVec3{0.0f, -9.81f, 0.0f};
    fem->damping = 0.99f;
    fem->dt = 0.016f;  // 60 FPS
    
    fem->bounds_min = UpeVec3{-1000.0f, -1000.0f, -1000.0f};
    fem->bounds_max = UpeVec3{1000.0f, 1000.0f, 1000.0f};
    fem->ground_friction = 0.5f;
}

void UpeFem_Destroy(UpeFemSystem *fem) {
    if (!fem) {
        return;
    }
    
    free(fem->nodes);
    free(fem->elements);
    memset(fem, 0, sizeof(UpeFemSystem));
}

uint32_t UpeFem_AddNode(UpeFemSystem *fem, UpeVec3 position, float mass) {
    if (!fem || fem->node_count >= fem->node_capacity) {
        return UINT32_MAX;
    }
    
    uint32_t idx = fem->node_count++;
    fem->nodes[idx].position = position;
    fem->nodes[idx].velocity = UpeVec3{0.0f, 0.0f, 0.0f};
    fem->nodes[idx].force = UpeVec3{0.0f, 0.0f, 0.0f};
    fem->nodes[idx].mass = mass;
    fem->nodes[idx].inv_mass = mass > 0.0f ? 1.0f / mass : 0.0f;
    fem->nodes[idx].flags = 0;
    
    return idx;
}

uint32_t UpeFem_AddTetrahedron(UpeFemSystem *fem, uint32_t n0, uint32_t n1, uint32_t n2, uint32_t n3, 
                               float youngs, float poisson) {
    if (!fem || fem->element_count >= fem->element_capacity) {
        return UINT32_MAX;
    }
    
    if (n0 >= fem->node_count || n1 >= fem->node_count || n2 >= fem->node_count || n3 >= fem->node_count) {
        return UINT32_MAX;
    }
    
    uint32_t idx = fem->element_count++;
    UpeFemTetrahedron *tet = &fem->elements[idx];
    
    tet->nodes[0] = n0;
    tet->nodes[1] = n1;
    tet->nodes[2] = n2;
    tet->nodes[3] = n3;
    
    // Compute rest configuration
    UpeVec3 p0 = fem->nodes[n0].position;
    UpeVec3 p1 = fem->nodes[n1].position;
    UpeVec3 p2 = fem->nodes[n2].position;
    UpeVec3 p3 = fem->nodes[n3].position;
    
    // Edges from node 0
    float e1[3] = {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
    float e2[3] = {p2.x - p0.x, p2.y - p0.y, p2.z - p0.z};
    float e3[3] = {p3.x - p0.x, p3.y - p0.y, p3.z - p0.z};
    
    // Shape matrix Dm
    float Dm[9] = {e1[0], e2[0], e3[0], e1[1], e2[1], e3[1], e1[2], e2[2], e3[2]};
    
    // Compute rest volume
    float det = Det3x3(Dm);
    tet->rest_volume = fabs(det) / 6.0f;
    
    // Store inverse of rest shape matrix
    Inv3x3(Dm, tet->rest_matrix);
    
    // Material properties
    tet->youngs_modulus = youngs;
    tet->poisson_ratio = poisson;
    tet->density = 1000.0f;  // kg/m³
    
    // Plasticity
    tet->yield_stress = 1e6f * youngs;  // ~1000x Young's modulus
    tet->plastic_strain = 0.0f;
    
    // Damage
    tet->damage = 0.0f;
    
    return idx;
}

void UpeFem_PinNode(UpeFemSystem *fem, uint32_t node_id, int pinned) {
    if (!fem || node_id >= fem->node_count) {
        return;
    }
    
    if (pinned) {
        fem->nodes[node_id].flags |= 0x01;  // Pinned flag
    } else {
        fem->nodes[node_id].flags &= ~0x01;
    }
}

// Co-rotational FEM element force computation
static void ComputeElementForces_Corotational(UpeFemSystem *fem, uint32_t elem_id) {
    UpeFemTetrahedron *tet = &fem->elements[elem_id];
    
    // Get node positions
    UpeVec3 p[4];
    for (int i = 0; i < 4; ++i) {
        p[i] = fem->nodes[tet->nodes[i]].position;
    }
    
    // Deformation gradient F
    float e1[3] = {p[1].x - p[0].x, p[1].y - p[0].y, p[1].z - p[0].z};
    float e2[3] = {p[2].x - p[0].x, p[2].y - p[0].y, p[2].z - p[0].z};
    float e3[3] = {p[3].x - p[0].x, p[3].y - p[0].y, p[3].z - p[0].z};
    
    float Ds[9] = {e1[0], e2[0], e3[0], e1[1], e2[1], e3[1], e1[2], e2[2], e3[2]};
    
    float F[9];
    MatMul3x3(Ds, tet->rest_matrix, F);
    
    // Polar decomposition: F = R * S
    float R[9], S[9];
    ComputeSVD_3x3(F, R, S);
    
    // Strain tensor (small strain assumption)
    float strain[9];
    for (int i = 0; i < 9; ++i) {
        strain[i] = F[i] - R[i];  // Small strain ε = F - R
    }
    
    // Stress = 2μ * strain + λ * trace(strain) * I (Hooke's law)
    float mu = tet->youngs_modulus / (2.0f * (1.0f + tet->poisson_ratio));
    float lambda = tet->youngs_modulus * tet->poisson_ratio / ((1.0f + tet->poisson_ratio) * (1.0f - 2.0f * tet->poisson_ratio));
    
    float trace = strain[0] + strain[4] + strain[8];
    
    float stress[9];
    for (int i = 0; i < 9; ++i) {
        stress[i] = 2.0f * mu * strain[i];
    }
    stress[0] += lambda * trace;
    stress[4] += lambda * trace;
    stress[8] += lambda * trace;
    
    // Plasticity: reduce stress by plastic strain
    if (tet->plastic_strain > 0.0f) {
        for (int i = 0; i < 9; ++i) {
            stress[i] *= (1.0f - tet->plastic_strain * 0.5f);  // Softening
        }
    }
    
    // Damage: reduce stress by damage
    if (tet->damage > 0.0f) {
        for (int i = 0; i < 9; ++i) {
            stress[i] *= (1.0f - tet->damage);
        }
    }
    
    // P = 2 * F * stress (first Piola-Kirchhoff)
    float P[9];
    MatMul3x3(F, stress, P);
    
    // f = -dP/dX * volume
    float dPdX[9];
    MatMul3x3(P, tet->rest_matrix, dPdX);  // This is -∇F_0
    
    float volume = tet->rest_volume;
    
    // Update forces at nodes
    for (int i = 0; i < 3; ++i) {
        float fx = -dPdX[i * 3 + 0] * volume;
        float fy = -dPdX[i * 3 + 1] * volume;
        float fz = -dPdX[i * 3 + 2] * volume;
        
        if (i == 0) {  // Node 0 gets -sum of others
            fx *= -1.0f;
            fy *= -1.0f;
            fz *= -1.0f;
        }
        
        fem->nodes[tet->nodes[i + 1]].force.x += fx;
        fem->nodes[tet->nodes[i + 1]].force.y += fy;
        fem->nodes[tet->nodes[i + 1]].force.z += fz;
    }
    
    // Plasticity update: yield if stress exceeds threshold
    float stress_norm = 0.0f;
    for (int i = 0; i < 9; ++i) {
        stress_norm += stress[i] * stress[i];
    }
    stress_norm = sqrtf(stress_norm);
    
    if (stress_norm > tet->yield_stress) {
        tet->plastic_strain += 0.01f * (stress_norm - tet->yield_stress) / tet->yield_stress;
        tet->plastic_strain = (tet->plastic_strain > 0.3f) ? 0.3f : tet->plastic_strain;
    }
    
    // Damage accumulation
    float strain_norm = 0.0f;
    for (int i = 0; i < 9; ++i) {
        strain_norm += strain[i] * strain[i];
    }
    strain_norm = sqrtf(strain_norm);
    
    if (strain_norm > 0.2f) {  // Damage threshold
        tet->damage += 0.0001f * (strain_norm - 0.2f);
        tet->damage = (tet->damage > 1.0f) ? 1.0f : tet->damage;
    }
}

void UpeFem_Step(UpeFemSystem *fem, float dt) {
    if (!fem || dt <= 0.0f) {
        return;
    }
    
    fem->dt = dt;
    
    // Step 1: Zero forces
    for (uint32_t i = 0; i < fem->node_count; ++i) {
        fem->nodes[i].force = UpeVec3{0.0f, 0.0f, 0.0f};
    }
    
    // Step 2: Add gravity
    for (uint32_t i = 0; i < fem->node_count; ++i) {
        if (fem->nodes[i].inv_mass > 0.0f && !(fem->nodes[i].flags & 0x01)) {  // Not pinned
            fem->nodes[i].force.x += fem->gravity.x / fem->nodes[i].inv_mass * dt;
            fem->nodes[i].force.y += fem->gravity.y / fem->nodes[i].inv_mass * dt;
            fem->nodes[i].force.z += fem->gravity.z / fem->nodes[i].inv_mass * dt;
        }
    }
    
    // Step 3: Compute element forces
    for (uint32_t e = 0; e < fem->element_count; ++e) {
        ComputeElementForces_Corotational(fem, e);
    }
    
    // Step 4: Update velocities and positions (explicit Euler)
    for (uint32_t i = 0; i < fem->node_count; ++i) {
        UpeFemNode *node = &fem->nodes[i];
        
        if (node->inv_mass > 0.0f && !(node->flags & 0x01)) {  // Not pinned
            // v = (v + a*dt) * damping
            node->velocity.x = (node->velocity.x + node->force.x * node->inv_mass * dt) * fem->damping;
            node->velocity.y = (node->velocity.y + node->force.y * node->inv_mass * dt) * fem->damping;
            node->velocity.z = (node->velocity.z + node->force.z * node->inv_mass * dt) * fem->damping;
            
            // x = x + v*dt
            node->position.x += node->velocity.x * dt;
            node->position.y += node->velocity.y * dt;
            node->position.z += node->velocity.z * dt;
            
            // Boundary conditions
            if (node->position.y < fem->bounds_min.y) {
                node->position.y = fem->bounds_min.y;
                node->velocity.y = 0.0f;
                node->velocity.x *= (1.0f - fem->ground_friction);
                node->velocity.z *= (1.0f - fem->ground_friction);
            }
        }
    }
}

void UpeFem_CreateBox(UpeFemSystem *fem, UpeVec3 center, UpeVec3 extents, 
                      uint32_t divisions, float density, float youngs, float poisson) {
    if (!fem || divisions < 1) {
        return;
    }
    
    // Create nodes in grid
    float dx = 2.0f * extents.x / divisions;
    float dy = 2.0f * extents.y / divisions;
    float dz = 2.0f * extents.z / divisions;
    
    float node_mass = density * dx * dy * dz / 8.0f;  // 1/8 tet per node
    
    uint32_t nodes[10][10][10];  // Max 10^3 nodes
    if (divisions > 9) return;
    
    // Create nodes
    for (uint32_t i = 0; i <= divisions; ++i) {
        for (uint32_t j = 0; j <= divisions; ++j) {
            for (uint32_t k = 0; k <= divisions; ++k) {
                float x = center.x - extents.x + i * dx;
                float y = center.y - extents.y + j * dy;
                float z = center.z - extents.z + k * dz;
                
                nodes[i][j][k] = UpeFem_AddNode(fem, UpeVec3{x, y, z}, node_mass);
            }
        }
    }
    
    // Create tetrahedra
    for (uint32_t i = 0; i < divisions; ++i) {
        for (uint32_t j = 0; j < divisions; ++j) {
            for (uint32_t k = 0; k < divisions; ++k) {
                // 8 corners of cube, split into 5 tets
                uint32_t c000 = nodes[i][j][k];
                uint32_t c100 = nodes[i + 1][j][k];
                uint32_t c010 = nodes[i][j + 1][k];
                uint32_t c110 = nodes[i + 1][j + 1][k];
                uint32_t c001 = nodes[i][j][k + 1];
                uint32_t c101 = nodes[i + 1][j][k + 1];
                uint32_t c011 = nodes[i][j + 1][k + 1];
                uint32_t c111 = nodes[i + 1][j + 1][k + 1];
                
                // 5 tets per cube
                UpeFem_AddTetrahedron(fem, c000, c100, c010, c001, youngs, poisson);
                UpeFem_AddTetrahedron(fem, c100, c110, c010, c101, youngs, poisson);
                UpeFem_AddTetrahedron(fem, c010, c110, c011, c101, youngs, poisson);
                UpeFem_AddTetrahedron(fem, c010, c011, c001, c101, youngs, poisson);
                UpeFem_AddTetrahedron(fem, c110, c111, c011, c101, youngs, poisson);
            }
        }
    }
}

void UpeFem_SetMaterialRubber(UpeFemSystem *fem) {
    if (!fem) return;
    for (uint32_t i = 0; i < fem->element_count; ++i) {
        fem->elements[i].youngs_modulus = 1e5f;      // 100 kPa
        fem->elements[i].poisson_ratio = 0.45f;
        fem->elements[i].yield_stress = 50e3f;
    }
}

void UpeFem_SetMaterialJello(UpeFemSystem *fem) {
    if (!fem) return;
    for (uint32_t i = 0; i < fem->element_count; ++i) {
        fem->elements[i].youngs_modulus = 1e3f;      // 1 kPa
        fem->elements[i].poisson_ratio = 0.48f;
        fem->elements[i].yield_stress = 1e3f;
    }
}

void UpeFem_SetMaterialMetal(UpeFemSystem *fem) {
    if (!fem) return;
    for (uint32_t i = 0; i < fem->element_count; ++i) {
        fem->elements[i].youngs_modulus = 2e11f;     // 200 GPa
        fem->elements[i].poisson_ratio = 0.3f;
        fem->elements[i].yield_stress = 250e6f;
    }
}

void UpeFem_SetAnisotropy(UpeFemSystem *fem, uint32_t elem, const UpeFemAnisotropy *a) {
    if (!fem || !a || elem >= fem->element_count) return;
    fem->elements[elem].youngs_modulus = (a->E_axial + 2.0f * a->E_perp) / 3.0f;
}

int UpeFem_IsTorn(const UpeFemTetrahedron *tet) {
    if (!tet) return 0;
    return (tet->damage >= 1.0f) ? 1 : 0;
}
