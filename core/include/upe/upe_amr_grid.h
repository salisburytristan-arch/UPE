#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Adaptive Multi-Resolution Grids (3.0.0.0)
  
  High resolution near interaction zones, coarse far away.
  Deterministic refinement + stitching rules (no seams, no energy spikes).
  
  Used for fluids, gases, fields, terrain, etc.
*/

// Grid cell types
typedef enum UpeAmrCellType {
    UPE_AMR_CELL_EMPTY = 0,
    UPE_AMR_CELL_LEAF = 1,       // Finest level (contains data)
    UPE_AMR_CELL_INTERIOR = 2    // Has children
} UpeAmrCellType;

// Refinement criteria
typedef enum UpeAmrRefineCriteria {
    UPE_AMR_REFINE_GRADIENT = 0,  // High gradient of scalar field
    UPE_AMR_REFINE_CURVATURE = 1, // High curvature
    UPE_AMR_REFINE_DISTANCE = 2,  // Distance from point/surface
    UPE_AMR_REFINE_MANUAL = 3     // User-specified regions
} UpeAmrRefineCriteria;

// AMR cell (octree node)
typedef struct UpeAmrCell {
    UpeAmrCellType type;
    uint8_t level;               // 0 = coarsest
    
    // Spatial bounds
    UpeVec3 center;
    float half_size;
    
    // Data (if leaf)
    float *scalar_data;          // For scalar fields
    UpeVec3 *vector_data;        // For vector fields
    uint32_t data_count;
    
    // Children (if interior, exactly 8 for octree)
    struct UpeAmrCell *children[8];
    
    // Parent
    struct UpeAmrCell *parent;
    
    // Neighbors (for stitching)
    struct UpeAmrCell *neighbors[6];  // ±X, ±Y, ±Z
    
    // Refinement state
    int marked_for_refine;
    int marked_for_coarsen;
    
} UpeAmrCell;

// AMR grid system
typedef struct UpeAmrGrid {
    UpeAmrCell *root;            // Top-level cell
    
    uint8_t max_level;           // Maximum refinement depth
    float base_cell_size;        // Size at level 0
    UpeVec3 origin;
    
    // Refinement parameters
    UpeAmrRefineCriteria criteria;
    float refinement_threshold;
    float coarsening_threshold;
    
    // Leaf cells (for fast iteration)
    UpeAmrCell **leaf_cells;
    uint32_t leaf_count;
    uint32_t leaf_capacity;
    
    // Statistics
    uint32_t total_cells;
    uint32_t cells_per_level[16];
    
    // Determinism
    uint32_t refinement_seed;    // For consistent tie-breaking
    
} UpeAmrGrid;

// Initialization
void UpeAmr_Init(UpeAmrGrid *grid, UpeVec3 origin, float base_size, uint8_t max_level);
void UpeAmr_Destroy(UpeAmrGrid *grid);

// Refinement
void UpeAmr_MarkForRefinement(UpeAmrGrid *grid);  // Mark cells based on criteria
void UpeAmr_Refine(UpeAmrGrid *grid);             // Apply refinement
void UpeAmr_Coarsen(UpeAmrGrid *grid);            // Remove unnecessary cells

// Data access
UpeAmrCell* UpeAmr_FindCell(const UpeAmrGrid *grid, UpeVec3 position);
float UpeAmr_SampleScalar(const UpeAmrGrid *grid, UpeVec3 position);
UpeVec3 UpeAmr_SampleVector(const UpeAmrGrid *grid, UpeVec3 position);

void UpeAmr_WriteScalar(UpeAmrGrid *grid, UpeVec3 position, float value);
void UpeAmr_WriteVector(UpeAmrGrid *grid, UpeVec3 position, UpeVec3 value);

// Stitching (ensure continuity across level boundaries)
void UpeAmr_StitchBoundaries(UpeAmrGrid *grid);
void UpeAmr_UpdateNeighbors(UpeAmrCell *cell);

// Gradient-based refinement
float UpeAmr_ComputeGradient(const UpeAmrCell *cell, const float *field);
void UpeAmr_RefineByGradient(UpeAmrGrid *grid, const float *field, float threshold);

// Distance-based refinement
void UpeAmr_RefineNearPoint(UpeAmrGrid *grid, UpeVec3 point, float radius, uint8_t target_level);
void UpeAmr_RefineNearSurface(UpeAmrGrid *grid, const void *surface, float distance);

// Manual refinement
void UpeAmr_RefineRegion(UpeAmrGrid *grid, UpeVec3 min_bounds, UpeVec3 max_bounds, uint8_t target_level);

// Deterministic ordering
void UpeAmr_SortLeafCells(UpeAmrGrid *grid);  // Sort by Morton code
uint64_t UpeAmr_MortonCode(UpeVec3 position, float cell_size);

// Iteration (deterministic order)
void UpeAmr_ForEachLeaf(UpeAmrGrid *grid, void (*callback)(UpeAmrCell *cell, void *user_data), void *user_data);

// Statistics
void UpeAmr_UpdateStats(UpeAmrGrid *grid);
void UpeAmr_PrintStats(const UpeAmrGrid *grid);

// Export/import (for serialization)
void UpeAmr_Export(const UpeAmrGrid *grid, void *buffer, uint32_t *size);
void UpeAmr_Import(UpeAmrGrid *grid, const void *buffer, uint32_t size);

#ifdef __cplusplus
}
#endif
