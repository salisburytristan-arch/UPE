#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
  Certification Harness (50.0.0.0)
  
  Repro packs, golden results, cross-compiler hash matrix, long-run drift tests.
  This is how you prove "greatest engine" instead of claiming it.
*/

// Test result status
typedef enum UpeTestStatus {
    UPE_TEST_PASS = 0,
    UPE_TEST_FAIL = 1,
    UPE_TEST_SKIP = 2,
    UPE_TEST_TIMEOUT = 3
} UpeTestStatus;

// Test categories
typedef enum UpeTestCategory {
    UPE_TEST_CORRECTNESS = 0,    // Does it produce correct physics?
    UPE_TEST_DETERMINISM = 1,    // Does it produce identical results?
    UPE_TEST_STABILITY = 2,      // Does it remain stable long-term?
    UPE_TEST_PERFORMANCE = 3,    // Does it meet performance budgets?
    UPE_TEST_REGRESSION = 4      // Does it match previous results?
} UpeTestCategory;

// Test configuration
typedef struct UpeTestConfig {
    char name[128];
    UpeTestCategory category;
    
    uint32_t step_count;         // How many simulation steps
    float time_step;             // dt per step
    uint32_t max_duration_ms;    // Timeout
    
    // Determinism
    uint32_t seed;               // PRNG seed
    int cross_compiler_test;     // Compare across compilers
    
    // Correctness
    float tolerance;             // Allowed error
    const char *golden_file;     // Path to golden results
    
} UpeTestConfig;

// Test result
typedef struct UpeTestResult {
    char test_name[128];
    UpeTestStatus status;
    
    // Timing
    uint64_t duration_ms;
    float avg_step_time_ms;
    
    // Determinism
    uint64_t final_state_hash;
    uint64_t frame_hashes[1000]; // Per-frame hashes (for drift detection)
    uint32_t hash_count;
    
    // Energy tracking
    float initial_energy;
    float final_energy;
    float max_energy_error;      // Peak deviation
    
    // Correctness
    float max_position_error;
    float max_velocity_error;
    float rms_error;
    
    // Failure info
    char failure_message[256];
    uint32_t failure_frame;
    
} UpeTestResult;

// Test suite
typedef struct UpeTestSuite {
    char name[64];
    
    UpeTestConfig *tests;
    uint32_t test_count;
    
    UpeTestResult *results;
    uint32_t result_count;
    
    // Summary
    uint32_t passed;
    uint32_t failed;
    uint32_t skipped;
    
} UpeTestSuite;

// === TORTURE TESTS ===

// 10K stack stability test
UpeTestConfig UpeCert_Test10KStack();

// Bullet-through-paper (no tunneling)
UpeTestConfig UpeCert_TestBulletThroughPaper();

// Ragdoll pile (100 ragdolls in heap)
UpeTestConfig UpeCert_TestRagdollPile();

// High angular spin (gyroscopic stability)
UpeTestConfig UpeCert_TestHighAngularSpin();

// Newton's cradle (energy conservation)
UpeTestConfig UpeCert_TestNewtonsCradle();

// Friction ramp (static/kinetic transition)
UpeTestConfig UpeCert_TestFrictionRamp();

// Restitution bounce (correct bounce heights)
UpeTestConfig UpeCert_TestRestitutionBounce();

// === LONG-RUN DRIFT TESTS ===

// 1M steps, track energy drift
UpeTestConfig UpeCert_TestLongRunDrift();

// Orbital simulation (N-body stability)
UpeTestConfig UpeCert_TestOrbitalStability();

// Pendulum (periodic motion check)
UpeTestConfig UpeCert_TestPendulum();

// === DETERMINISM TESTS ===

// Same input → same output (single run)
UpeTestConfig UpeCert_TestDeterminismSingle();

// Cross-compiler (MSVC vs GCC vs Clang)
UpeTestConfig UpeCert_TestCrossCompiler();

// Replay verification (save/load state)
UpeTestConfig UpeCert_TestReplay();

// === PERFORMANCE TESTS ===

// 10K rigid bodies (throughput)
UpeTestConfig UpeCert_Test10KBodies();

// Fluid splash (particle performance)
UpeTestConfig UpeCert_TestFluidSplash();

// Cloth simulation (constraint solver perf)
UpeTestConfig UpeCert_TestClothPerf();

// === REGRESSION TESTS ===

// Compare against golden results
UpeTestConfig UpeCert_TestRegression(const char *golden_file);

// Initialization
void UpeCert_InitSuite(UpeTestSuite *suite, const char *name);
void UpeCert_DestroySuite(UpeTestSuite *suite);

// Test execution
void UpeCert_AddTest(UpeTestSuite *suite, const UpeTestConfig *config);
void UpeCert_RunAll(UpeTestSuite *suite);
void UpeCert_RunTest(UpeTestSuite *suite, uint32_t test_index);

// Result analysis
void UpeCert_PrintSummary(const UpeTestSuite *suite);
void UpeCert_PrintDetailed(const UpeTestSuite *suite);
void UpeCert_ExportResults(const UpeTestSuite *suite, const char *filename);

// Golden result management
void UpeCert_SaveGolden(const UpeTestResult *result, const char *filename);
int UpeCert_CompareGolden(const UpeTestResult *result, const char *filename);

// Energy tracking
float UpeCert_ComputeTotalEnergy(const void *world);
float UpeCert_ComputeKineticEnergy(const void *world);
float UpeCert_ComputePotentialEnergy(const void *world);

// State hashing
uint64_t UpeCert_HashWorldState(const void *world);
void UpeCert_HashPerFrame(void *world, uint64_t *hashes, uint32_t max_frames);

// Repro pack (binary snapshot for bug reproduction)
typedef struct UpeReproPack {
    char description[256];
    uint32_t platform_id;        // Windows, Linux, macOS
    uint32_t compiler_id;        // MSVC, GCC, Clang
    uint32_t engine_version;
    
    uint64_t snapshot_hash;
    void *world_state;           // Serialized world
    uint32_t state_size;
    
    // Input sequence (for replay)
    void *inputs;
    uint32_t input_count;
    
} UpeReproPack;

void UpeCert_CreateReproPack(UpeReproPack *pack, const void *world, const char *description);
void UpeCert_LoadReproPack(UpeReproPack *pack, const char *filename);
void UpeCert_SaveReproPack(const UpeReproPack *pack, const char *filename);

// CI Integration
int UpeCert_RunCI(const char *suite_name);  // Returns exit code (0 = all pass)

#ifdef __cplusplus
}
#endif
