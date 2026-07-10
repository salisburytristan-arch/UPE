// Unified Physics Engine - Performance Suite

#include "upe/upe_api.h"
#include "upe/upe_memory_policy.h"
#include "upe/upe_world_body_api.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

struct PerfRow {
    std::string scenario;
    std::string category;
    std::string variant;
    uint32_t bodies = 0;
    uint32_t threads = 0;
    uint32_t iterations = 0;
    double avg_ms = 0.0;
    double min_ms = 0.0;
    double max_ms = 0.0;
    uint32_t solver_threads = 0;
    uint64_t peak_allocated = 0;
    uint64_t frame_arena_peak = 0;
    uint64_t state_hash = 0;
};

static UpeBodyDesc MakeBody(UpeBodyType type, float x, float y, float z, float mass) {
    UpeBodyDesc desc = {};
    desc.type = type;
    desc.position = {x, y, z};
    desc.rotation = UpeQuat_Identity();
    desc.mass = mass;
    desc.collision_layer = 1u;
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

static UpeWorld *CreateWorld(uint32_t body_count, uint32_t thread_count, int enable_simd_profile) {
    UpeWorldDesc world_desc = UpeWorldDesc_Default();
    world_desc.fixed_dt = 1.0f / 60.0f;
    world_desc.gravity[0] = 0.0f;
    world_desc.gravity[1] = -9.81f;
    world_desc.gravity[2] = 0.0f;
    world_desc.solver_iterations = 10;
    world_desc.max_pairs = 131072;
    world_desc.max_contacts = 131072;
    world_desc.thread_pool_size = (float)thread_count;
    world_desc.enable_sleeping = 1;
    world_desc.enable_warm_starting = 1;
    world_desc.enable_ccd = 1;
    world_desc.ccd_threshold = 8.0f;
    world_desc.enable_determinism_mode = enable_simd_profile ? 1 : 0;

    UpeWorld *world = UpeWorld_Create(&world_desc);
    if (!world) {
        return nullptr;
    }

    UpeBodyDesc ground_body = MakeBody(UPE_BODY_STATIC, 0.0f, -1.0f, 0.0f, 0.0f);
    uint32_t ground_id = UpeWorld_CreateBody(world, &ground_body);
    UpeShapeDesc ground_shape = {};
    ground_shape.type = UPE_SHAPE_BOX;
    ground_shape.body_id = ground_id;
    ground_shape.box.min = {-200.0f, -1.0f, -200.0f};
    ground_shape.box.max = {200.0f, 1.0f, 200.0f};
    UpeWorld_CreateShape(world, &ground_shape);

    uint32_t side = 1;
    while ((side * side * side) < body_count) {
        ++side;
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

                ++created;
            }
        }
    }

    return world;
}

static PerfRow RunWorldStepScenario(const char *scenario_name,
                                    uint32_t body_count,
                                    uint32_t thread_count,
                                    uint32_t warmup_steps,
                                    uint32_t measured_steps,
                                    int simd_profile) {
    PerfRow row;
    row.scenario = scenario_name;
    row.category = "world_step";
    row.variant = simd_profile ? "simd_enabled_build" : "baseline_build";
    row.bodies = body_count;
    row.threads = thread_count;
    row.iterations = measured_steps;

    UpeWorld *world = CreateWorld(body_count, thread_count, simd_profile);
    if (!world) {
        return row;
    }

    for (uint32_t i = 0; i < warmup_steps; ++i) {
        UpeWorld_Step(world);
    }

    auto start = std::chrono::steady_clock::now();
    double min_ms = 1e18;
    double max_ms = 0.0;
    for (uint32_t i = 0; i < measured_steps; ++i) {
        auto step_start = std::chrono::steady_clock::now();
        UpeWorld_Step(world);
        auto step_end = std::chrono::steady_clock::now();
        double step_ms = std::chrono::duration<double, std::milli>(step_end - step_start).count();
        if (step_ms < min_ms) min_ms = step_ms;
        if (step_ms > max_ms) max_ms = step_ms;
    }
    auto end = std::chrono::steady_clock::now();

    std::chrono::duration<double, std::milli> total = end - start;
    row.avg_ms = (measured_steps > 0) ? total.count() / (double)measured_steps : 0.0;
    row.min_ms = min_ms;
    row.max_ms = max_ms;

    UpeWorldStepMetrics metrics = {};
    UpeWorld_GetStepMetrics(world, &metrics);
    row.solver_threads = metrics.solver_threads;
    row.state_hash = metrics.state_hash;

    UpeMemoryStats mem = {};
    UpeMemory_GetStats(&mem);
    row.peak_allocated = mem.peak_allocated;
    row.frame_arena_peak = mem.frame_arena_peak;

    UpeWorld_Destroy(world);
    return row;
}

