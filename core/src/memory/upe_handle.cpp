#include "upe/upe_handle.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/*==============================================================================
  HANDLE TABLE IMPLEMENTATION
==============================================================================*/

UpeHandleTable* UpeHandleTable_Create(uint32_t capacity) {
    UpeHandleTable *table = (UpeHandleTable*)malloc(sizeof(UpeHandleTable));
    if (!table) return NULL;
    
    table->generations = (uint32_t*)malloc(sizeof(uint32_t) * capacity);
    table->objects = (void**)malloc(sizeof(void*) * capacity);
    
    if (!table->generations || !table->objects) {
        free(table->generations);
        free(table->objects);
        free(table);
        return NULL;
    }
    
    table->capacity = capacity;
    table->count = 0;
    table->first_free = 0xFFFFFFFF;
    
    /* Initialize generations to 1 (0 is invalid) */
    for (uint32_t i = 0; i < capacity; i++) {
        table->generations[i] = 1;
        table->objects[i] = NULL;
    }
    
    return table;
}

void UpeHandleTable_Destroy(UpeHandleTable *table) {
    if (!table) return;
    free(table->generations);
    free(table->objects);
    free(table);
}

UpeHandle UpeHandleTable_Alloc(UpeHandleTable *table) {
    assert(table);
    
    UpeHandle handle = UPE_HANDLE_INVALID;
    
    if (table->first_free != 0xFFFFFFFF) {
        /* Reuse from free list */
        handle.index = table->first_free;
        
        /* Next free is stored in the generation slot (bit hack) */
        uint32_t next_free = *(uint32_t*)&table->objects[handle.index];
        table->first_free = next_free;
        table->objects[handle.index] = NULL;
    } else if (table->count < table->capacity) {
        /* Allocate from end */
        handle.index = table->count;
        table->count++;
    } else {
        /* Table is full */
        fprintf(stderr, "Handle table exhausted! Capacity %u reached.\n", table->capacity);
        return UPE_HANDLE_INVALID;
    }
    
    handle.generation = table->generations[handle.index];
    return handle;
}

void UpeHandleTable_Free(UpeHandleTable *table, UpeHandle handle) {
    assert(table);
    
    if (handle.index >= table->capacity) {
        fprintf(stderr, "Invalid handle index %u\n", handle.index);
        return;
    }
    
    if (table->generations[handle.index] != handle.generation) {
        fprintf(stderr, "Double-free or stale handle detected!\n");
        return;
    }
    
    /* Increment generation for next allocation of this slot */
    table->generations[handle.index]++;
    if (table->generations[handle.index] == 0) {
        table->generations[handle.index] = 1;
    }
    
    /* Add to free list */
    uint32_t next_free = table->first_free;
    *(uint32_t*)&table->objects[handle.index] = next_free;
    table->first_free = handle.index;
    table->objects[handle.index] = NULL;
}

void* UpeHandleTable_Get(UpeHandleTable *table, UpeHandle handle) {
    assert(table);
    
    if (handle.index >= table->capacity) {
        return NULL;
    }
    
    if (table->generations[handle.index] != handle.generation) {
        /* Handle is invalid (freed or never allocated) */
        return NULL;
    }
    
    return table->objects[handle.index];
}

UpeHandle UpeHandleTable_Set(UpeHandleTable *table, UpeHandle handle, void *object) {
    assert(table);
    
    if (handle.index == 0xFFFFFFFF) {
        /* Allocate new handle */
        handle = UpeHandleTable_Alloc(table);
    } else {
        /* Verify existing handle is valid */
        if (handle.index >= table->capacity || table->generations[handle.index] != handle.generation) {
            fprintf(stderr, "Attempted set on invalid handle\n");
            return UPE_HANDLE_INVALID;
        }
    }
    
    if (handle.index != 0xFFFFFFFF) {
        table->objects[handle.index] = object;
    }
    
    return handle;
}

