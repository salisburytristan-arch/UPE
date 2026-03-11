#pragma once

#include <stdint.h>
#include "upe/upe_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeArray {
    void *data;
    uint32_t count;
    uint32_t capacity;
    uint32_t stride;
    UpeAllocator *allocator;
} UpeArray;

void UpeArray_Init(UpeArray *array, uint32_t stride, UpeAllocator *allocator);
void UpeArray_Destroy(UpeArray *array);
void UpeArray_Clear(UpeArray *array);
int UpeArray_Reserve(UpeArray *array, uint32_t capacity);
int UpeArray_Push(UpeArray *array, const void *element);
void *UpeArray_At(UpeArray *array, uint32_t index);
const void *UpeArray_AtConst(const UpeArray *array, uint32_t index);

#ifdef __cplusplus
}
#endif
