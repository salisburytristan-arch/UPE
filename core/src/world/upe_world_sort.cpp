#include "upe_world_sort.h"

#include "upe_world.h"
#include "upe/upe_broadphase.h"
#include "upe/upe_memory_context.h"
#include "upe/upe_frame_allocator.h"
#include "upe/upe_solver.h"
#include "upe/upe_joint.h"

#include <stdlib.h>
#include <string.h>

static uint64_t UpeWorldSort_BodyPairKey(uint32_t a, uint32_t b) {
    uint32_t lo = (a < b) ? a : b;
    uint32_t hi = (a < b) ? b : a;
    return ((uint64_t)lo << 32) | (uint64_t)hi;
}

static uint32_t UpeWorldSort_ShapeIndex(const UpeWorld *world, const UpeShape *shape) {
    if (!world || !world->shapes || !shape) {
        return 0xFFFFFFFFu;
    }
    ptrdiff_t idx = shape - world->shapes;
    if (idx < 0 || (uint32_t)idx >= world->shape_capacity) {
        return 0xFFFFFFFFu;
    }
    return (uint32_t)idx;
}

static uint32_t UpeWorldSort_Quantize10(float value) {
    float shifted = value * 64.0f + 1024.0f;
    if (shifted < 0.0f) {
        shifted = 0.0f;
    }
    if (shifted > 2047.0f) {
        shifted = 2047.0f;
    }
    return (uint32_t)shifted;
}

static void *UpeWorldSort_AllocTemp(UpeMemoryContext *memory, size_t size, uint32_t align, int *heap_allocated) {
    if (heap_allocated) {
        *heap_allocated = 0;
    }
    if (size == 0) {
        return nullptr;
    }
    if (memory && memory->frame_arena) {
        return UpeFrameArena_Alloc(memory->frame_arena, (uint32_t)size, align);
    }
    if (heap_allocated) {
        *heap_allocated = 1;
    }
    return malloc(size);
}

static void UpeWorldSort_CountingPass8(const uint64_t *src_keys,
                                       uint64_t *dst_keys,
                                       const uint8_t *src_records,
                                       uint8_t *dst_records,
                                       uint32_t count,
                                       uint32_t stride,
                                       uint32_t shift_bits) {
    uint32_t histogram[256];
    uint32_t offsets[256];

    for (uint32_t i = 0; i < 256; ++i) {
        histogram[i] = 0;
    }

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t bucket = (uint32_t)((src_keys[i] >> shift_bits) & 0xFFull);
        histogram[bucket] += 1u;
    }

    uint32_t running = 0;
    for (uint32_t i = 0; i < 256; ++i) {
        offsets[i] = running;
        running += histogram[i];
    }

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t bucket = (uint32_t)((src_keys[i] >> shift_bits) & 0xFFull);
        uint32_t out_idx = offsets[bucket]++;
        dst_keys[out_idx] = src_keys[i];
        memcpy(dst_records + (size_t)out_idx * stride,
               src_records + (size_t)i * stride,
               stride);
    }
}

static void UpeWorldSort_Radix8_u64(void *records,
                                    uint32_t count,
                                    uint32_t stride,
                                    uint64_t *keys,
                                    uint8_t *scratch_records,
                                    uint64_t *scratch_keys) {
    if (!records || !keys || !scratch_records || !scratch_keys || count <= 1u || stride == 0u) {
        return;
    }

    uint8_t *src_records = (uint8_t*)records;
    uint8_t *dst_records = scratch_records;
    uint64_t *src_keys = keys;
    uint64_t *dst_keys = scratch_keys;

    for (uint32_t pass = 0; pass < 8; ++pass) {
        uint32_t shift_bits = pass * 8u;
        UpeWorldSort_CountingPass8(src_keys, dst_keys, src_records, dst_records, count, stride, shift_bits);

        uint8_t *tmp_records = src_records;
        src_records = dst_records;
        dst_records = tmp_records;

        uint64_t *tmp_keys = src_keys;
        src_keys = dst_keys;
        dst_keys = tmp_keys;
    }

    if (src_records != (uint8_t*)records) {
        memcpy(records, src_records, (size_t)count * stride);
    }
    if (src_keys != keys) {
        memcpy(keys, src_keys, (size_t)count * sizeof(uint64_t));
    }
}

