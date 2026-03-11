#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Octree / Loose Octree (Math/CS — Phase 10)

  A static or dynamic spatial subdivision structure.
  Supports point insertion, AABB intersection queries, and nearest-neighbor.
  Caller-owned node pool — no internal allocations after init.

  Loose octree uses 2× expanded cells to reduce insert/remove costs.
*/

#define UPE_OCTREE_MAX_DEPTH   8
#define UPE_OCTREE_MAX_ITEMS   16   /* items per leaf before subdivision */
#define UPE_OCTREE_NULL_NODE   (~(uint32_t)0)

/* ---- Item stored in the octree ---- */
typedef struct UpeOctreeItem {
    UpeVec3  center;    /* bounding sphere center */
    float    radius;    /* bounding sphere radius */
    uint32_t user_id;   /* caller's ID for this object */
} UpeOctreeItem;

/* ---- Octree node (internal or leaf) ---- */
typedef struct UpeOctreeNode {
    UpeVec3  min_corner;
    UpeVec3  max_corner;
    uint32_t children[8];   /* UPE_OCTREE_NULL_NODE if absent */
    uint32_t item_start;    /* index into item_pool */
    uint32_t item_count;
    uint32_t depth;
    int      is_leaf;
} UpeOctreeNode;

/* ---- Octree main structure ---- */
typedef struct UpeOctree {
    UpeOctreeNode *nodes;          /* node pool */
    uint32_t       node_count;
    uint32_t       node_capacity;

    UpeOctreeItem *item_pool;      /* flat item storage */
    uint32_t       item_count;
    uint32_t       item_capacity;

    uint32_t       root;           /* index of root node */
    float          loose_factor;   /* 1.0 = tight, 2.0 = loose */
} UpeOctree;

/* ---- Query result ---- */
typedef struct UpeOctreeResult {
    uint32_t *ids;          /* caller provides buffer */
    uint32_t  capacity;
    uint32_t  count;
} UpeOctreeResult;

/* --- Lifecycle --- */
int  UpeOctree_Init(UpeOctree *tree, uint32_t max_nodes, uint32_t max_items,
                    UpeVec3 world_min, UpeVec3 world_max, float loose_factor);
void UpeOctree_Destroy(UpeOctree *tree);
void UpeOctree_Clear(UpeOctree *tree);    /* remove all items, keep structure */

/* --- Insertion / removal --- */
int  UpeOctree_Insert(UpeOctree *tree, const UpeOctreeItem *item);
void UpeOctree_Remove(UpeOctree *tree, uint32_t user_id);

/* --- Queries --- */
/* AABB overlap: fills result with IDs of items whose spheres touch the box */
void UpeOctree_QueryAABB(const UpeOctree *tree, UpeVec3 box_min, UpeVec3 box_max,
                          UpeOctreeResult *result);

/* Sphere overlap */
void UpeOctree_QuerySphere(const UpeOctree *tree, UpeVec3 center, float radius,
                            UpeOctreeResult *result);

/* Nearest item to a point (returns user_id, or UPE_OCTREE_NULL_NODE if empty) */
uint32_t UpeOctree_NearestPoint(const UpeOctree *tree, UpeVec3 point, float *out_dist_sq);

/* Ray vs. octree (returns first hit user_id, or UPE_OCTREE_NULL_NODE) */
uint32_t UpeOctree_Raycast(const UpeOctree *tree,
                            UpeVec3 origin, UpeVec3 dir,
                            float max_dist, float *out_t);

/* --- Debug --- */
uint32_t UpeOctree_NodeCount(const UpeOctree *tree);
uint32_t UpeOctree_ItemCount(const UpeOctree *tree);

#ifdef __cplusplus
}
#endif
