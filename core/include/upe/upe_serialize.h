#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeSerializeContext {
    uint8_t *buffer;
    uint32_t size;
    uint32_t capacity;
} UpeSerializeContext;

void UpeSerialize_Init(UpeSerializeContext *ctx, uint8_t *buffer, uint32_t capacity);
void UpeSerialize_WriteU32(UpeSerializeContext *ctx, uint32_t value);
void UpeSerialize_WriteF32(UpeSerializeContext *ctx, float value);
void UpeSerialize_WriteBytes(UpeSerializeContext *ctx, const void *data, uint32_t size);

uint32_t UpeDeserialize_ReadU32(UpeSerializeContext *ctx);
float UpeDeserialize_ReadF32(UpeSerializeContext *ctx);
void UpeDeserialize_ReadBytes(UpeSerializeContext *ctx, void *data, uint32_t size);

#ifdef __cplusplus
}
#endif
