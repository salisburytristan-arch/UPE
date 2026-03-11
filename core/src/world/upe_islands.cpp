#include "upe/upe_islands.h"

void UpeIsland_Init(UpeIsland *island) {
    if (!island) {
        return;
    }
    island->bodies = nullptr;
    island->body_count = 0;
    island->awake = 1;
    island->sleep_time = 0.0f;
    island->kinetic_energy = 0.0f;
}

void UpeIsland_Destroy(UpeIsland *island) {
    if (!island) {
        return;
    }
    delete[] island->bodies;
    island->bodies = nullptr;
    island->body_count = 0;
}

void UpeIsland_AddBody(UpeIsland *island, uint32_t body_id) {
    if (!island) {
        return;
    }
    uint32_t *new_bodies = new uint32_t[island->body_count + 1];
    for (uint32_t i = 0; i < island->body_count; ++i) {
        new_bodies[i] = island->bodies[i];
    }
    new_bodies[island->body_count] = body_id;
    delete[] island->bodies;
    island->bodies = new_bodies;
    island->body_count++;
}

void UpeIsland_Sleep(UpeIsland *island) {
    if (!island) {
        return;
    }
    island->awake = 0;
    island->sleep_time = 0.0f;
}

void UpeIsland_Wake(UpeIsland *island) {
    if (!island) {
        return;
    }
    island->awake = 1;
    island->sleep_time = 0.0f;
}
