// Unified Physics Engine - Performance Benchmarking Harness
// Zero-dependency benchmark framework with deterministic timing

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
typedef LARGE_INTEGER BenchTimer;
static inline void BenchTimer_Start(BenchTimer* t) { QueryPerformanceCounter(t); }
static inline double BenchTimer_ElapsedMs(BenchTimer* start) {
    LARGE_INTEGER end, freq;
    QueryPerformanceCounter(&end);
    QueryPerformanceFrequency(&freq);
    return (double)(end.QuadPart - start->QuadPart) * 1000.0 / (double)freq.QuadPart;
}
#else
#include <time.h>
typedef struct timespec BenchTimer;
static inline void BenchTimer_Start(BenchTimer* t) { clock_gettime(CLOCK_MONOTONIC, t); }
static inline double BenchTimer_ElapsedMs(BenchTimer* start) {
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    return (double)(end.tv_sec - start->tv_sec) * 1000.0 + 
           (double)(end.tv_nsec - start->tv_nsec) / 1000000.0;
}
#endif

// Benchmark structure
typedef struct {
    const char* name;
    void (*func)(void);
    double avg_ms;
    double min_ms;
    double max_ms;
    int iterations;
} Benchmark;

#define MAX_BENCHMARKS 128
static Benchmark g_benchmarks[MAX_BENCHMARKS];
static int g_benchmark_count = 0;

// Register benchmark
void RegisterBenchmark(const char* name, void (*func)(void)) {
    if (g_benchmark_count >= MAX_BENCHMARKS) {
        fprintf(stderr, "ERROR: Too many benchmarks registered\n");
        exit(1);
    }
    g_benchmarks[g_benchmark_count].name = name;
    g_benchmarks[g_benchmark_count].func = func;
    g_benchmarks[g_benchmark_count].iterations = 100; // default
    g_benchmark_count++;
}

// Macro for benchmark registration
#define BENCHMARK(name, iterations) \
    static void Bench_##name(void); \
    static void __attribute__((constructor)) Register_##name(void) { \
        RegisterBenchmark(#name, Bench_##name); \
        g_benchmarks[g_benchmark_count-1].iterations = iterations; \
    } \
    static void Bench_##name(void)

#ifdef _MSC_VER
#undef BENCHMARK
#define BENCHMARK(name, iterations) \
    static void Bench_##name(void); \
    struct Register_##name { Register_##name() { \
        RegisterBenchmark(#name, Bench_##name); \
        g_benchmarks[g_benchmark_count-1].iterations = iterations; \
    } }; \
    static Register_##name g_register_##name; \
    static void Bench_##name(void)
#endif

// Run single benchmark
void RunBenchmark(Benchmark* bench) {
    printf("Running: %s (%d iterations)...\n", bench->name, bench->iterations);
    
    double total_ms = 0.0;
    bench->min_ms = 1e9;
    bench->max_ms = 0.0;
    
    // Warmup
    for (int i = 0; i < 3; i++) {
        bench->func();
    }
    
    // Timed runs
    for (int i = 0; i < bench->iterations; i++) {
        BenchTimer timer;
        BenchTimer_Start(&timer);
        bench->func();
        double elapsed = BenchTimer_ElapsedMs(&timer);
        
        total_ms += elapsed;
        if (elapsed < bench->min_ms) bench->min_ms = elapsed;
        if (elapsed > bench->max_ms) bench->max_ms = elapsed;
    }
    
    bench->avg_ms = total_ms / (double)bench->iterations;
}

// Print results
void PrintResults() {
    printf("\n");
    printf("=================================================================\n");
    printf("Benchmark Results\n");
    printf("=================================================================\n");
    printf("%-40s %10s %10s %10s\n", "Name", "Avg (ms)", "Min (ms)", "Max (ms)");
    printf("-----------------------------------------------------------------\n");
    
    for (int i = 0; i < g_benchmark_count; i++) {
        printf("%-40s %10.4f %10.4f %10.4f\n",
               g_benchmarks[i].name,
               g_benchmarks[i].avg_ms,
               g_benchmarks[i].min_ms,
               g_benchmarks[i].max_ms);
    }
    
    printf("=================================================================\n");
}

// Main benchmark runner
int main(int argc, char** argv) {
    printf("UPE Performance Benchmarking Harness\n\n");
    
    const char* filter = (argc > 1) ? argv[1] : NULL;
    
    int run_count = 0;
    for (int i = 0; i < g_benchmark_count; i++) {
        if (filter == NULL || strstr(g_benchmarks[i].name, filter) != NULL) {
            RunBenchmark(&g_benchmarks[i]);
            run_count++;
        }
    }
    
    if (run_count == 0) {
        printf("No benchmarks matched filter: %s\n", filter ? filter : "(none)");
        return 1;
    }
    
    PrintResults();
    return 0;
}