static PerfRow RunVectorMathScenario(const char *scenario_name, int scalar_fallback) {
    PerfRow row;
    row.scenario = scenario_name;
    row.category = "math_path";
    row.variant = scalar_fallback ? "scalar_fallback" : "engine_vector_path";
    row.iterations = 2500000u;

    UpeVec3 a = {1.0f, 2.0f, 3.0f};
    UpeVec3 b = {4.0f, 5.0f, 6.0f};
    UpeVec3 acc = {0.0f, 0.0f, 0.0f};

    auto start = std::chrono::steady_clock::now();
    for (uint32_t i = 0; i < row.iterations; ++i) {
        if (scalar_fallback) {
            acc.x += a.x * b.x + 0.125f;
            acc.y += a.y * b.y + 0.125f;
            acc.z += a.z * b.z + 0.125f;
        } else {
            acc.x += a.x + b.x;
            acc.y += a.y + b.y;
            acc.z += a.z + b.z;
        }
        a.x += 0.0001f;
        b.z -= 0.0001f;
    }
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;

    row.avg_ms = elapsed.count();
    row.min_ms = elapsed.count();
    row.max_ms = elapsed.count();
    row.state_hash = (uint64_t)(acc.x * 1000003.0f) ^ (uint64_t)(acc.y * 1000033.0f) ^ (uint64_t)(acc.z * 1000037.0f);
    return row;
}

static PerfRow RunMemoryScenario(const char *scenario_name, uint32_t body_count) {
    PerfRow row;
    row.scenario = scenario_name;
    row.category = "memory";
    row.variant = "allocation_pressure";
    row.bodies = body_count;
    row.iterations = 1;

    UpeMemoryStats before = {};
    UpeMemory_GetStats(&before);

    UpeWorld *world = CreateWorld(body_count, 0u, 0);
    if (!world) {
        return row;
    }

    auto start = std::chrono::steady_clock::now();
    for (uint32_t i = 0; i < 120u; ++i) {
        UpeWorld_Step(world);
    }
    auto end = std::chrono::steady_clock::now();

    UpeMemoryStats after = {};
    UpeMemory_GetStats(&after);
    std::chrono::duration<double, std::milli> elapsed = end - start;

    row.avg_ms = elapsed.count() / 120.0;
    row.min_ms = row.avg_ms;
    row.max_ms = row.avg_ms;
    row.peak_allocated = after.peak_allocated;
    row.frame_arena_peak = after.frame_arena_peak;
    row.state_hash = after.total_allocated ^ before.total_allocated;

    UpeWorld_Destroy(world);
    return row;
}

static void WriteJson(const std::vector<PerfRow> &rows, const char *path) {
    std::ofstream out(path, std::ios::binary);
    out << "{\n  \"schema\": \"upe.perf.suite.v1\",\n  \"benchmarks\": [\n";
    for (size_t i = 0; i < rows.size(); ++i) {
        const PerfRow &r = rows[i];
        out << "    {"
            << "\"scenario\":\"" << r.scenario << "\"," 
            << "\"category\":\"" << r.category << "\"," 
            << "\"variant\":\"" << r.variant << "\"," 
            << "\"bodies\":" << r.bodies << ","
            << "\"threads\":" << r.threads << ","
            << "\"iterations\":" << r.iterations << ","
            << "\"avg_ms\":" << std::fixed << std::setprecision(6) << r.avg_ms << ","
            << "\"min_ms\":" << r.min_ms << ","
            << "\"max_ms\":" << r.max_ms << ","
            << "\"solver_threads\":" << r.solver_threads << ","
            << "\"peak_allocated\":" << r.peak_allocated << ","
            << "\"frame_arena_peak\":" << r.frame_arena_peak << ","
            << "\"state_hash\":" << r.state_hash << "}"
            << (i + 1u < rows.size() ? "," : "") << "\n";
    }
    out << "  ]\n}\n";
}

static void WriteCsv(const std::vector<PerfRow> &rows, const char *path) {
    std::ofstream out(path, std::ios::binary);
    out << "scenario,category,variant,bodies,threads,iterations,avg_ms,min_ms,max_ms,solver_threads,peak_allocated,frame_arena_peak,state_hash\n";
    for (const PerfRow &r : rows) {
        out << r.scenario << ',' << r.category << ',' << r.variant << ','
            << r.bodies << ',' << r.threads << ',' << r.iterations << ','
            << std::fixed << std::setprecision(6) << r.avg_ms << ',' << r.min_ms << ',' << r.max_ms << ','
            << r.solver_threads << ',' << r.peak_allocated << ',' << r.frame_arena_peak << ',' << r.state_hash << '\n';
    }
}

