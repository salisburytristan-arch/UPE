#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeRng {
    uint64_t state;
} UpeRng;

typedef enum UpeRngStreamId {
    UPE_RNG_STREAM_BROADPHASE = 0,
    UPE_RNG_STREAM_SOLVER = 1,
    UPE_RNG_STREAM_PARTICLES = 2,
    UPE_RNG_STREAM_CLOTH = 3,
    UPE_RNG_STREAM_FLUID = 4,
    UPE_RNG_STREAM_GAMEPLAY = 5,
    UPE_RNG_STREAM_COUNT
} UpeRngStreamId;

typedef struct UpeRngSet {
    UpeRng streams[UPE_RNG_STREAM_COUNT];
    uint64_t base_seed;
} UpeRngSet;

void UpeRng_Seed(UpeRng *rng, uint64_t seed);
uint32_t UpeRng_NextU32(UpeRng *rng);
uint64_t UpeRng_NextU64(UpeRng *rng);
float UpeRng_NextF32(UpeRng *rng);

void UpeRngSet_Init(UpeRngSet *set, uint64_t seed);
UpeRng *UpeRngSet_Get(UpeRngSet *set, UpeRngStreamId stream);
uint32_t UpeRngSet_NextU32(UpeRngSet *set, UpeRngStreamId stream);
float UpeRngSet_NextF32(UpeRngSet *set, UpeRngStreamId stream);

#ifdef __cplusplus
}
#endif
