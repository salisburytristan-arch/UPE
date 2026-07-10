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

// Uncertainty quantification / calibration
typedef struct UpeMonteCarloSummary {
    float mean;
    float stddev;
    float min_value;
    float max_value;
    float p05;
    float p50;
    float p95;
    uint32_t sample_count;
} UpeMonteCarloSummary;

typedef struct UpeCalibrationReport {
    uint32_t sample_count;
    float scale;
    float bias;
    float rmse_before;
    float rmse_after;
    float mean_corrected_error;
    float confidence95_low;
    float confidence95_high;
} UpeCalibrationReport;

typedef float (*UpeParameterObjective1D)(float parameter, void *user_data);

typedef struct UpeParameterSweepSummary {
    float min_parameter;
    float max_parameter;
    float best_parameter;
    float best_score;
    float mean_score;
    float stddev_score;
    float estimated_sensitivity;
    uint32_t sample_count;
} UpeParameterSweepSummary;

typedef struct UpeSensitivityAnalysis1D {
    float center_parameter;
    float delta;
    float value_minus;
    float value_center;
    float value_plus;
    float gradient;
    float curvature;
    int locally_convex;
} UpeSensitivityAnalysis1D;

typedef struct UpeConservationAudit {
    uint32_t sample_count;
    uint32_t worst_sample_index;
    float initial_value;
    float final_value;
    float peak_relative_drift;
    float mean_relative_drift;
    float drift_slope_per_sample;
    float stability_score;
} UpeConservationAudit;

typedef struct UpeSystemIdentificationReport {
    uint32_t sample_count;
    float scale;
    float bias;
    float r_squared;
    float rmse;
    float max_abs_error;
} UpeSystemIdentificationReport;

typedef struct UpeDistributionComparison {
    uint32_t sample_count_a;
    uint32_t sample_count_b;
    float mean_a;
    float mean_b;
    float mean_shift;
    float ks_distance;
    float overlap_score;
} UpeDistributionComparison;

typedef struct UpeThresholdRobustnessReport {
    uint32_t sample_count;
    uint32_t pass_count;
    float pass_rate;
    float min_value;
    float max_value;
    float threshold;
    float worst_margin;
    float mean_margin;
} UpeThresholdRobustnessReport;

typedef struct UpeReliabilityCurvePoint {
    float threshold;
    float pass_rate;
    float mean_margin;
} UpeReliabilityCurvePoint;

typedef struct UpeReliabilityCurveSummary {
    uint32_t point_count;
    float threshold_min;
    float threshold_max;
    float mean_pass_rate;
    float min_pass_rate;
    float max_pass_rate;
    float monotonicity_score;
} UpeReliabilityCurveSummary;

typedef struct UpeUncertaintyCalibrationReport {
    uint32_t sample_count;
    float predicted_mean;
    float observed_mean;
    float bias;
    float nrmse;
    float coverage_ratio;
    float calibration_score;
} UpeUncertaintyCalibrationReport;

typedef struct UpeConfusionMatrix {
    uint32_t true_positive;
    uint32_t false_positive;
    uint32_t true_negative;
    uint32_t false_negative;
    float precision;
    float recall;
    float specificity;
    float f1_score;
    float accuracy;
} UpeConfusionMatrix;

typedef struct UpeRocPoint {
    float threshold;
    float true_positive_rate;
    float false_positive_rate;
    float precision;
} UpeRocPoint;

typedef struct UpeRocCurveSummary {
    uint32_t point_count;
    float auc_estimate;
    float best_f1_threshold;
    float best_f1_score;
} UpeRocCurveSummary;

typedef struct UpePrecisionRecallPoint {
    float threshold;
    float precision;
    float recall;
    float f1_score;
} UpePrecisionRecallPoint;

typedef struct UpePrecisionRecallCurveSummary {
    uint32_t point_count;
    float average_precision_estimate;
    float best_f1_threshold;
    float best_f1_score;
    float max_precision;
    float max_recall;
} UpePrecisionRecallCurveSummary;

typedef struct UpeProbabilityScoreReport {
    uint32_t sample_count;
    float positive_rate;
    float mean_confidence;
    float brier_score;
    float log_loss;
    float expected_calibration_error;
    float balanced_accuracy;
    float matthews_corrcoef;
} UpeProbabilityScoreReport;

