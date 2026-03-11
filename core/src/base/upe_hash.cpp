#include "upe/upe_hash.h"
#include <string.h>

static const uint64_t UPE_FNV_OFFSET = 14695981039346656037ULL;
static const uint64_t UPE_FNV_PRIME = 1099511628211ULL;

void UpeHash64_Init(UpeHash64 *hash, uint64_t seed) {
    if (!hash) {
        return;
    }
    hash->state = UPE_FNV_OFFSET ^ seed;
}

void UpeHash64_UpdateBytes(UpeHash64 *hash, const void *data, uint32_t size) {
    if (!hash || !data) {
        return;
    }
    const unsigned char *bytes = (const unsigned char *)data;
    for (uint32_t i = 0; i < size; ++i) {
        hash->state ^= (uint64_t)bytes[i];
        hash->state *= UPE_FNV_PRIME;
    }
}

void UpeHash64_UpdateU32(UpeHash64 *hash, uint32_t value) {
    UpeHash64_UpdateBytes(hash, &value, (uint32_t)sizeof(value));
}

void UpeHash64_UpdateF32(UpeHash64 *hash, float value) {
    uint32_t bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    UpeHash64_UpdateU32(hash, bits);
}

uint64_t UpeHash64_Final(const UpeHash64 *hash) {
    return hash ? hash->state : 0ULL;
}
