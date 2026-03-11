#include "upe/upe_rng.h"

static uint64_t UpeSplitMix64(uint64_t *state) {
    uint64_t z = (*state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

void UpeRng_Seed(UpeRng *rng, uint64_t seed) {
    if (!rng) {
        return;
    }
    rng->state = seed ? seed : 0x9E3779B97F4A7C15ULL;
}

uint64_t UpeRng_NextU64(UpeRng *rng) {
    if (!rng) {
        return 0;
    }
    return UpeSplitMix64(&rng->state);
}

uint32_t UpeRng_NextU32(UpeRng *rng) {
    return (uint32_t)(UpeRng_NextU64(rng) >> 32);
}

float UpeRng_NextF32(UpeRng *rng) {
    uint32_t bits = UpeRng_NextU32(rng) >> 8;
    return (float)bits * (1.0f / 16777216.0f);
}

void UpeRngSet_Init(UpeRngSet *set, uint64_t seed) {
    if (!set) {
        return;
    }

    set->base_seed = seed;
    static const uint64_t stream_salts[UPE_RNG_STREAM_COUNT] = {
        0x9E3779B97F4A7C15ULL,
        0xBF58476D1CE4E5B9ULL,
        0x94D049BB133111EBULL,
        0xD1342543DE82EF95ULL,
        0xF1357AEA2E62A9C5ULL,
        0xA0761D6478BD642FULL
    };

    for (uint32_t i = 0; i < UPE_RNG_STREAM_COUNT; ++i) {
        uint64_t stream_seed = seed ^ stream_salts[i];
        UpeRng_Seed(&set->streams[i], stream_seed);
    }
}

UpeRng *UpeRngSet_Get(UpeRngSet *set, UpeRngStreamId stream) {
    if (!set) {
        return 0;
    }
    if ((uint32_t)stream >= (uint32_t)UPE_RNG_STREAM_COUNT) {
        return 0;
    }
    return &set->streams[stream];
}

uint32_t UpeRngSet_NextU32(UpeRngSet *set, UpeRngStreamId stream) {
    UpeRng *rng = UpeRngSet_Get(set, stream);
    return rng ? UpeRng_NextU32(rng) : 0u;
}

float UpeRngSet_NextF32(UpeRngSet *set, UpeRngStreamId stream) {
    UpeRng *rng = UpeRngSet_Get(set, stream);
    return rng ? UpeRng_NextF32(rng) : 0.0f;
}
