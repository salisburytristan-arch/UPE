#include "upe/upe_task.h"

struct UpeTask {
    UpeTaskFn fn;
    void *user;
};

struct UpeTaskSystem {
    UpeTask *tasks;
    uint32_t count;
    uint32_t capacity;
    UpeAllocator *allocator;
};

UpeTaskSystem *UpeTaskSystem_Create(const UpeTaskSystemDesc *desc) {
    UpeTaskSystemDesc local_desc;
    if (!desc) {
        local_desc.max_tasks = 256;
        local_desc.allocator = nullptr;
        desc = &local_desc;
    }
    uint32_t capacity = desc->max_tasks ? desc->max_tasks : 256;
    UpeTaskSystem *system = (UpeTaskSystem *)UpeAlloc(desc->allocator, sizeof(UpeTaskSystem), 16);
    if (!system) {
        return nullptr;
    }
    system->tasks = (UpeTask *)UpeAlloc(desc->allocator, sizeof(UpeTask) * capacity, 16);
    if (!system->tasks) {
        UpeFree(desc->allocator, system);
        return nullptr;
    }
    system->count = 0;
    system->capacity = capacity;
    system->allocator = desc->allocator;
    return system;
}

void UpeTaskSystem_Destroy(UpeTaskSystem *system) {
    if (!system) {
        return;
    }
    UpeFree(system->allocator, system->tasks);
    UpeFree(system->allocator, system);
}

int UpeTaskSystem_Enqueue(UpeTaskSystem *system, UpeTaskFn fn, void *user) {
    if (!system || !fn || system->count >= system->capacity) {
        return 0;
    }
    system->tasks[system->count++] = {fn, user};
    return 1;
}

void UpeTaskSystem_Run(UpeTaskSystem *system) {
    if (!system) {
        return;
    }
    for (uint32_t i = 0; i < system->count; ++i) {
        system->tasks[i].fn(system->tasks[i].user);
    }
    system->count = 0;
}

uint32_t UpeTaskSystem_Pending(const UpeTaskSystem *system) {
    return system ? system->count : 0;
}
