#include "upe/upe_api.h"
#include "upe/upe_world_body_api.h"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <vector>

struct ScenarioResult {
    uint32_t body_count;
    uint32_t step_count;
    double total_ms;
    double avg_ms;
    double steps_per_sec;
    UpeWorldStepMetrics metrics;
};

static UpeBodyDesc MakeBody(UpeBodyType type, float x, float y, float z, float mass) {
    UpeBodyDesc desc = {};
    desc.type = type;
    desc.position = {x, y, z};
    desc.rotation = UpeQuat_Identity();
    desc.mass = mass;
    desc.collision_layer = 1;
    desc.collision_mask = 0xFFFFFFFFu;
    return desc;
}

static UpeShapeDesc MakeSphereShape(uint32_t body_id, float radius) {
    UpeShapeDesc shape = {};
    shape.type = UPE_SHAPE_SPHERE;
    shape.body_id = body_id;
    shape.friction = 0.6f;
    shape.restitution = 0.1f;
    shape.sphere.center = {0.0f, 0.0f, 0.0f};
    shape.sphere.radius = radius;
    return shape;
}

static UpeShapeDesc MakeGroundBox(uint32_t body_id, float hx, float hy, float hz) {
    UpeShapeDesc shape = {};
    shape.type = UPE_SHAPE_BOX;
    shape.body_id = body_id;
    shape.friction = 0.9f;
    shape.restitution = 0.0f;
    shape.box.min = {-hx, -hy, -hz};
    shape.box.max = {hx, hy, hz};
    return shape;
}

static UpeWorld *CreateBenchmarkWorld(uint32_t body_count, uint32_t thread_count, int enable_sleeping) {
    UpeWorldDesc world_desc = {};
    world_desc.fixed_dt = 1.0f / 60.0f;
    world_desc.gravity[0] = 0.0f;
    world_desc.gravity[1] = -9.81f;
    world_desc.gravity[2] = 0.0f;
    world_desc.solver_iterations = 10;
    world_desc.max_pairs = 131072;
    world_desc.max_contacts = 131072;
    world_desc.thread_pool_size = (float)thread_count;
    world_desc.enable_sleeping = enable_sleeping;
    world_desc.enable_warm_starting = 1;
    world_desc.enable_ccd = 1;
    world_desc.ccd_threshold = 8.0f;

    UpeWorld *world = UpeWorld_Create(&world_desc);
    if (!world) {
        return nullptr;
    }

    UpeBodyDesc ground_body = MakeBody(UPE_BODY_STATIC, 0.0f, -1.0f, 0.0f, 0.0f);
    uint32_t ground_id = UpeWorld_CreateBody(world, &ground_body);
    UpeShapeDesc ground_shape = MakeGroundBox(ground_id, 200.0f, 1.0f, 200.0f);
    UpeWorld_CreateShape(world, &ground_shape);

    uint32_t side = 1;
    while ((side * side * side) < body_count) {
        side++;
    }

    uint32_t created = 0;
    for (uint32_t y = 0; y < side && created < body_count; ++y) {
        for (uint32_t x = 0; x < side && created < body_count; ++x) {
            for (uint32_t z = 0; z < side && created < body_count; ++z) {
                float px = ((float)x - (float)side * 0.5f) * 0.9f;
                float py = 0.5f + (float)y * 0.9f;
                float pz = ((float)z - (float)side * 0.5f) * 0.9f;

                UpeBodyDesc body = MakeBody(UPE_BODY_DYNAMIC, px, py, pz, 1.0f);
                uint32_t body_id = UpeWorld_CreateBody(world, &body);
                UpeShapeDesc shape = MakeSphereShape(body_id, 0.35f);
                UpeWorld_CreateShape(world, &shape);

                UpeBody *body_ptr = UpeWorld_GetBody(world, body_id);
                if (body_ptr) {
                    body_ptr->velocity.x = ((x % 3u) == 0u) ? 0.5f : 0.0f;
                    body_ptr->velocity.z = ((z % 3u) == 0u) ? -0.5f : 0.0f;
                }

                created++;
            }
        }
    }

    return world;
}

