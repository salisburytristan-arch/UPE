#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeHash64 {
    uint64_t state;
} UpeHash64;

void UpeHash64_Init(UpeHash64 *hash, uint64_t seed);
void UpeHash64_UpdateBytes(UpeHash64 *hash, const void *data, uint32_t size);
void UpeHash64_UpdateU32(UpeHash64 *hash, uint32_t value);
void UpeHash64_UpdateF32(UpeHash64 *hash, float value);
uint64_t UpeHash64_Final(const UpeHash64 *hash);

#ifdef __cplusplus
}
#endif
