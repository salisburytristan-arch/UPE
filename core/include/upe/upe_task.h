#pragma once

#include <stdint.h>
#include "upe/upe_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*UpeTaskFn)(void *user);

typedef struct UpeTaskSystemDesc {
    uint32_t max_tasks;
    UpeAllocator *allocator;
} UpeTaskSystemDesc;

typedef struct UpeTaskSystem UpeTaskSystem;

UpeTaskSystem *UpeTaskSystem_Create(const UpeTaskSystemDesc *desc);
void UpeTaskSystem_Destroy(UpeTaskSystem *system);
int UpeTaskSystem_Enqueue(UpeTaskSystem *system, UpeTaskFn fn, void *user);
void UpeTaskSystem_Run(UpeTaskSystem *system);
uint32_t UpeTaskSystem_Pending(const UpeTaskSystem *system);

#ifdef __cplusplus
}
#endif
