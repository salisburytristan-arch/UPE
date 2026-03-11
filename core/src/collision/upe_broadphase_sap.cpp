#include "upe/upe_broadphase.h"
#include "upe/upe_bvh.h"
#include <stdlib.h>
#include <string.h>

struct UpeSapEntry {
    uint32_t index;
    float min_x;
    float max_x;
};

static int UpeSapCompare(const void *a, const void *b) {
    const UpeSapEntry *ea = (const UpeSapEntry *)a;
    const UpeSapEntry *eb = (const UpeSapEntry *)b;
    if (ea->min_x < eb->min_x) {
        return -1;
    }
    if (ea->min_x > eb->min_x) {
        return 1;
    }
    return (ea->index < eb->index) ? -1 : (ea->index > eb->index);
}

uint32_t UpeBroadphase_SapPairs(const UpeAabb3 *bounds, uint32_t count,
                                 UpeBroadphasePair *out_pairs, uint32_t max_pairs) {
    if (!bounds || count == 0 || !out_pairs || max_pairs == 0) {
        return 0;
    }

    UpeSapEntry *entries = new UpeSapEntry[count];
    for (uint32_t i = 0; i < count; ++i) {
        entries[i].index = i;
        entries[i].min_x = bounds[i].min.x;
        entries[i].max_x = bounds[i].max.x;
    }

    qsort(entries, count, sizeof(UpeSapEntry), UpeSapCompare);

    uint32_t pair_count = 0;
    for (uint32_t i = 0; i < count; ++i) {
        const UpeSapEntry &a = entries[i];
        for (uint32_t j = i + 1; j < count; ++j) {
            const UpeSapEntry &b = entries[j];
            if (b.min_x > a.max_x) {
                break;
            }
            if (UpeAabb3_Overlap(bounds[a.index], bounds[b.index])) {
                if (pair_count < max_pairs) {
                    out_pairs[pair_count++] = {a.index, b.index};
                }
            }
        }
    }

    delete[] entries;
    return pair_count;
}

uint32_t UpeBroadphase_BvhPairs(const UpeAabb3 *bounds, uint32_t count,
                                 UpeBroadphasePair *out_pairs, uint32_t max_pairs,
                                 void *bvh_scratch, uint32_t scratch_size) {
    if (!bounds || count == 0 || !out_pairs || max_pairs == 0 || !bvh_scratch) {
        return 0;
    }
    
    /* Scratch layout: [indices: count*4][nodes: count*2*sizeof(UpeBvhNode)][query_temp: count*4] */
    uint32_t indices_size = count * sizeof(uint32_t);
    uint32_t nodes_size = count * 2 * sizeof(UpeBvhNode);
    uint32_t query_temp_size = count * sizeof(uint32_t);
    uint32_t required_size = indices_size + nodes_size + query_temp_size;
    
    if (scratch_size < required_size) {
        return 0;
    }
    
    uint8_t *scratch_ptr = (uint8_t*)bvh_scratch;
    uint32_t *indices = (uint32_t*)scratch_ptr;
    UpeBvhNode *nodes = (UpeBvhNode*)(scratch_ptr + indices_size);
    uint32_t *query_temp = (uint32_t*)(scratch_ptr + indices_size + nodes_size);
    (void)query_temp;
    
    /* Build BVH */
    uint32_t node_count = UpeBvh_Build(bounds, count, indices, nodes, count * 2);
    if (node_count == 0) {
        return 0;
    }
    
    return UpeBvh_GeneratePairs(nodes, node_count, bounds, indices, out_pairs, max_pairs);
}
