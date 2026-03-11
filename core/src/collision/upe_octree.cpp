#include "upe/upe_octree.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int aabb_overlaps_sphere(UpeVec3 bmin, UpeVec3 bmax, UpeVec3 c, float r) {
    float dx = c.x < bmin.x ? bmin.x - c.x : (c.x > bmax.x ? c.x - bmax.x : 0.0f);
    float dy = c.y < bmin.y ? bmin.y - c.y : (c.y > bmax.y ? c.y - bmax.y : 0.0f);
    float dz = c.z < bmin.z ? bmin.z - c.z : (c.z > bmax.z ? c.z - bmax.z : 0.0f);
    return (dx*dx + dy*dy + dz*dz) <= r*r;
}

static int aabb_overlaps_aabb(UpeVec3 a0, UpeVec3 a1, UpeVec3 b0, UpeVec3 b1) {
    return (a0.x <= b1.x && a1.x >= b0.x &&
            a0.y <= b1.y && a1.y >= b0.y &&
            a0.z <= b1.z && a1.z >= b0.z);
}

/* -----------------------------------------------------------------------
   Lifecycle
   ----------------------------------------------------------------------- */

int UpeOctree_Init(UpeOctree *tree, uint32_t max_nodes, uint32_t max_items,
                   UpeVec3 world_min, UpeVec3 world_max, float loose_factor) {
    if (!tree || max_nodes == 0 || max_items == 0) return -1;
    memset(tree, 0, sizeof(*tree));
    tree->loose_factor  = (loose_factor < 1.0f) ? 1.0f : loose_factor;
    tree->node_capacity = max_nodes;
    tree->item_capacity = max_items;

    tree->nodes     = (UpeOctreeNode *)malloc(max_nodes * sizeof(UpeOctreeNode));
    tree->item_pool = (UpeOctreeItem *)malloc(max_items  * sizeof(UpeOctreeItem));
    if (!tree->nodes || !tree->item_pool) {
        free(tree->nodes); free(tree->item_pool);
        memset(tree, 0, sizeof(*tree));
        return -1;
    }
    memset(tree->nodes,     0, max_nodes * sizeof(UpeOctreeNode));
    memset(tree->item_pool, 0, max_items  * sizeof(UpeOctreeItem));

    /* Create root */
    tree->root = 0;
    tree->node_count = 1;
    UpeOctreeNode *root = &tree->nodes[0];
    root->min_corner = world_min;
    root->max_corner = world_max;
    root->is_leaf    = 1;
    root->depth      = 0;
    root->item_start = 0;
    root->item_count = 0;
    for (int i = 0; i < 8; i++) root->children[i] = UPE_OCTREE_NULL_NODE;

    return 0;
}

void UpeOctree_Destroy(UpeOctree *tree) {
    if (!tree) return;
    free(tree->nodes);
    free(tree->item_pool);
    memset(tree, 0, sizeof(*tree));
}

void UpeOctree_Clear(UpeOctree *tree) {
    if (!tree || tree->node_count == 0) return;
    tree->item_count = 0;
    /* Reset all nodes to empty leaves */
    for (uint32_t i = 0; i < tree->node_count; i++) {
        tree->nodes[i].item_count = 0;
        tree->nodes[i].item_start = 0;
        tree->nodes[i].is_leaf    = (i == tree->root);
        for (int c = 0; c < 8; c++) tree->nodes[i].children[c] = UPE_OCTREE_NULL_NODE;
    }
    tree->node_count = 1;
}

/* -----------------------------------------------------------------------
   Insert: simplified flat insertion — stores in global pool, associates
   with leaf that best fits item's bounding sphere.
   ----------------------------------------------------------------------- */

int UpeOctree_Insert(UpeOctree *tree, const UpeOctreeItem *item) {
    if (!tree || !item || tree->item_count >= tree->item_capacity) return -1;
    tree->item_pool[tree->item_count++] = *item;
    return 0;
}

void UpeOctree_Remove(UpeOctree *tree, uint32_t user_id) {
    if (!tree) return;
    for (uint32_t i = 0; i < tree->item_count; i++) {
        if (tree->item_pool[i].user_id == user_id) {
            /* Swap with last */
            tree->item_pool[i] = tree->item_pool[--tree->item_count];
            return;
        }
    }
}

