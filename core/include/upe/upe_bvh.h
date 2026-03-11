#pragma once

#include <stdint.h>
#include "upe/upe_geometry.h"
#include "upe/upe_broadphase.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeBvhNode {
    UpeAabb3 bounds;
    uint32_t left;
    uint32_t right;
    uint32_t start;
    uint32_t count;
} UpeBvhNode;

uint32_t UpeBvh_Build(const UpeAabb3 *bounds, uint32_t count, uint32_t *indices,
                      UpeBvhNode *nodes, uint32_t max_nodes);

uint32_t UpeBvh_QueryAabb(const UpeBvhNode *nodes, uint32_t node_count,
                          const UpeAabb3 *bounds, const uint32_t *indices,
                          UpeAabb3 query, uint32_t *out_indices, uint32_t max_out);

/* Raycast query against BVH */
uint32_t UpeBvh_QueryRay(const UpeBvhNode *nodes, uint32_t node_count,
                         const UpeAabb3 *bounds, const uint32_t *indices,
                         UpeVec3 origin, UpeVec3 direction, float max_distance,
                         uint32_t *out_indices, uint32_t max_out);

/* Generate unique overlap pairs directly via node-node BVH traversal */
uint32_t UpeBvh_GeneratePairs(const UpeBvhNode *nodes, uint32_t node_count,
                              const UpeAabb3 *bounds, const uint32_t *indices,
                              UpeBroadphasePair *out_pairs, uint32_t max_pairs);

#ifdef __cplusplus
}
#endif
