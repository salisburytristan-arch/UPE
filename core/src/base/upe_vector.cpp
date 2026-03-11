#include "upe/upe_vector.h"
#include <string.h>

void UpeVector_Init(UpeVector *vector, uint32_t stride, UpeAllocator *allocator) {
    if (!vector || stride == 0) {
        return;
    }
    vector->data = nullptr;
    vector->count = 0;
    vector->capacity = 0;
    vector->stride = stride;
    vector->allocator = allocator;
}

void UpeVector_Destroy(UpeVector *vector) {
    if (!vector) {
        return;
    }
    if (vector->data) {
        UpeFree(vector->allocator, vector->data);
    }
    vector->data = nullptr;
    vector->count = 0;
    vector->capacity = 0;
    vector->stride = 0;
}

void UpeVector_Clear(UpeVector *vector) {
    if (!vector) {
        return;
    }
    vector->count = 0;
}

int UpeVector_Reserve(UpeVector *vector, uint32_t capacity) {
    if (!vector || vector->stride == 0) {
        return 0;
    }
    if (capacity <= vector->capacity) {
        return 1;
    }
    size_t new_size = (size_t)capacity * vector->stride;
    void *new_data = UpeAlloc(vector->allocator, new_size, 16);
    if (!new_data) {
        return 0;
    }
    if (vector->data && vector->count > 0) {
        size_t old_size = (size_t)vector->count * vector->stride;
        memcpy(new_data, vector->data, old_size);
        UpeFree(vector->allocator, vector->data);
    }
    vector->data = new_data;
    vector->capacity = capacity;
    return 1;
}

int UpeVector_Resize(UpeVector *vector, uint32_t count) {
    if (!vector) {
        return 0;
    }
    if (count > vector->capacity) {
        if (!UpeVector_Reserve(vector, count)) {
            return 0;
        }
    }
    vector->count = count;
    return 1;
}

int UpeVector_Push(UpeVector *vector, const void *element) {
    if (!vector || !element) {
        return 0;
    }
    if (vector->count == vector->capacity) {
        uint32_t new_cap = vector->capacity ? vector->capacity * 2 : 8;
        if (!UpeVector_Reserve(vector, new_cap)) {
            return 0;
        }
    }
    char *dst = (char *)vector->data + (size_t)vector->count * vector->stride;
    memcpy(dst, element, vector->stride);
    vector->count++;
    return 1;
}

int UpeVector_Pop(UpeVector *vector, void *out_element) {
    if (!vector || vector->count == 0) {
        return 0;
    }
    vector->count--;
    if (out_element) {
        char *src = (char *)vector->data + (size_t)vector->count * vector->stride;
        memcpy(out_element, src, vector->stride);
    }
    return 1;
}

void *UpeVector_At(UpeVector *vector, uint32_t index) {
    if (!vector || index >= vector->count) {
        return nullptr;
    }
    return (char *)vector->data + (size_t)index * vector->stride;
}

const void *UpeVector_AtConst(const UpeVector *vector, uint32_t index) {
    if (!vector || index >= vector->count) {
        return nullptr;
    }
    return (const char *)vector->data + (size_t)index * vector->stride;
}
