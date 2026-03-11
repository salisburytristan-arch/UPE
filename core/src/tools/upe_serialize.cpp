#include "upe/upe_serialize.h"
#include <string.h>

void UpeSerialize_Init(UpeSerializeContext *ctx, uint8_t *buffer, uint32_t capacity) {
    if (!ctx) {
        return;
    }
    ctx->buffer = buffer;
    ctx->size = 0;
    ctx->capacity = capacity;
}

void UpeSerialize_WriteU32(UpeSerializeContext *ctx, uint32_t value) {
    if (!ctx || ctx->size + 4 > ctx->capacity) {
        return;
    }
    memcpy(ctx->buffer + ctx->size, &value, 4);
    ctx->size += 4;
}

void UpeSerialize_WriteF32(UpeSerializeContext *ctx, float value) {
    if (!ctx || ctx->size + 4 > ctx->capacity) {
        return;
    }
    memcpy(ctx->buffer + ctx->size, &value, 4);
    ctx->size += 4;
}

void UpeSerialize_WriteBytes(UpeSerializeContext *ctx, const void *data, uint32_t size) {
    if (!ctx || !data || ctx->size + size > ctx->capacity) {
        return;
    }
    memcpy(ctx->buffer + ctx->size, data, size);
    ctx->size += size;
}

uint32_t UpeDeserialize_ReadU32(UpeSerializeContext *ctx) {
    if (!ctx || ctx->size + 4 > ctx->capacity) {
        return 0;
    }
    uint32_t value;
    memcpy(&value, ctx->buffer + ctx->size, 4);
    ctx->size += 4;
    return value;
}

float UpeDeserialize_ReadF32(UpeSerializeContext *ctx) {
    if (!ctx || ctx->size + 4 > ctx->capacity) {
        return 0.0f;
    }
    float value;
    memcpy(&value, ctx->buffer + ctx->size, 4);
    ctx->size += 4;
    return value;
}

void UpeDeserialize_ReadBytes(UpeSerializeContext *ctx, void *data, uint32_t size) {
    if (!ctx || !data || ctx->size + size > ctx->capacity) {
        return;
    }
    memcpy(data, ctx->buffer + ctx->size, size);
    ctx->size += size;
}
