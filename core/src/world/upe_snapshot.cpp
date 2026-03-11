#include "upe/upe_snapshot.h"
#include <stdlib.h>
#include <string.h>

UpeSnapshot *UpeSnapshot_Create(const void *world_state, uint32_t size, uint32_t frame) {
    if (!world_state || size == 0) {
        return nullptr;
    }

    UpeSnapshot *snapshot = (UpeSnapshot *)malloc(sizeof(UpeSnapshot));
    if (!snapshot) {
        return nullptr;
    }

    snapshot->data = malloc(size);
    if (!snapshot->data) {
        free(snapshot);
        return nullptr;
    }

    memcpy(snapshot->data, world_state, size);
    snapshot->size = size;
    snapshot->frame = frame;
    return snapshot;
}

void UpeSnapshot_Destroy(UpeSnapshot *snapshot) {
    if (!snapshot) {
        return;
    }
    free(snapshot->data);
    free(snapshot);
}

int UpeSnapshot_Restore(UpeSnapshot *snapshot, void *world_state, uint32_t max_size) {
    if (!snapshot || !world_state || snapshot->size > max_size) {
        return 0;
    }
    memcpy(world_state, snapshot->data, snapshot->size);
    return 1;
}
