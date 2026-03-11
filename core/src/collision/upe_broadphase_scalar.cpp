#include "upe/upe_broadphase_scalar.h"

#include <stdlib.h>

struct UpeScalarSapEntry {
    uint32_t index;
    UpeScalar min_x;
    UpeScalar max_x;
};

static int UpeScalarSapCompare(const void *a, const void *b) {
    const UpeScalarSapEntry *ea = (const UpeScalarSapEntry *)a;
    const UpeScalarSapEntry *eb = (const UpeScalarSapEntry *)b;

    int cmp = UpeScalar_Cmp(ea->min_x, eb->min_x);
    if (cmp != 0) {
        return cmp;
    }
    if (ea->index < eb->index) {
        return -1;
    }
    if (ea->index > eb->index) {
        return 1;
    }
    return 0;
}

static int UpeScalarPairCompare(const void *a, const void *b) {
    const UpeScalarBroadphasePair *pa = (const UpeScalarBroadphasePair *)a;
    const UpeScalarBroadphasePair *pb = (const UpeScalarBroadphasePair *)b;
    if (pa->a < pb->a) {
        return -1;
    }
    if (pa->a > pb->a) {
        return 1;
    }
    if (pa->b < pb->b) {
        return -1;
    }
    if (pa->b > pb->b) {
        return 1;
    }
    return 0;
}

static int UpeScalarCollisionFilterPass(const uint32_t *collision_layers,
                                        const uint32_t *collision_masks,
                                        uint32_t a,
                                        uint32_t b) {
    if (!collision_layers || !collision_masks) {
        return 1;
    }

    const uint32_t layer_a = collision_layers[a];
    const uint32_t layer_b = collision_layers[b];
    if (layer_a >= 32 || layer_b >= 32) {
        return 0;
    }

    const uint32_t mask_a = collision_masks[a];
    const uint32_t mask_b = collision_masks[b];
    const uint32_t bit_a = 1u << layer_a;
    const uint32_t bit_b = 1u << layer_b;
    return ((mask_a & bit_b) != 0u) && ((mask_b & bit_a) != 0u);
}

static uint32_t UpeBroadphaseScalar_SapPairsInternal(const UpeScalarAabb3 *bounds,
                                                     const uint32_t *collision_layers,
                                                     const uint32_t *collision_masks,
                                                     uint32_t count,
                                                     UpeScalarBroadphasePair *out_pairs,
                                                     uint32_t max_pairs) {
    if (!bounds || count == 0 || !out_pairs || max_pairs == 0) {
        return 0;
    }

    UpeScalarSapEntry *entries = new UpeScalarSapEntry[count];
    for (uint32_t i = 0; i < count; ++i) {
        entries[i].index = i;
        entries[i].min_x = bounds[i].min.x;
        entries[i].max_x = bounds[i].max.x;
    }

    qsort(entries, count, sizeof(UpeScalarSapEntry), UpeScalarSapCompare);

    uint32_t pair_count = 0;
    for (uint32_t i = 0; i < count; ++i) {
        const UpeScalarSapEntry &a = entries[i];
        for (uint32_t j = i + 1; j < count; ++j) {
            const UpeScalarSapEntry &b = entries[j];

            if (UpeScalar_Cmp(b.min_x, a.max_x) > 0) {
                break;
            }

            if (!UpeScalarCollisionFilterPass(collision_layers, collision_masks, a.index, b.index)) {
                continue;
            }

            if (UpeScalarAabb3_Overlap(bounds[a.index], bounds[b.index])) {
                if (pair_count < max_pairs) {
                    uint32_t ai = a.index < b.index ? a.index : b.index;
                    uint32_t bi = a.index < b.index ? b.index : a.index;
                    out_pairs[pair_count++] = {ai, bi};
                }
            }
        }
    }

    if (pair_count > 1) {
        qsort(out_pairs, pair_count, sizeof(UpeScalarBroadphasePair), UpeScalarPairCompare);
    }

    delete[] entries;
    return pair_count;
}

uint32_t UpeBroadphaseScalar_SapPairs(const UpeScalarAabb3 *bounds, uint32_t count,
                                      UpeScalarBroadphasePair *out_pairs, uint32_t max_pairs) {
    return UpeBroadphaseScalar_SapPairsInternal(bounds, nullptr, nullptr, count, out_pairs, max_pairs);
}

uint32_t UpeBroadphaseScalar_SapPairsFiltered(const UpeScalarAabb3 *bounds,
                                              const uint32_t *collision_layers,
                                              const uint32_t *collision_masks,
                                              uint32_t count,
                                              UpeScalarBroadphasePair *out_pairs,
                                              uint32_t max_pairs) {
    return UpeBroadphaseScalar_SapPairsInternal(bounds, collision_layers, collision_masks, count, out_pairs, max_pairs);
}