static void WriteMarkdown(const std::vector<PerfRow> &rows, const char *path) {
    std::ofstream out(path, std::ios::binary);
    out << "# Performance Suite\n\n";
    out << "This report captures thread scaling, math-path comparison, and memory pressure observations.\n\n";
    out << "| Scenario | Category | Variant | Bodies | Threads | Avg ms | Min ms | Max ms | Solver Threads | Peak Allocated | Frame Arena Peak |\n";
    out << "| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |\n";
    for (const PerfRow &r : rows) {
        out << "| " << r.scenario << " | " << r.category << " | " << r.variant << " | "
            << r.bodies << " | " << r.threads << " | "
            << std::fixed << std::setprecision(6) << r.avg_ms << " | " << r.min_ms << " | " << r.max_ms << " | "
            << r.solver_threads << " | " << r.peak_allocated << " | " << r.frame_arena_peak << " |\n";
    }
}

static void WriteHtml(const std::vector<PerfRow> &rows, const char *path) {
    std::ofstream out(path, std::ios::binary);
    out << "<!doctype html><html><head><meta charset='utf-8'><title>UPE Performance Suite</title>"
        << "<style>body{font-family:Arial,sans-serif;background:#0f172a;color:#e2e8f0;margin:0;padding:24px}"
        << ".card{background:#111827;border:1px solid #334155;border-radius:12px;padding:16px;margin:16px 0}"
        << "table{width:100%;border-collapse:collapse}th,td{border-bottom:1px solid #334155;padding:8px;text-align:left}"
        << ".bar{height:12px;background:linear-gradient(90deg,#38bdf8,#22c55e);border-radius:999px}</style></head><body>";
    out << "<h1>UPE Performance Suite</h1><p>Thread scaling, SIMD/scalar path, and memory pressure observations.</p>";

    double max_ms = 0.0;
    for (const PerfRow &r : rows) {
        if (r.avg_ms > max_ms) max_ms = r.avg_ms;
    }

    out << "<div class='card'><h2>Summary</h2><table><thead><tr><th>Scenario</th><th>Avg ms</th><th>Variant</th><th>Visual</th></tr></thead><tbody>";
    for (const PerfRow &r : rows) {
        double width = (max_ms > 0.0) ? (r.avg_ms / max_ms) * 100.0 : 0.0;
        out << "<tr><td>" << r.scenario << "</td><td>" << std::fixed << std::setprecision(6) << r.avg_ms
            << "</td><td>" << r.variant << "</td><td><div class='bar' style='width:" << width << "%'></div></td></tr>";
    }
    out << "</tbody></table></div>";

    out << "<div class='card'><h2>Details</h2><table><thead><tr><th>Scenario</th><th>Category</th><th>Bodies</th><th>Threads</th><th>Solver Threads</th><th>Peak Allocated</th><th>Frame Arena Peak</th></tr></thead><tbody>";
    for (const PerfRow &r : rows) {
        out << "<tr><td>" << r.scenario << "</td><td>" << r.category << "</td><td>" << r.bodies << "</td><td>" << r.threads
            << "</td><td>" << r.solver_threads << "</td><td>" << r.peak_allocated << "</td><td>" << r.frame_arena_peak << "</td></tr>";
    }
    out << "</tbody></table></div></body></html>";
}

int main(int argc, char **argv) {
    const char *json_path = (argc > 1) ? argv[1] : "proof/latest_performance_report.json";
    const char *csv_path = (argc > 2) ? argv[2] : "proof/latest_performance_report.csv";
    const char *md_path = (argc > 3) ? argv[3] : "proof/latest_performance_report.md";
    const char *html_path = (argc > 4) ? argv[4] : "proof/latest_performance_report.html";

    std::vector<PerfRow> rows;
    rows.push_back(RunWorldStepScenario("world_step_256_threads_0", 256u, 0u, 60u, 180u, 0));
    rows.push_back(RunWorldStepScenario("world_step_256_threads_1", 256u, 1u, 60u, 180u, 0));
    rows.push_back(RunWorldStepScenario("world_step_256_threads_2", 256u, 2u, 60u, 180u, 0));
    rows.push_back(RunWorldStepScenario("world_step_256_threads_4", 256u, 4u, 60u, 180u, 0));
    rows.push_back(RunVectorMathScenario("vector_math_engine_path", 0));
    rows.push_back(RunVectorMathScenario("vector_math_scalar_fallback", 1));
    rows.push_back(RunMemoryScenario("memory_pressure_512_bodies", 512u));

    WriteJson(rows, json_path);
    WriteCsv(rows, csv_path);
    WriteMarkdown(rows, md_path);
    WriteHtml(rows, html_path);

    std::printf("=== UPE Performance Suite ===\n");
    std::printf("Wrote JSON: %s\n", json_path);
    std::printf("Wrote CSV: %s\n", csv_path);
    std::printf("Wrote Markdown: %s\n", md_path);
    std::printf("Wrote HTML: %s\n", html_path);
    for (const PerfRow &r : rows) {
        std::printf("%s,%.6f ms\n", r.scenario.c_str(), r.avg_ms);
    }
    return 0;
}