void UpeWorldSort_BroadphasePairs(UpeBroadphasePair *pairs,
                                  uint32_t pair_count,
                                  const uint32_t *body_indices,
                                  UpeMemoryContext *memory) {
    if (!pairs || !body_indices || pair_count <= 1u) {
        return;
    }

    int heap_records = 0;
    int heap_keys = 0;
    int heap_scratch_keys = 0;

    uint8_t *scratch_records = (uint8_t*)UpeWorldSort_AllocTemp(memory,
                                                                 (size_t)pair_count * sizeof(UpeBroadphasePair),
                                                                 16,
                                                                 &heap_records);
    uint64_t *keys = (uint64_t*)UpeWorldSort_AllocTemp(memory,
                                                       (size_t)pair_count * sizeof(uint64_t),
                                                       16,
                                                       &heap_keys);
    uint64_t *scratch_keys = (uint64_t*)UpeWorldSort_AllocTemp(memory,
                                                               (size_t)pair_count * sizeof(uint64_t),
                                                               16,
                                                               &heap_scratch_keys);
    if (!scratch_records || !keys || !scratch_keys) {
        if (heap_records && scratch_records) free(scratch_records);
        if (heap_keys && keys) free(keys);
        if (heap_scratch_keys && scratch_keys) free(scratch_keys);
        return;
    }

    for (uint32_t i = 0; i < pair_count; ++i) {
        uint32_t body_a = body_indices[pairs[i].a];
        uint32_t body_b = body_indices[pairs[i].b];
        uint64_t pair_key = UpeWorldSort_BodyPairKey(body_a, body_b);
        uint64_t tie_key = ((uint64_t)pairs[i].a << 32) | (uint64_t)pairs[i].b;
        keys[i] = pair_key ^ (tie_key & 0x00000000FFFFFFFFull);
    }

    UpeWorldSort_Radix8_u64(pairs,
                            pair_count,
                            (uint32_t)sizeof(UpeBroadphasePair),
                            keys,
                            scratch_records,
                            scratch_keys);

    if (heap_records) free(scratch_records);
    if (heap_keys) free(keys);
    if (heap_scratch_keys) free(scratch_keys);
}

void UpeWorldSort_Contacts(UpeWorld *world,
                           UpeContactPair *contacts,
                           uint32_t contact_count,
                           UpeMemoryContext *memory) {
    if (!world || !contacts || contact_count <= 1u) {
        return;
    }

    int heap_records = 0;
    int heap_keys = 0;
    int heap_scratch_keys = 0;

    uint8_t *scratch_records = (uint8_t*)UpeWorldSort_AllocTemp(memory,
                                                                 (size_t)contact_count * sizeof(UpeContactPair),
                                                                 16,
                                                                 &heap_records);
    uint64_t *keys = (uint64_t*)UpeWorldSort_AllocTemp(memory,
                                                       (size_t)contact_count * sizeof(uint64_t),
                                                       16,
                                                       &heap_keys);
    uint64_t *scratch_keys = (uint64_t*)UpeWorldSort_AllocTemp(memory,
                                                               (size_t)contact_count * sizeof(uint64_t),
                                                               16,
                                                               &heap_scratch_keys);

    if (!scratch_records || !keys || !scratch_keys) {
        if (heap_records && scratch_records) free(scratch_records);
        if (heap_keys && keys) free(keys);
        if (heap_scratch_keys && scratch_keys) free(scratch_keys);
        return;
    }

    for (uint32_t i = 0; i < contact_count; ++i) {
        const UpeContactPair &c = contacts[i];
        uint64_t pair_key = UpeWorldSort_BodyPairKey(c.body_a, c.body_b);
        uint32_t shape_a = UpeWorldSort_ShapeIndex(world, c.shape_a);
        uint32_t shape_b = UpeWorldSort_ShapeIndex(world, c.shape_b);
        uint32_t tie_low = ((shape_a & 0xFFFFu) << 16) | (shape_b & 0xFFFFu);
        keys[i] = pair_key ^ (uint64_t)(tie_low ^ (c.manifold.count & 0xFFu));
    }

    UpeWorldSort_Radix8_u64(contacts,
                            contact_count,
                            (uint32_t)sizeof(UpeContactPair),
                            keys,
                            scratch_records,
                            scratch_keys);

    if (heap_records) free(scratch_records);
    if (heap_keys) free(keys);
    if (heap_scratch_keys) free(scratch_keys);
}

