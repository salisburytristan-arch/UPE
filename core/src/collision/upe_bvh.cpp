#include "upe/upe_bvh.h"
#include <algorithm>
#include <math.h>
#include <stdlib.h>

struct UpeBvhBuildCtx {
    const UpeAabb3 *bounds;
    uint32_t *indices;
    UpeBvhNode *nodes;
    uint32_t max_nodes;
    uint32_t node_count;
};

static UpeAabb3 UpeBoundsRange(const UpeAabb3 *bounds, const uint32_t *indices, uint32_t start, uint32_t count) {
    UpeAabb3 out = bounds[indices[start]];
    for (uint32_t i = 1; i < count; ++i) {
        out = UpeAabb3_Union(out, bounds[indices[start + i]]);
    }
    return out;
}

static uint32_t UpeBvh_BuildNode(UpeBvhBuildCtx &ctx, uint32_t start, uint32_t count) {
    if (ctx.node_count >= ctx.max_nodes) {
        return UINT32_MAX;
    }

    uint32_t node_index = ctx.node_count++;
    UpeBvhNode &node = ctx.nodes[node_index];
    node.start = start;
    node.count = count;
    node.left = UINT32_MAX;
    node.right = UINT32_MAX;
    node.bounds = UpeBoundsRange(ctx.bounds, ctx.indices, start, count);

    if (count <= 2) {
        return node_index;
    }

    UpeVec3 extent = UpeVec3_Sub(node.bounds.max, node.bounds.min);
    int axis = 0;
    if (extent.y > extent.x && extent.y >= extent.z) {
        axis = 1;
    } else if (extent.z > extent.x && extent.z >= extent.y) {
        axis = 2;
    }

    auto begin = ctx.indices + start;
    auto end = ctx.indices + start + count;
    std::sort(begin, end, [&](uint32_t a, uint32_t b) {
        UpeVec3 ca = UpeAabb3_Center(ctx.bounds[a]);
        UpeVec3 cb = UpeAabb3_Center(ctx.bounds[b]);
        float va = axis == 0 ? ca.x : (axis == 1 ? ca.y : ca.z);
        float vb = axis == 0 ? cb.x : (axis == 1 ? cb.y : cb.z);
        if (va == vb) {
            return a < b;
        }
        return va < vb;
    });

    uint32_t mid = start + count / 2;
    uint32_t left = UpeBvh_BuildNode(ctx, start, mid - start);
    uint32_t right = UpeBvh_BuildNode(ctx, mid, start + count - mid);
    node.left = left;
    node.right = right;
    return node_index;
}

uint32_t UpeBvh_Build(const UpeAabb3 *bounds, uint32_t count, uint32_t *indices,
                      UpeBvhNode *nodes, uint32_t max_nodes) {
    if (!bounds || !indices || !nodes || count == 0 || max_nodes == 0) {
        return 0;
    }

    for (uint32_t i = 0; i < count; ++i) {
        indices[i] = i;
    }

    UpeBvhBuildCtx ctx;
    ctx.bounds = bounds;
    ctx.indices = indices;
    ctx.nodes = nodes;
    ctx.max_nodes = max_nodes;
    ctx.node_count = 0;

    uint32_t root = UpeBvh_BuildNode(ctx, 0, count);
    if (root == UINT32_MAX) {
        return 0;
    }
    return ctx.node_count;
}

uint32_t UpeBvh_QueryAabb(const UpeBvhNode *nodes, uint32_t node_count,
                          const UpeAabb3 *bounds, const uint32_t *indices,
                          UpeAabb3 query, uint32_t *out_indices, uint32_t max_out) {
    if (!nodes || node_count == 0 || !bounds || !indices || !out_indices || max_out == 0) {
        return 0;
    }

    uint32_t stack[64];
    uint32_t stack_count = 0;
    stack[stack_count++] = 0;

    uint32_t out_count = 0;
    while (stack_count) {
        uint32_t node_index = stack[--stack_count];
        const UpeBvhNode &node = nodes[node_index];
        if (!UpeAabb3_Overlap(node.bounds, query)) {
            continue;
        }
        if (node.left == UINT32_MAX && node.right == UINT32_MAX) {
            for (uint32_t i = 0; i < node.count && out_count < max_out; ++i) {
                uint32_t idx = indices[node.start + i];
                if (UpeAabb3_Overlap(bounds[idx], query)) {
                    out_indices[out_count++] = idx;
                }
            }
        } else {
            if (node.left != UINT32_MAX) {
                stack[stack_count++] = node.left;
            }
            if (node.right != UINT32_MAX) {
                stack[stack_count++] = node.right;
            }
        }
    }

    return out_count;
}