typedef struct UpeStepResponseReport {
    uint32_t sample_count;
    float target_value;
    float final_value;
    float steady_state_error;
    float peak_value;
    float overshoot_ratio;
    float rise_time;
    float settling_time;
    float time_to_peak;
    float oscillation_score;
} UpeStepResponseReport;

typedef struct UpeTrackingErrorReport {
    uint32_t sample_count;
    float mean_abs_error;
    float rmse;
    float max_abs_error;
    float integral_abs_error;
    float integral_sq_error;
    float integral_time_abs_error;
    float fit_r_squared;
} UpeTrackingErrorReport;

typedef struct UpeInvariantAuditReport {
    uint32_t sample_count;
    float max_abs_drift;
    float mean_abs_drift;
    float final_abs_drift;
    float monotonic_violation_ratio;
    float invariant_score;
} UpeInvariantAuditReport;

typedef struct UpeStateEstimateReport {
    uint32_t sample_count;
    float mean_measurement_residual;
    float residual_stddev;
    float final_estimate;
    float final_covariance;
    float innovation_consistency;
    float smoothing_gain;
} UpeStateEstimateReport;

typedef struct UpeLinearSystem2AuditReport {
    float trace;
    float determinant;
    float spectral_radius_bound;
    float damping_indicator;
    float controllability_measure;
    float observability_measure;
    int stable_linearized;
    int controllable;
    int observable;
} UpeLinearSystem2AuditReport;

typedef struct UpeScalarMpcReport {
    uint32_t horizon_steps;
    float model_a;
    float model_b;
    float state_cost;
    float control_cost;
    float terminal_weight;
    float feedback_gain;
    float first_control;
    float predicted_final_state;
    float terminal_error;
    float mean_abs_control;
    float predicted_cost;
    int stabilizing;
} UpeScalarMpcReport;

typedef struct UpeAlphaBetaTrackerReport {
    uint32_t sample_count;
    float final_position;
    float final_velocity;
    float mean_abs_residual;
    float residual_stddev;
    float mean_position_gain;
    float mean_velocity_gain;
    float tracking_score;
} UpeAlphaBetaTrackerReport;

typedef struct UpeScalarTrajectoryAuditReport {
    uint32_t sample_count;
    float duration;
    float displacement;
    float path_length;
    float mean_speed;
    float max_speed;
    float max_acceleration;
    float max_jerk;
    float smoothness_score;
    float monotonic_progress_ratio;
} UpeScalarTrajectoryAuditReport;

typedef struct UpeTrapezoidalTrajectoryPlan {
    float start_position;
    float goal_position;
    float displacement;
    float velocity_limit;
    float acceleration_limit;
    float accel_time;
    float cruise_time;
    float decel_time;
    float total_time;
    float peak_velocity;
    float mean_speed;
    float feasibility_score;
    int triangular_profile;
} UpeTrapezoidalTrajectoryPlan;

typedef struct UpeConstantVelocityKalmanReport {
    uint32_t sample_count;
    float final_position;
    float final_velocity;
    float final_position_covariance;
    float final_velocity_covariance;
    float mean_abs_innovation;
    float innovation_stddev;
    float normalized_innovation_score;
} UpeConstantVelocityKalmanReport;

int UpeCert_SummarizeMonteCarlo(const float *samples,
                                uint32_t count,
                                UpeMonteCarloSummary *out_summary);
int UpeCert_RunMeasurementCalibration(const float *reference,
                                      const float *measured,
                                      uint32_t count,
                                      UpeCalibrationReport *out_report);
int UpeCert_RunParameterSweep1D(float min_value,
                                float max_value,
                                uint32_t sample_count,
                                UpeParameterObjective1D objective,
                                void *user_data,
                                UpeParameterSweepSummary *out_summary);
int UpeCert_GenerateLatinHypercube1D(float min_value,
                                     float max_value,
                                     uint32_t sample_count,
                                     uint32_t seed,
                                     float *out_samples);
int UpeCert_RunAffineSystemIdentification(const float *inputs,
                                          const float *outputs,
                                          uint32_t count,
                                          UpeSystemIdentificationReport *out_report);
int UpeCert_CompareSampleDistributions(const float *sample_a,
                                       uint32_t count_a,
                                       const float *sample_b,
                                       uint32_t count_b,
                                       UpeDistributionComparison *out_report);