void UpeWorldSort_Constraints(UpeSolverConstraint *constraints,
                              uint32_t constraint_count,
                              UpeMemoryContext *memory) {
    if (!constraints || constraint_count <= 1u) {
        return;
    }

    int heap_records = 0;
    int heap_keys = 0;
    int heap_scratch_keys = 0;

    uint8_t *scratch_records = (uint8_t*)UpeWorldSort_AllocTemp(memory,
                                                                 (size_t)constraint_count * sizeof(UpeSolverConstraint),
                                                                 16,
                                                                 &heap_records);
    uint64_t *keys = (uint64_t*)UpeWorldSort_AllocTemp(memory,
                                                       (size_t)constraint_count * sizeof(uint64_t),
                                                       16,
                                                       &heap_keys);
    uint64_t *scratch_keys = (uint64_t*)UpeWorldSort_AllocTemp(memory,
                                                               (size_t)constraint_count * sizeof(uint64_t),
                                                               16,
                                                               &heap_scratch_keys);

    if (!scratch_records || !keys || !scratch_keys) {
        if (heap_records && scratch_records) free(scratch_records);
        if (heap_keys && keys) free(keys);
        if (heap_scratch_keys && scratch_keys) free(scratch_keys);
        return;
    }

    for (uint32_t i = 0; i < constraint_count; ++i) {
        const UpeSolverConstraint &c = constraints[i];
        uint64_t pair_key = UpeWorldSort_BodyPairKey(c.body_a, c.body_b);
        uint32_t qx = UpeWorldSort_Quantize10(c.contact_point.x);
        uint32_t qy = UpeWorldSort_Quantize10(c.contact_point.y);
        uint32_t qz = UpeWorldSort_Quantize10(c.contact_point.z);
        uint32_t contact_pack = (qx & 0x3FFu) | ((qy & 0x3FFu) << 10) | ((qz & 0x3FFu) << 20);
        keys[i] = pair_key ^ (uint64_t)contact_pack;
    }

    UpeWorldSort_Radix8_u64(constraints,
                            constraint_count,
                            (uint32_t)sizeof(UpeSolverConstraint),
                            keys,
                            scratch_records,
                            scratch_keys);

    if (heap_records) free(scratch_records);
    if (heap_keys) free(keys);
    if (heap_scratch_keys) free(scratch_keys);
}

void UpeWorldSort_Joints(UpeJoint *joints,
                         uint32_t joint_count,
                         UpeMemoryContext *memory) {
    if (!joints || joint_count <= 1u) {
        return;
    }

    int heap_records = 0;
    int heap_keys = 0;
    int heap_scratch_keys = 0;

    uint8_t *scratch_records = (uint8_t*)UpeWorldSort_AllocTemp(memory,
                                                                 (size_t)joint_count * sizeof(UpeJoint),
                                                                 16,
                                                                 &heap_records);
    uint64_t *keys = (uint64_t*)UpeWorldSort_AllocTemp(memory,
                                                       (size_t)joint_count * sizeof(uint64_t),
                                                       16,
                                                       &heap_keys);
    uint64_t *scratch_keys = (uint64_t*)UpeWorldSort_AllocTemp(memory,
                                                               (size_t)joint_count * sizeof(uint64_t),
                                                               16,
                                                               &heap_scratch_keys);

    if (!scratch_records || !keys || !scratch_keys) {
        if (heap_records && scratch_records) free(scratch_records);
        if (heap_keys && keys) free(keys);
        if (heap_scratch_keys && scratch_keys) free(scratch_keys);
        return;
    }

    for (uint32_t i = 0; i < joint_count; ++i) {
        const UpeJoint &j = joints[i];
        uint64_t pair_key = UpeWorldSort_BodyPairKey(j.body_a, j.body_b);
        keys[i] = pair_key ^ ((uint64_t)j.type & 0xFFu);
    }

    UpeWorldSort_Radix8_u64(joints,
                            joint_count,
                            (uint32_t)sizeof(UpeJoint),
                            keys,
                            scratch_records,
                            scratch_keys);

    if (heap_records) free(scratch_records);
    if (heap_keys) free(keys);
    if (heap_scratch_keys) free(scratch_keys);
}
