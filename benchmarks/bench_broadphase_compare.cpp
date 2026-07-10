// Unified Physics Engine - Broadphase Comparison Benchmark

#include "upe/upe_broadphase.h"
#include "upe/upe_bvh.h"
#include "upe/upe_geometry.h"

#ifdef _WIN32
#include <windows.h>
static double NowMs() {
    LARGE_INTEGER freq;
    LARGE_INTEGER now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    return (double)now.QuadPart * 1000.0 / (double)freq.QuadPart;
}
#else
#include <time.h>
static double NowMs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}
#endif

#include <stdio.h>
#include <stdlib.h>

static void FillScene(UpeAabb3 *bounds, uint32_t count, float jitter) {
    for (uint32_t i = 0; i < count; ++i) {
        float base = (float)(i % 32) * 0.45f;
        float layer = (float)(i / 32) * 0.35f;
        bounds[i].min.x = base;
        bounds[i].min.y = layer;
        bounds[i].min.z = (float)(i % 7) * 0.31f;
        bounds[i].max.x = base + 0.4f + jitter * (float)(i & 1);
        bounds[i].max.y = layer + 0.4f;
        bounds[i].max.z = (float)(i % 7) * 0.31f + 0.4f;
    }
}

static void RunScenario(const char *name, uint32_t count, uint32_t iterations, float jitter) {
    UpeAabb3 *bounds = (UpeAabb3*)malloc(sizeof(UpeAabb3) * count);
    UpeBroadphasePair *pairs = (UpeBroadphasePair*)malloc(sizeof(UpeBroadphasePair) * (count * 8u));
    uint8_t *scratch = (uint8_t*)malloc(count * sizeof(uint32_t) + count * 2u * sizeof(UpeBvhNode) + count * sizeof(uint32_t));
    if (!bounds || !pairs || !scratch) {
        free(bounds);
        free(pairs);
        free(scratch);
        printf("%s: allocation failed\n", name);
        return;
    }

    FillScene(bounds, count, jitter);

    double sap_total = 0.0;
    double bvh_total = 0.0;
    uint32_t sap_pairs = 0;
    uint32_t bvh_pairs = 0;

    for (uint32_t i = 0; i < iterations; ++i) {
        double start = NowMs();
        sap_pairs = UpeBroadphase_SapPairs(bounds, count, pairs, count * 8u);
        sap_total += NowMs() - start;

        start = NowMs();
        bvh_pairs = UpeBroadphase_BvhPairs(bounds, count, pairs, count * 8u, scratch,
                                           count * sizeof(uint32_t) + count * 2u * sizeof(UpeBvhNode) + count * sizeof(uint32_t));
        bvh_total += NowMs() - start;
    }

    printf("%s,%u,%u,%.6f,%.6f,%u,%u\n",
           name,
           count,
           iterations,
           sap_total / (double)iterations,
           bvh_total / (double)iterations,
           sap_pairs,
           bvh_pairs);

    free(bounds);
    free(pairs);
    free(scratch);
}

int main(void) {
    printf("=== UPE Broadphase Comparison Benchmark ===\n");
    printf("scenario,count,iterations,sapAvgMs,bvhAvgMs,sapPairs,bvhPairs\n");
    RunScenario("collision_cluster_512", 512u, 20u, 0.03f);
    RunScenario("collision_cluster_2048", 2048u, 8u, 0.01f);
    return 0;
}