/*============================================================================
  Ray-AABB intersection for BVH traversal
============================================================================*/

static int RayAabbIntersect(UpeVec3 origin, UpeVec3 dir, UpeAabb3 box, float max_t) {
    float tmin = 0.0f;
    float tmax = max_t;
    
    for (int i = 0; i < 3; ++i) {
        float d = ((float*)&dir)[i];
        float o = ((float*)&origin)[i];
        float min_val = ((float*)&box.min)[i];
        float max_val = ((float*)&box.max)[i];
        
        if (fabsf(d) < 1.0e-8f) {
            /* Ray parallel to slab */
            if (o < min_val || o > max_val) {
                return 0;
            }
        } else {
            float inv_d = 1.0f / d;
            float t1 = (min_val - o) * inv_d;
            float t2 = (max_val - o) * inv_d;
            
            if (t1 > t2) {
                float temp = t1;
                t1 = t2;
                t2 = temp;
            }
            
            tmin = t1 > tmin ? t1 : tmin;
            tmax = t2 < tmax ? t2 : tmax;
            
            if (tmin > tmax) {
                return 0;
            }
        }
    }
    
    return (tmin >= 0.0f);
}

/*============================================================================
  Raycast query against BVH
============================================================================*/

uint32_t UpeBvh_QueryRay(const UpeBvhNode *nodes, uint32_t node_count,
                         const UpeAabb3 *bounds, const uint32_t *indices,
                         UpeVec3 origin, UpeVec3 direction, float max_distance,
                         uint32_t *out_indices, uint32_t max_out) {
    if (!nodes || node_count == 0 || !bounds || !indices || !out_indices || max_out == 0) {
        return 0;
    }

    /* Normalize direction */
    float dir_len = sqrtf(direction.x * direction.x + 
                          direction.y * direction.y + 
                          direction.z * direction.z);
    if (dir_len < 1.0e-6f) {
        return 0;
    }
    
    UpeVec3 dir = {
        direction.x / dir_len,
        direction.y / dir_len,
        direction.z / dir_len
    };

    uint32_t stack[64];
    uint32_t stack_count = 0;
    stack[stack_count++] = 0;

    uint32_t out_count = 0;
    while (stack_count > 0) {
        uint32_t node_index = stack[--stack_count];
        const UpeBvhNode &node = nodes[node_index];
        
        /* Test ray against node bounds */
        if (!RayAabbIntersect(origin, dir, node.bounds, max_distance)) {
            continue;
        }
        
        /* Leaf node: collect indices */
        if (node.left == UINT32_MAX && node.right == UINT32_MAX) {
            for (uint32_t i = 0; i < node.count && out_count < max_out; ++i) {
                uint32_t idx = indices[node.start + i];
                /* Candidate shape - caller will do detailed ray-shape test */
                out_indices[out_count++] = idx;
            }
        } else {
            /* Interior node: traverse children */
            if (node.right != UINT32_MAX && stack_count < 64) {
                stack[stack_count++] = node.right;
            }
            if (node.left != UINT32_MAX && stack_count < 64) {
                stack[stack_count++] = node.left;
            }
        }
    }

    return out_count;
}

struct UpeBvhPairEmitCtx {
    const UpeBvhNode *nodes;
    const UpeAabb3 *bounds;
    const uint32_t *indices;
    UpeBroadphasePair *out_pairs;
    uint32_t max_pairs;
    uint32_t pair_count;
};

static int UpeBvh_IsLeaf(const UpeBvhNode &node) {
    return (node.left == UINT32_MAX && node.right == UINT32_MAX) ? 1 : 0;
}

