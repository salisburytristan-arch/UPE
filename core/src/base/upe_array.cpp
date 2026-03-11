#include "upe/upe_array.h"
#include <string.h>

void UpeArray_Init(UpeArray *array, uint32_t stride, UpeAllocator *allocator) {
    if (!array || stride == 0) {
        return;
    }
    array->data = nullptr;
    array->count = 0;
    array->capacity = 0;
    array->stride = stride;
    array->allocator = allocator;
}

void UpeArray_Destroy(UpeArray *array) {
    if (!array) {
        return;
    }
    if (array->data) {
        UpeFree(array->allocator, array->data);
    }
    array->data = nullptr;
    array->count = 0;
    array->capacity = 0;
    array->stride = 0;
}

void UpeArray_Clear(UpeArray *array) {
    if (!array) {
        return;
    }
    array->count = 0;
}

int UpeArray_Reserve(UpeArray *array, uint32_t capacity) {
    if (!array || array->stride == 0) {
        return 0;
    }
    if (capacity <= array->capacity) {
        return 1;
    }
    size_t new_size = (size_t)capacity * array->stride;
    void *new_data = UpeAlloc(array->allocator, new_size, 16);
    if (!new_data) {
        return 0;
    }
    if (array->data && array->count > 0) {
        size_t old_size = (size_t)array->count * array->stride;
        memcpy(new_data, array->data, old_size);
        UpeFree(array->allocator, array->data);
    }
    array->data = new_data;
    array->capacity = capacity;
    return 1;
}

int UpeArray_Push(UpeArray *array, const void *element) {
    if (!array || !element) {
        return 0;
    }
    if (array->count == array->capacity) {
        uint32_t new_cap = array->capacity ? array->capacity * 2 : 8;
        if (!UpeArray_Reserve(array, new_cap)) {
            return 0;
        }
    }
    char *dst = (char *)array->data + (size_t)array->count * array->stride;
    memcpy(dst, element, array->stride);
    array->count++;
    return 1;
}

void *UpeArray_At(UpeArray *array, uint32_t index) {
    if (!array || index >= array->count) {
        return nullptr;
    }
    return (char *)array->data + (size_t)index * array->stride;
}

const void *UpeArray_AtConst(const UpeArray *array, uint32_t index) {
    if (!array || index >= array->count) {
        return nullptr;
    }
    return (const char *)array->data + (size_t)index * array->stride;
}