static ScenarioResult RunScenario(uint32_t body_count,
                                  uint32_t warmup_steps,
                                  uint32_t measured_steps,
                                  uint32_t thread_count,
                                  int enable_sleeping) {
    ScenarioResult result = {};
    result.body_count = body_count;
    result.step_count = measured_steps;

    UpeWorld *world = CreateBenchmarkWorld(body_count, thread_count, enable_sleeping);
    if (!world) {
        return result;
    }

    for (uint32_t i = 0; i < warmup_steps; ++i) {
        UpeWorld_Step(world);
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (uint32_t i = 0; i < measured_steps; ++i) {
        UpeWorld_Step(world);
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> elapsed = end - start;
    result.total_ms = elapsed.count();
    result.avg_ms = (measured_steps > 0) ? (result.total_ms / (double)measured_steps) : 0.0;
    result.steps_per_sec = (result.avg_ms > 0.0) ? (1000.0 / result.avg_ms) : 0.0;

    UpeWorld_GetStepMetrics(world, &result.metrics);

    UpeWorld_Destroy(world);
    return result;
}

static void PrintResultHeader() {
    std::printf("bodyCount,stepCount,totalMs,avgMs,stepsPerSec,activeBodies,pairs,contacts,constraints,islands,solverIters,solverThreads,solverIslandTasks,solverParallelIslands,solverSerialIslands,ccdCandidates,ccdTested,ccdHits,frameHash\n");
}

static void PrintResultCsv(const ScenarioResult &r) {
    std::printf("%u,%u,%.3f,%.6f,%.3f,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,0x%llx\n",
                r.body_count,
                r.step_count,
                r.total_ms,
                r.avg_ms,
                r.steps_per_sec,
                r.metrics.active_dynamic_bodies,
                r.metrics.broadphase_pairs,
                r.metrics.contacts,
                r.metrics.constraints,
                r.metrics.islands,
                r.metrics.solver_iterations,
                r.metrics.solver_threads,
                r.metrics.solver_island_tasks,
                r.metrics.solver_parallel_islands,
                r.metrics.solver_serial_islands,
                r.metrics.ccd_candidate_pairs,
                r.metrics.ccd_tested_pairs,
                r.metrics.ccd_contact_hits,
                (unsigned long long)r.metrics.state_hash);
}

static void PrintUsage(const char *exe_name) {
    const char *name = (exe_name && exe_name[0] != '\0') ? exe_name : "upe_bench_world_step_v2";
    std::printf("Usage: %s [options]\n", name);
    std::printf("  --steps N        Measured simulation steps (default: 600)\n");
    std::printf("  --warmup N       Warmup steps before timing (default: 120)\n");
    std::printf("  --bodies N       Run a single scenario with N bodies\n");
    std::printf("  --threads N      Requested thread pool size (default: 0 = disabled)\n");
    std::printf("  --no-sleep       Disable sleeping for stress scenarios\n");
    std::printf("  --help           Show this help\n");
}

int main(int argc, char **argv) {
    UpeCapabilities caps = UpeEngine_GetCapabilities();
    std::printf("=== UPE World Step Benchmark v2 ===\n");
    std::printf("Engine version: %u.%u.%u\n",
                (unsigned)caps.version.major,
                (unsigned)caps.version.minor,
                (unsigned)caps.version.patch);

    uint32_t warmup_steps = 120;
    uint32_t measured_steps = 600;
    uint32_t thread_count = 0;
    int enable_sleeping = 1;

    std::vector<uint32_t> scenarios;
    scenarios.push_back(128);
    scenarios.push_back(256);
    scenarios.push_back(512);
    scenarios.push_back(1024);

    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        if (!arg) {
            continue;
        }

        if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0) {
            PrintUsage(argv[0]);
            return 0;
        }

        if (std::strcmp(arg, "--steps") == 0 && (i + 1) < argc) {
            measured_steps = (uint32_t)std::strtoul(argv[++i], nullptr, 10);
        } else if (std::strcmp(arg, "--warmup") == 0 && (i + 1) < argc) {
            warmup_steps = (uint32_t)std::strtoul(argv[++i], nullptr, 10);
        } else if (std::strcmp(arg, "--threads") == 0 && (i + 1) < argc) {
            thread_count = (uint32_t)std::strtoul(argv[++i], nullptr, 10);
        } else if (std::strcmp(arg, "--bodies") == 0 && (i + 1) < argc) {
            uint32_t n = (uint32_t)std::strtoul(argv[++i], nullptr, 10);
            if (n > 0) {
                scenarios.clear();
                scenarios.push_back(n);
            }
        } else if (std::strcmp(arg, "--no-sleep") == 0) {
            enable_sleeping = 0;
        }
    }

    if (measured_steps == 0) {
        measured_steps = 1;
    }
    if (warmup_steps > 100000u) {
        warmup_steps = 100000u;
    }
    if (thread_count > 64u) {
        thread_count = 64u;
    }

    std::printf("Config: warmup=%u measured=%u threads=%u sleeping=%d\n",
                warmup_steps,
                measured_steps,
                thread_count,
                enable_sleeping);

    std::vector<ScenarioResult> results;
    results.reserve(scenarios.size());

    for (uint32_t n : scenarios) {
        std::printf("Running scenario with %u bodies...\n", n);
        ScenarioResult r = RunScenario(n, warmup_steps, measured_steps, thread_count, enable_sleeping);
        results.push_back(r);
    }

    std::printf("\n=== CSV Output ===\n");
    PrintResultHeader();
    for (const ScenarioResult &r : results) {
        PrintResultCsv(r);
    }

    std::printf("\nBenchmark complete.\n");
    return 0;
}
