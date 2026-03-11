#include "upe/upe_hashmap.h"
#include <string.h>

static uint64_t UpeHashU64(uint64_t x) {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}

static uint32_t UpeNextPow2(uint32_t v) {
    if (v < 8) {
        return 8;
    }
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v + 1;
}

static void UpeHashMap64_Allocate(UpeHashMap64 *map, uint32_t capacity) {
    map->keys = (uint64_t *)UpeAlloc(map->allocator, sizeof(uint64_t) * capacity, 16);
    map->values = (uint64_t *)UpeAlloc(map->allocator, sizeof(uint64_t) * capacity, 16);
    map->states = (uint8_t *)UpeAlloc(map->allocator, sizeof(uint8_t) * capacity, 16);
    if (map->states) {
        memset(map->states, 0, sizeof(uint8_t) * capacity);
    }
    map->capacity = capacity;
    map->count = 0;
}

void UpeHashMap64_Init(UpeHashMap64 *map, uint32_t capacity, UpeAllocator *allocator) {
    if (!map) {
        return;
    }
    map->allocator = allocator;
    map->keys = nullptr;
    map->values = nullptr;
    map->states = nullptr;
    map->capacity = 0;
    map->count = 0;

    uint32_t cap = UpeNextPow2(capacity);
    UpeHashMap64_Allocate(map, cap);
}

void UpeHashMap64_Destroy(UpeHashMap64 *map) {
    if (!map) {
        return;
    }
    if (map->keys) {
        UpeFree(map->allocator, map->keys);
        UpeFree(map->allocator, map->values);
        UpeFree(map->allocator, map->states);
    }
    map->keys = nullptr;
    map->values = nullptr;
    map->states = nullptr;
    map->capacity = 0;
    map->count = 0;
}

void UpeHashMap64_Clear(UpeHashMap64 *map) {
    if (!map || !map->states) {
        return;
    }
    memset(map->states, 0, sizeof(uint8_t) * map->capacity);
    map->count = 0;
}

static int UpeHashMap64_Rehash(UpeHashMap64 *map, uint32_t new_capacity) {
    UpeHashMap64 temp = {};
    temp.allocator = map->allocator;
    UpeHashMap64_Allocate(&temp, new_capacity);
    if (!temp.states) {
        return 0;
    }

    for (uint32_t i = 0; i < map->capacity; ++i) {
        if (map->states[i] == 1) {
            uint64_t key = map->keys[i];
            uint64_t value = map->values[i];
            uint32_t mask = temp.capacity - 1;
            uint32_t index = (uint32_t)UpeHashU64(key) & mask;
            while (temp.states[index] == 1) {
                index = (index + 1) & mask;
            }
            temp.keys[index] = key;
            temp.values[index] = value;
            temp.states[index] = 1;
            temp.count++;
        }
    }

    UpeHashMap64_Destroy(map);
    *map = temp;
    return 1;
}

int UpeHashMap64_Set(UpeHashMap64 *map, uint64_t key, uint64_t value) {
    if (!map || map->capacity == 0) {
        return 0;
    }
    if ((map->count + 1) * 10 >= map->capacity * 7) {
        if (!UpeHashMap64_Rehash(map, map->capacity * 2)) {
            return 0;
        }
    }

    uint32_t mask = map->capacity - 1;
    uint32_t index = (uint32_t)UpeHashU64(key) & mask;
    uint32_t tombstone = UINT32_MAX;

    while (map->states[index] != 0) {
        if (map->states[index] == 1 && map->keys[index] == key) {
            map->values[index] = value;
            return 1;
        }
        if (map->states[index] == 2 && tombstone == UINT32_MAX) {
            tombstone = index;
        }
        index = (index + 1) & mask;
    }

    if (tombstone != UINT32_MAX) {
        index = tombstone;
    }
    map->keys[index] = key;
    map->values[index] = value;
    map->states[index] = 1;
    map->count++;
    return 1;
}

int UpeHashMap64_Get(const UpeHashMap64 *map, uint64_t key, uint64_t *out_value) {
    if (!map || map->capacity == 0) {
        return 0;
    }
    uint32_t mask = map->capacity - 1;
    uint32_t index = (uint32_t)UpeHashU64(key) & mask;
    while (map->states[index] != 0) {
        if (map->states[index] == 1 && map->keys[index] == key) {
            if (out_value) {
                *out_value = map->values[index];
            }
            return 1;
        }
        index = (index + 1) & mask;
    }
    return 0;
}

int UpeHashMap64_Remove(UpeHashMap64 *map, uint64_t key) {
    if (!map || map->capacity == 0) {
        return 0;
    }
    uint32_t mask = map->capacity - 1;
    uint32_t index = (uint32_t)UpeHashU64(key) & mask;
    while (map->states[index] != 0) {
        if (map->states[index] == 1 && map->keys[index] == key) {
            map->states[index] = 2;
            map->count--;
            return 1;
        }
        index = (index + 1) & mask;
    }
    return 0;
}
