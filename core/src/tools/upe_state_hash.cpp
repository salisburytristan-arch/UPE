#include "upe/upe_state_hash.h"
#include "upe/upe_hash.h"

void UpeStateHash_Begin(UpeStateHash *hash, uint64_t seed) {
    if (!hash) {
        return;
    }
    UpeHash64 h;
    UpeHash64_Init(&h, seed);
    hash->state = h.state;
}

void UpeStateHash_PushBytes(UpeStateHash *hash, const void *data, uint32_t size) {
    if (!hash) {
        return;
    }
    UpeHash64 h;
    h.state = hash->state;
    UpeHash64_UpdateBytes(&h, data, size);
    hash->state = h.state;
}

void UpeStateHash_PushU32(UpeStateHash *hash, uint32_t value) {
    UpeStateHash_PushBytes(hash, &value, (uint32_t)sizeof(value));
}

void UpeStateHash_PushF32(UpeStateHash *hash, float value) {
    UpeStateHash_PushBytes(hash, &value, (uint32_t)sizeof(value));
}

uint64_t UpeStateHash_Final(const UpeStateHash *hash) {
    return hash ? hash->state : 0ULL;
}
