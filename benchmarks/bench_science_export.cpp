// Unified Physics Engine - Science Catalog Export Benchmark

#include "upe/upe_science_export.h"
#include "upe/upe_science_topics.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

struct ExportBenchResult {
    const char *name;
    uint32_t json_bytes;
    uint32_t iterations;
    double avg_us;
    double min_us;
    double max_us;
};

static ExportBenchResult RunExportBenchmark(const char *name,
                                            const UpeScienceExportOptions &options,
                                            uint32_t iterations) {
    ExportBenchResult result{};
    result.name = name;
    result.iterations = iterations;

    const uint32_t required_len = UpeScience_WriteCatalogJsonEx(&options, nullptr, 0);
    result.json_bytes = required_len;

    std::vector<char> buffer(static_cast<size_t>(required_len) + 1u);

    for (int warmup = 0; warmup < 4; ++warmup) {
        UpeScience_WriteCatalogJsonEx(&options, buffer.data(), static_cast<uint32_t>(buffer.size()));
    }

    using Clock = std::chrono::steady_clock;
    result.min_us = 1e18;
    result.max_us = 0.0;

    for (uint32_t i = 0; i < iterations; ++i) {
        const auto start = Clock::now();
        const uint32_t written = UpeScience_WriteCatalogJsonEx(&options,
                                                               buffer.data(),
                                                               static_cast<uint32_t>(buffer.size()));
        const auto end = Clock::now();

        if (written != required_len) {
            std::fprintf(stderr,
                         "ERROR: unstable export size for %s (expected=%u, got=%u)\n",
                         name,
                         required_len,
                         written);
            std::exit(2);
        }

        const double elapsed_us = std::chrono::duration<double, std::micro>(end - start).count();
        if (elapsed_us < result.min_us) result.min_us = elapsed_us;
        if (elapsed_us > result.max_us) result.max_us = elapsed_us;
        result.avg_us += elapsed_us;
    }

    result.avg_us /= static_cast<double>(iterations);
    return result;
}

static void WriteJsonReport(const ExportBenchResult *results,
                            size_t count,
                            const char *path) {
    FILE *f = std::fopen(path, "wb");
    if (!f) {
        std::fprintf(stderr, "ERROR: failed to write benchmark report: %s\n", path);
        std::exit(3);
    }

    std::fprintf(f, "{\n  \"schema\":\"upe.perf.science_export.v1\",\n  \"benchmarks\":[\n");
    for (size_t i = 0; i < count; ++i) {
        const ExportBenchResult &r = results[i];
        std::fprintf(f,
                     "    {\"name\":\"%s\",\"avg_us\":%.3f,\"min_us\":%.3f,\"max_us\":%.3f,\"iterations\":%u,\"json_bytes\":%u}%s\n",
                     r.name,
                     r.avg_us,
                     r.min_us,
                     r.max_us,
                     r.iterations,
                     r.json_bytes,
                     (i + 1u < count) ? "," : "");
    }
    std::fprintf(f, "  ]\n}\n");
    std::fclose(f);
}

int main(int argc, char **argv) {
    const char *report_path = (argc > 1) ? argv[1] : "science_export_bench.json";

    UpeScienceExportOptions compact = UpeScienceExportOptions_Default();
    compact.pretty = 0;
    compact.include_schema = 1;
    compact.include_totals = 1;
    compact.include_topics = 1;
    compact.include_domain_summaries = 1;
    compact.include_filter_metadata = 0;
    compact.include_empty_domains = 1;
    compact.topic_order = (int)UPE_SCIENCE_EXPORT_TOPIC_NAME_ASC;

    UpeScienceExportOptions filtered = UpeScienceExportOptions_Default();
    filtered.pretty = 1;
    filtered.include_filter_metadata = 1;
    filtered.include_domain_summaries = 0;
    filtered.include_only_implemented = 1;
    filtered.filter_level = (int)UPE_SCIENCE_LEVEL_MINIMAL;
    filtered.topic_substring = "solver";
    filtered.topic_substring_case_sensitive = 0;
    filtered.max_topics_per_domain = 32;
    filtered.domain_order = (int)UPE_SCIENCE_EXPORT_DOMAIN_IMPLEMENTED_DESC;
    filtered.topic_order = (int)UPE_SCIENCE_EXPORT_TOPIC_LEVEL_DESC_ID_ASC;

    const ExportBenchResult results[] = {
        RunExportBenchmark("science_export_compact_full", compact, 2500),
        RunExportBenchmark("science_export_filtered_pretty", filtered, 2500),
    };

    std::printf("=== Science Export Benchmark ===\n");
    for (const ExportBenchResult &r : results) {
        std::printf("%s: avg=%.3f us min=%.3f us max=%.3f us bytes=%u iterations=%u\n",
                    r.name,
                    r.avg_us,
                    r.min_us,
                    r.max_us,
                    r.json_bytes,
                    r.iterations);
    }

    WriteJsonReport(results, sizeof(results) / sizeof(results[0]), report_path);
    std::printf("Wrote report: %s\n", report_path);
    return 0;
}