/* -----------------------------------------------------------------------
   Queries
   ----------------------------------------------------------------------- */

void UpeOctree_QueryAABB(const UpeOctree *tree, UpeVec3 box_min, UpeVec3 box_max,
                          UpeOctreeResult *result) {
    if (!tree || !result) return;
    result->count = 0;
    for (uint32_t i = 0; i < tree->item_count && result->count < result->capacity; i++) {
        const UpeOctreeItem *it = &tree->item_pool[i];
        UpeVec3 sp_min = { it->center.x - it->radius, it->center.y - it->radius, it->center.z - it->radius };
        UpeVec3 sp_max = { it->center.x + it->radius, it->center.y + it->radius, it->center.z + it->radius };
        if (aabb_overlaps_aabb(box_min, box_max, sp_min, sp_max)) {
            result->ids[result->count++] = it->user_id;
        }
    }
}

void UpeOctree_QuerySphere(const UpeOctree *tree, UpeVec3 center, float radius,
                            UpeOctreeResult *result) {
    if (!tree || !result) return;
    result->count = 0;
    UpeVec3 bmin = { center.x-radius, center.y-radius, center.z-radius };
    UpeVec3 bmax = { center.x+radius, center.y+radius, center.z+radius };
    /* Octree node culling via root-level AABB check */
    const UpeOctreeNode *root = &tree->nodes[tree->root];
    if (!aabb_overlaps_aabb(root->min_corner, root->max_corner, bmin, bmax)) return;

    for (uint32_t i = 0; i < tree->item_count && result->count < result->capacity; i++) {
        if (aabb_overlaps_sphere(bmin, bmax, tree->item_pool[i].center,
                                 tree->item_pool[i].radius + radius)) {
            result->ids[result->count++] = tree->item_pool[i].user_id;
        }
    }
}

uint32_t UpeOctree_NearestPoint(const UpeOctree *tree, UpeVec3 point, float *out_dist_sq) {
    if (!tree || tree->item_count == 0) return UPE_OCTREE_NULL_NODE;
    uint32_t best_id = UPE_OCTREE_NULL_NODE;
    float best_d2 = 1e38f;
    for (uint32_t i = 0; i < tree->item_count; i++) {
        UpeVec3 d = { point.x - tree->item_pool[i].center.x,
                      point.y - tree->item_pool[i].center.y,
                      point.z - tree->item_pool[i].center.z };
        float d2 = d.x*d.x + d.y*d.y + d.z*d.z;
        if (d2 < best_d2) {
            best_d2 = d2;
            best_id = tree->item_pool[i].user_id;
        }
    }
    if (out_dist_sq) *out_dist_sq = best_d2;
    return best_id;
}

uint32_t UpeOctree_Raycast(const UpeOctree *tree,
                            UpeVec3 origin, UpeVec3 dir,
                            float max_dist, float *out_t) {
    if (!tree || tree->item_count == 0) return UPE_OCTREE_NULL_NODE;
    float dl = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
    if (dl < 1e-7f) return UPE_OCTREE_NULL_NODE;
    dir.x /= dl; dir.y /= dl; dir.z /= dl;

    uint32_t best_id = UPE_OCTREE_NULL_NODE;
    float best_t = max_dist;

    for (uint32_t i = 0; i < tree->item_count; i++) {
        const UpeOctreeItem *it = &tree->item_pool[i];
        /* Ray-sphere intersection */
        float ox = origin.x - it->center.x;
        float oy = origin.y - it->center.y;
        float oz = origin.z - it->center.z;
        float b = ox*dir.x + oy*dir.y + oz*dir.z;
        float c = ox*ox + oy*oy + oz*oz - it->radius*it->radius;
        float disc = b*b - c;
        if (disc < 0.0f) continue;
        float t = -b - sqrtf(disc);
        if (t < 0.0f) t = -b + sqrtf(disc);
        if (t >= 0.0f && t < best_t) {
            best_t  = t;
            best_id = it->user_id;
        }
    }
    if (out_t) *out_t = best_t;
    return best_id;
}

uint32_t UpeOctree_NodeCount(const UpeOctree *tree) { return tree ? tree->node_count : 0; }
uint32_t UpeOctree_ItemCount(const UpeOctree *tree) { return tree ? tree->item_count : 0; }