static void UpeBvh_EmitLeafPairs(UpeBvhPairEmitCtx *ctx, uint32_t na, uint32_t nb) {
    if (!ctx || na == UINT32_MAX || nb == UINT32_MAX || ctx->pair_count >= ctx->max_pairs) {
        return;
    }

    const UpeBvhNode &a = ctx->nodes[na];
    const UpeBvhNode &b = ctx->nodes[nb];

    if (na == nb) {
        for (uint32_t i = 0; i < a.count && ctx->pair_count < ctx->max_pairs; ++i) {
            uint32_t ia = ctx->indices[a.start + i];
            for (uint32_t j = i + 1; j < a.count && ctx->pair_count < ctx->max_pairs; ++j) {
                uint32_t ib = ctx->indices[a.start + j];
                if (!UpeAabb3_Overlap(ctx->bounds[ia], ctx->bounds[ib])) {
                    continue;
                }
                uint32_t lo = ia < ib ? ia : ib;
                uint32_t hi = ia < ib ? ib : ia;
                ctx->out_pairs[ctx->pair_count++] = {lo, hi};
            }
        }
    } else {
        for (uint32_t i = 0; i < a.count && ctx->pair_count < ctx->max_pairs; ++i) {
            uint32_t ia = ctx->indices[a.start + i];
            for (uint32_t j = 0; j < b.count && ctx->pair_count < ctx->max_pairs; ++j) {
                uint32_t ib = ctx->indices[b.start + j];
                if (ia == ib) {
                    continue;
                }
                if (!UpeAabb3_Overlap(ctx->bounds[ia], ctx->bounds[ib])) {
                    continue;
                }
                uint32_t lo = ia < ib ? ia : ib;
                uint32_t hi = ia < ib ? ib : ia;
                ctx->out_pairs[ctx->pair_count++] = {lo, hi};
            }
        }
    }
}

static void UpeBvh_TraversePair(UpeBvhPairEmitCtx *ctx, uint32_t na, uint32_t nb) {
    if (!ctx || na == UINT32_MAX || nb == UINT32_MAX || ctx->pair_count >= ctx->max_pairs) {
        return;
    }

    const UpeBvhNode &a = ctx->nodes[na];
    const UpeBvhNode &b = ctx->nodes[nb];
    if (!UpeAabb3_Overlap(a.bounds, b.bounds)) {
        return;
    }

    int a_leaf = UpeBvh_IsLeaf(a);
    int b_leaf = UpeBvh_IsLeaf(b);

    if (a_leaf && b_leaf) {
        UpeBvh_EmitLeafPairs(ctx, na, nb);
        return;
    }

    if (na == nb) {
        if (!a_leaf) {
            UpeBvh_TraversePair(ctx, a.left, a.left);
            UpeBvh_TraversePair(ctx, a.left, a.right);
            UpeBvh_TraversePair(ctx, a.right, a.right);
        }
        return;
    }

    if (a_leaf) {
        UpeBvh_TraversePair(ctx, na, b.left);
        UpeBvh_TraversePair(ctx, na, b.right);
        return;
    }

    if (b_leaf) {
        UpeBvh_TraversePair(ctx, a.left, nb);
        UpeBvh_TraversePair(ctx, a.right, nb);
        return;
    }

    UpeBvh_TraversePair(ctx, a.left, b.left);
    UpeBvh_TraversePair(ctx, a.left, b.right);
    UpeBvh_TraversePair(ctx, a.right, b.left);
    UpeBvh_TraversePair(ctx, a.right, b.right);
}

static int UpeBroadphasePairCompare(const void *lhs_ptr, const void *rhs_ptr) {
    const UpeBroadphasePair *lhs = (const UpeBroadphasePair*)lhs_ptr;
    const UpeBroadphasePair *rhs = (const UpeBroadphasePair*)rhs_ptr;
    if (lhs->a < rhs->a) return -1;
    if (lhs->a > rhs->a) return 1;
    if (lhs->b < rhs->b) return -1;
    if (lhs->b > rhs->b) return 1;
    return 0;
}

uint32_t UpeBvh_GeneratePairs(const UpeBvhNode *nodes, uint32_t node_count,
                              const UpeAabb3 *bounds, const uint32_t *indices,
                              UpeBroadphasePair *out_pairs, uint32_t max_pairs) {
    if (!nodes || node_count == 0 || !bounds || !indices || !out_pairs || max_pairs == 0) {
        return 0;
    }

    UpeBvhPairEmitCtx ctx = {};
    ctx.nodes = nodes;
    ctx.bounds = bounds;
    ctx.indices = indices;
    ctx.out_pairs = out_pairs;
    ctx.max_pairs = max_pairs;
    ctx.pair_count = 0;

    UpeBvh_TraversePair(&ctx, 0, 0);

    if (ctx.pair_count > 1) {
        qsort(ctx.out_pairs, ctx.pair_count, sizeof(UpeBroadphasePair), UpeBroadphasePairCompare);
        uint32_t write_idx = 1;
        for (uint32_t i = 1; i < ctx.pair_count; ++i) {
            const UpeBroadphasePair &prev = ctx.out_pairs[write_idx - 1];
            const UpeBroadphasePair &curr = ctx.out_pairs[i];
            if (prev.a == curr.a && prev.b == curr.b) {
                continue;
            }
            ctx.out_pairs[write_idx++] = curr;
        }
        ctx.pair_count = write_idx;
    }

    return ctx.pair_count;
}
