#pragma once

#include <stdint.h>
#include "upe/upe_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeVector {
    void *data;
    uint32_t count;
    uint32_t capacity;
    uint32_t stride;
    UpeAllocator *allocator;
} UpeVector;

void UpeVector_Init(UpeVector *vector, uint32_t stride, UpeAllocator *allocator);
void UpeVector_Destroy(UpeVector *vector);
void UpeVector_Clear(UpeVector *vector);
int UpeVector_Reserve(UpeVector *vector, uint32_t capacity);
int UpeVector_Resize(UpeVector *vector, uint32_t count);
int UpeVector_Push(UpeVector *vector, const void *element);
int UpeVector_Pop(UpeVector *vector, void *out_element);
void *UpeVector_At(UpeVector *vector, uint32_t index);
const void *UpeVector_AtConst(const UpeVector *vector, uint32_t index);

#ifdef __cplusplus
}
#endif
