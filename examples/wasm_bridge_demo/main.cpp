#include "upe/upe_api.h"

#include <cstdio>

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

static UpeWorld *g_world = nullptr;

static void EnsureWorld() {
    if (g_world) {
        return;
    }

    UpeWorldDesc desc = UpeWorldDesc_Default();
    desc.fixed_dt = 1.0f / 60.0f;
    desc.enable_determinism_mode = 1;
    g_world = UpeWorld_Create(&desc);
}

#if defined(__EMSCRIPTEN__)
extern "C" {
EMSCRIPTEN_KEEPALIVE void upe_wasm_tick() {
    EnsureWorld();
    if (g_world) {
        UpeWorld_Step(g_world);
    }
}

EMSCRIPTEN_KEEPALIVE unsigned int upe_wasm_frame() {
    EnsureWorld();
    return g_world ? UpeWorld_GetFrameIndex(g_world) : 0u;
}
}
#endif

int main() {
    std::printf("=== UPE WASM / JS Bridge Demo ===\n");
    std::printf("Compile this target with Emscripten for browser embedding, or run it natively as a smoke test.\n");

    EnsureWorld();
    if (!g_world) {
        std::printf("Failed to create world\n");
        return 1;
    }

    for (int i = 0; i < 10; ++i) {
        UpeWorld_Step(g_world);
    }

    UpeWorldStepMetrics metrics = {};
    UpeWorld_GetStepMetrics(g_world, &metrics);
    std::printf("Frame index: %llu\n", (unsigned long long)UpeWorld_GetFrameIndex(g_world));
    std::printf("State hash: 0x%llX\n", (unsigned long long)metrics.state_hash);

    UpeWorld_Destroy(g_world);
    g_world = nullptr;
    return 0;
}