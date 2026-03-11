#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeStateHash {
    uint64_t state;
} UpeStateHash;

void UpeStateHash_Begin(UpeStateHash *hash, uint64_t seed);
void UpeStateHash_PushBytes(UpeStateHash *hash, const void *data, uint32_t size);
void UpeStateHash_PushU32(UpeStateHash *hash, uint32_t value);
void UpeStateHash_PushF32(UpeStateHash *hash, float value);
uint64_t UpeStateHash_Final(const UpeStateHash *hash);

#ifdef __cplusplus
}
#endif