int UpeCert_AnalyzeThresholdRobustness(const float *values,
                                       uint32_t count,
                                       float threshold,
                                       int require_greater_equal,
                                       UpeThresholdRobustnessReport *out_report);
int UpeCert_BuildReliabilityCurve(const float *values,
                                  uint32_t count,
                                  float threshold_min,
                                  float threshold_max,
                                  uint32_t point_count,
                                  int require_greater_equal,
                                  UpeReliabilityCurvePoint *out_points,
                                  UpeReliabilityCurveSummary *out_summary);
int UpeCert_AnalyzeUncertaintyCalibration(const float *predicted,
                                          const float *observed,
                                          uint32_t count,
                                          float tolerance_band,
                                          UpeUncertaintyCalibrationReport *out_report);
int UpeCert_ComputeConfusionMatrix(const float *scores,
                                   const uint8_t *labels,
                                   uint32_t count,
                                   float threshold,
                                   UpeConfusionMatrix *out_matrix);
int UpeCert_BuildRocCurve(const float *scores,
                          const uint8_t *labels,
                          uint32_t count,
                          float threshold_min,
                          float threshold_max,
                          uint32_t point_count,
                          UpeRocPoint *out_points,
                          UpeRocCurveSummary *out_summary);
int UpeCert_BuildPrecisionRecallCurve(const float *scores,
                                      const uint8_t *labels,
                                      uint32_t count,
                                      float threshold_min,
                                      float threshold_max,
                                      uint32_t point_count,
                                      UpePrecisionRecallPoint *out_points,
                                      UpePrecisionRecallCurveSummary *out_summary);
int UpeCert_AnalyzeProbabilityScores(const float *probabilities,
                                     const uint8_t *labels,
                                     uint32_t count,
                                     float threshold,
                                     uint32_t calibration_bins,
                                     UpeProbabilityScoreReport *out_report);
int UpeCert_AnalyzeStepResponse(const float *times,
                                const float *response,
                                uint32_t count,
                                float target_value,
                                float settling_band,
                                UpeStepResponseReport *out_report);
int UpeCert_AnalyzeTrackingErrorSeries(const float *reference,
                                       const float *observed,
                                       const float *times,
                                       uint32_t count,
                                       UpeTrackingErrorReport *out_report);
int UpeCert_AnalyzeInvariantSeries(const float *values,
                                   uint32_t count,
                                   float expected_value,
                                   int require_monotonic_nonincreasing,
                                   UpeInvariantAuditReport *out_report);
int UpeCert_RunScalarStateEstimator(const float *measurements,
                                    uint32_t count,
                                    float process_variance,
                                    float measurement_variance,
                                    UpeStateEstimateReport *out_report);
int UpeCert_AnalyzeLinearSystem2(const float *a_matrix_row_major,
                                 const float *b_vector,
                                 const float *c_vector,
                                 UpeLinearSystem2AuditReport *out_report);
int UpeCert_PlanScalarMpc(float initial_state,
                          float target_state,
                          float model_a,
                          float model_b,
                          float state_cost,
                          float control_cost,
                          float terminal_weight,
                          uint32_t horizon_steps,
                          UpeScalarMpcReport *out_report);
int UpeCert_RunAlphaBetaTracker(const float *measurements,
                                const float *times,
                                uint32_t count,
                                float alpha,
                                float beta,
                                UpeAlphaBetaTrackerReport *out_report);
int UpeCert_AnalyzeScalarTrajectory(const float *positions,
                                    const float *times,
                                    uint32_t count,
                                    UpeScalarTrajectoryAuditReport *out_report);
int UpeCert_PlanTrapezoidalTrajectory1D(float start_position,
                                        float goal_position,
                                        float velocity_limit,
                                        float acceleration_limit,
                                        UpeTrapezoidalTrajectoryPlan *out_report);
int UpeCert_RunConstantVelocityKalman1D(const float *measurements,
                                        const float *times,
                                        uint32_t count,
                                        float process_accel_variance,
                                        float measurement_variance,
                                        UpeConstantVelocityKalmanReport *out_report);
int UpeCert_RunSensitivityAnalysis1D(float center_parameter,
                                     float delta,
                                     UpeParameterObjective1D objective,
                                     void *user_data,
                                     UpeSensitivityAnalysis1D *out_analysis);
int UpeCert_AnalyzeConservationSeries(const float *values,
                                      uint32_t count,
                                      UpeConservationAudit *out_audit);
