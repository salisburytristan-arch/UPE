#include "upe/upe_net_delta.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static uint32_t count_diff_blocks(const uint8_t *base_data, const uint8_t *next_data, uint32_t size) {
    uint32_t blocks = 0;
    uint32_t i = 0;
    while (i < size) {
        if (base_data[i] == next_data[i]) {
            ++i;
            continue;
        }
        ++blocks;
        while (i < size && base_data[i] != next_data[i]) {
            ++i;
        }
    }
    return blocks;
}

static uint32_t calc_diff_size(const uint8_t *base_data, const uint8_t *next_data, uint32_t size, uint32_t blocks) {
    uint32_t total = sizeof(uint32_t); // block count
    if (blocks == 0) {
        return total;
    }

    uint32_t i = 0;
    while (i < size) {
        if (base_data[i] == next_data[i]) {
            ++i;
            continue;
        }
        uint32_t start = i;
        while (i < size && base_data[i] != next_data[i]) {
            ++i;
        }
        uint32_t len = i - start;
        total += sizeof(uint32_t) * 2 + len;
    }

    return total;
}

UpeNetDelta *UpeNetDelta_Create(uint32_t frame, const void *data, uint32_t size) {
    if (!data || size == 0) {
        return nullptr;
    }

    UpeNetDelta *delta = (UpeNetDelta *)malloc(sizeof(UpeNetDelta));
    if (!delta) {
        return nullptr;
    }

    delta->frame = frame;
    delta->size = size;
    delta->data = (uint8_t *)malloc(size);
    if (!delta->data) {
        free(delta);
        return nullptr;
    }

    memcpy(delta->data, data, size);
    return delta;
}

UpeNetDelta *UpeNetDelta_CreateDiff(uint32_t frame, const void *base_data, const void *next_data, uint32_t size) {
    if (!base_data || !next_data || size == 0) {
        return nullptr;
    }

    const uint8_t *base_bytes = (const uint8_t *)base_data;
    const uint8_t *next_bytes = (const uint8_t *)next_data;

    uint32_t blocks = count_diff_blocks(base_bytes, next_bytes, size);
    uint32_t total_size = calc_diff_size(base_bytes, next_bytes, size, blocks);

    UpeNetDelta *delta = (UpeNetDelta *)malloc(sizeof(UpeNetDelta));
    if (!delta) {
        return nullptr;
    }

    delta->frame = frame;
    delta->size = total_size;
    delta->data = (uint8_t *)malloc(total_size);
    if (!delta->data) {
        free(delta);
        return nullptr;
    }

    uint8_t *cursor = delta->data;
    memcpy(cursor, &blocks, sizeof(uint32_t));
    cursor += sizeof(uint32_t);

    if (blocks == 0) {
        return delta;
    }

    uint32_t i = 0;
    while (i < size) {
        if (base_bytes[i] == next_bytes[i]) {
            ++i;
            continue;
        }
        uint32_t start = i;
        while (i < size && base_bytes[i] != next_bytes[i]) {
            ++i;
        }
        uint32_t len = i - start;

        memcpy(cursor, &start, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        memcpy(cursor, &len, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        memcpy(cursor, next_bytes + start, len);
        cursor += len;
    }

    return delta;
}

int UpeNetDelta_Apply(const UpeNetDelta *delta, const void *base_data, uint32_t base_size,
                      void *out_data, uint32_t out_size) {
    if (!delta || !base_data || !out_data) {
        return 0;
    }
    if (base_size == 0 || out_size < base_size) {
        return 0;
    }
    if (!delta->data || delta->size < sizeof(uint32_t)) {
        return 0;
    }

    memcpy(out_data, base_data, base_size);

    const uint8_t *cursor = delta->data;
    uint32_t blocks = 0;
    memcpy(&blocks, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);

    for (uint32_t b = 0; b < blocks; ++b) {
        if ((uint32_t)(cursor - delta->data) + sizeof(uint32_t) * 2 > delta->size) {
            return 0;
        }
        uint32_t offset = 0;
        uint32_t len = 0;
        memcpy(&offset, cursor, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        memcpy(&len, cursor, sizeof(uint32_t));
        cursor += sizeof(uint32_t);

        if (offset + len > base_size) {
            return 0;
        }
        if ((uint32_t)(cursor - delta->data) + len > delta->size) {
            return 0;
        }

        memcpy((uint8_t *)out_data + offset, cursor, len);
        cursor += len;
    }

    return 1;
}

uint16_t UpeNetDelta_QuantizeF32(float value, float min_value, float max_value, uint32_t bits) {
    if (bits == 0) {
        return 0;
    }
    if (max_value <= min_value) {
        return 0;
    }
    if (value < min_value) value = min_value;
    if (value > max_value) value = max_value;
    float range = max_value - min_value;
    float normalized = (value - min_value) / range;
    float max_q = (float)((1u << bits) - 1u);
    uint32_t q = (uint32_t)floorf(normalized * max_q + 0.5f);
    if (q > (uint32_t)max_q) {
        q = (uint32_t)max_q;
    }
    return (uint16_t)q;
}

float UpeNetDelta_DequantizeF32(uint16_t value, float min_value, float max_value, uint32_t bits) {
    if (bits == 0) {
        return min_value;
    }
    if (max_value <= min_value) {
        return min_value;
    }
    float max_q = (float)((1u << bits) - 1u);
    float normalized = (float)value / max_q;
    return min_value + normalized * (max_value - min_value);
}

void UpeNetDelta_QuantizeVec3(UpeVec3 value, UpeVec3 min_value, UpeVec3 max_value,
                              uint32_t bits, uint16_t out_q[3]) {
    if (!out_q) {
        return;
    }
    out_q[0] = UpeNetDelta_QuantizeF32(value.x, min_value.x, max_value.x, bits);
    out_q[1] = UpeNetDelta_QuantizeF32(value.y, min_value.y, max_value.y, bits);
    out_q[2] = UpeNetDelta_QuantizeF32(value.z, min_value.z, max_value.z, bits);
}

UpeVec3 UpeNetDelta_DequantizeVec3(const uint16_t q[3], UpeVec3 min_value, UpeVec3 max_value,
                                   uint32_t bits) {
    UpeVec3 out = {0.0f, 0.0f, 0.0f};
    if (!q) {
        return out;
    }
    out.x = UpeNetDelta_DequantizeF32(q[0], min_value.x, max_value.x, bits);
    out.y = UpeNetDelta_DequantizeF32(q[1], min_value.y, max_value.y, bits);
    out.z = UpeNetDelta_DequantizeF32(q[2], min_value.z, max_value.z, bits);
    return out;
}

void UpeNetDelta_Destroy(UpeNetDelta *delta) {
    if (!delta) {
        return;
    }
    free(delta->data);
    free(delta);
}

void UpeNetDelta_DeadReckoning(UpeVec3 pos, UpeVec3 vel, UpeVec3 accel,
                                 float dt, UpeVec3 *out_pos) {
    if (!out_pos) return;
    out_pos->x = pos.x + vel.x * dt + 0.5f * accel.x * dt * dt;
    out_pos->y = pos.y + vel.y * dt + 0.5f * accel.y * dt * dt;
    out_pos->z = pos.z + vel.z * dt + 0.5f * accel.z * dt * dt;
}

int UpeNetDelta_RequestOwnership(UpeOwnershipState *state, uint32_t requester_id) {
    if (!state) return 0;
    if (state->owner_id == requester_id) return 1;
    /* Simple immediate handoff with authority version bump */
    state->owner_id = requester_id;
    state->authority_version += 1u;
    state->handoff_timer = 0.0f;
    return 1;
}