int UpeHandleTable_IsValid(UpeHandleTable *table, UpeHandle handle) {
    assert(table);
    
    if (handle.index >= table->capacity) {
        return 0;
    }
    
    return table->generations[handle.index] == handle.generation ? 1 : 0;
}

void UpeHandleTable_GetStats(UpeHandleTable *table, UpeHandleTableStats *out_stats) {
    assert(table && out_stats);
    
    out_stats->capacity = table->capacity;
    out_stats->count = table->count;
    
    /* Count free slots */
    out_stats->free_count = 0;
    uint32_t free_idx = table->first_free;
    while (free_idx != 0xFFFFFFFF && out_stats->free_count < table->capacity) {
        out_stats->free_count++;
        free_idx = *(uint32_t*)&table->objects[free_idx];
    }
    
    out_stats->utilization_percent = table->count > 0 
        ? (100.0 * (double)table->count / table->capacity) 
        : 0.0;
}

/*==============================================================================
  HANDLE POOL IMPLEMENTATION
==============================================================================*/

UpeHandlePool* UpeHandlePool_Create(size_t object_size, uint32_t capacity) {
    UpeHandlePool *pool = (UpeHandlePool*)malloc(sizeof(UpeHandlePool));
    if (!pool) return NULL;
    
    pool->table = UpeHandleTable_Create(capacity);
    pool->objects = malloc(object_size * capacity);
    pool->object_size = object_size;
    
    if (!pool->table || !pool->objects) {
        if (pool->table) UpeHandleTable_Destroy(pool->table);
        free(pool->objects);
        free(pool);
        return NULL;
    }
    
    memset(pool->objects, 0, object_size * capacity);
    
    return pool;
}

void UpeHandlePool_Destroy(UpeHandlePool *pool) {
    if (!pool) return;
    UpeHandleTable_Destroy(pool->table);
    free(pool->objects);
    free(pool);
}

UpeHandle UpeHandlePool_Alloc(UpeHandlePool *pool) {
    assert(pool);
    
    UpeHandle handle = UpeHandleTable_Alloc(pool->table);
    if (handle.index != 0xFFFFFFFF) {
        void *object_ptr = (uint8_t*)pool->objects + handle.index * pool->object_size;
        memset(object_ptr, 0, pool->object_size);
        UpeHandleTable_Set(pool->table, handle, object_ptr);
    }
    
    return handle;
}

void UpeHandlePool_Free(UpeHandlePool *pool, UpeHandle handle) {
    assert(pool);
    UpeHandleTable_Free(pool->table, handle);
}

void* UpeHandlePool_Get(UpeHandlePool *pool, UpeHandle handle) {
    assert(pool);
    return UpeHandleTable_Get(pool->table, handle);
}

int UpeHandlePool_IsValid(UpeHandlePool *pool, UpeHandle handle) {
    assert(pool);
    return UpeHandleTable_IsValid(pool->table, handle);
}

/*==============================================================================
  ITERATOR IMPLEMENTATION
==============================================================================*/

void UpeHandleTable_GetIterator(UpeHandleTable *table, UpeHandleIterator *out_iter) {
    assert(table && out_iter);
    
    memset(out_iter, 0, sizeof(UpeHandleIterator));
    
    if (table->count == 0) return;
    
    out_iter->handles = (UpeHandle*)malloc(sizeof(UpeHandle) * table->count);
    if (!out_iter->handles) return;
    
    out_iter->capacity = table->count;
    out_iter->count = 0;
    
    /* Collect all valid handles */
    for (uint32_t i = 0; i < table->count; i++) {
        if (table->objects[i] != NULL) {
            UpeHandle handle;
            handle.index = i;
            handle.generation = table->generations[i];
            out_iter->handles[out_iter->count++] = handle;
        }
    }
}

void UpeHandleIterator_Destroy(UpeHandleIterator *iter) {
    if (iter && iter->handles) {
        free(iter->handles);
        iter->handles = NULL;
        iter->count = 0;
        iter->capacity = 0;
    }
}