uint32_t UpeCert_WriteMonteCarloSummaryJson(const UpeMonteCarloSummary *summary,
                                            char *buffer,
                                            uint32_t buffer_size);
uint32_t UpeCert_WriteCalibrationReportJson(const UpeCalibrationReport *report,
                                            char *buffer,
                                            uint32_t buffer_size);
uint32_t UpeCert_WriteSystemIdentificationJson(const UpeSystemIdentificationReport *report,
                                               char *buffer,
                                               uint32_t buffer_size);
uint32_t UpeCert_WriteDistributionComparisonJson(const UpeDistributionComparison *report,
                                                 char *buffer,
                                                 uint32_t buffer_size);
uint32_t UpeCert_WriteThresholdRobustnessJson(const UpeThresholdRobustnessReport *report,
                                              char *buffer,
                                              uint32_t buffer_size);
uint32_t UpeCert_WriteReliabilityCurveJson(const UpeReliabilityCurveSummary *summary,
                                           const UpeReliabilityCurvePoint *points,
                                           char *buffer,
                                           uint32_t buffer_size);
uint32_t UpeCert_WriteUncertaintyCalibrationJson(const UpeUncertaintyCalibrationReport *report,
                                                 char *buffer,
                                                 uint32_t buffer_size);
uint32_t UpeCert_WriteConfusionMatrixJson(const UpeConfusionMatrix *matrix,
                                          char *buffer,
                                          uint32_t buffer_size);
uint32_t UpeCert_WriteRocCurveJson(const UpeRocCurveSummary *summary,
                                   const UpeRocPoint *points,
                                   char *buffer,
                                   uint32_t buffer_size);
uint32_t UpeCert_WritePrecisionRecallCurveJson(const UpePrecisionRecallCurveSummary *summary,
                                               const UpePrecisionRecallPoint *points,
                                               char *buffer,
                                               uint32_t buffer_size);
uint32_t UpeCert_WriteProbabilityScoreJson(const UpeProbabilityScoreReport *report,
                                           char *buffer,
                                           uint32_t buffer_size);
uint32_t UpeCert_WriteStepResponseJson(const UpeStepResponseReport *report,
                                       char *buffer,
                                       uint32_t buffer_size);
uint32_t UpeCert_WriteTrackingErrorJson(const UpeTrackingErrorReport *report,
                                        char *buffer,
                                        uint32_t buffer_size);
uint32_t UpeCert_WriteInvariantAuditJson(const UpeInvariantAuditReport *report,
                                         char *buffer,
                                         uint32_t buffer_size);
uint32_t UpeCert_WriteStateEstimateJson(const UpeStateEstimateReport *report,
                                        char *buffer,
                                        uint32_t buffer_size);
uint32_t UpeCert_WriteLinearSystemAuditJson(const UpeLinearSystem2AuditReport *report,
                                            char *buffer,
                                            uint32_t buffer_size);
uint32_t UpeCert_WriteScalarMpcJson(const UpeScalarMpcReport *report,
                                    char *buffer,
                                    uint32_t buffer_size);
uint32_t UpeCert_WriteAlphaBetaTrackerJson(const UpeAlphaBetaTrackerReport *report,
                                           char *buffer,
                                           uint32_t buffer_size);
uint32_t UpeCert_WriteScalarTrajectoryAuditJson(const UpeScalarTrajectoryAuditReport *report,
                                                char *buffer,
                                                uint32_t buffer_size);
uint32_t UpeCert_WriteTrapezoidalTrajectoryJson(const UpeTrapezoidalTrajectoryPlan *report,
                                                char *buffer,
                                                uint32_t buffer_size);
uint32_t UpeCert_WriteConstantVelocityKalmanJson(const UpeConstantVelocityKalmanReport *report,
                                                 char *buffer,
                                                 uint32_t buffer_size);
uint32_t UpeCert_WriteParameterSweepJson(const UpeParameterSweepSummary *summary,
                                         char *buffer,
                                         uint32_t buffer_size);
uint32_t UpeCert_WriteConservationAuditJson(const UpeConservationAudit *audit,
                                            char *buffer,
                                            uint32_t buffer_size);
uint32_t UpeCert_WriteValidationAuditJson(const void *world,
                                          char *buffer,
                                          uint32_t buffer_size);

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
