#include "gtest/gtest.h"
#include "upe/upe_config.h"
#include "upe/upe_error.h"
#include "upe/upe_telemetry.h"
#include "upe/upe_descriptor.h"
#include "upe/upe_api.h"
#include "upe/upe_shape.h"
#include "upe/upe_world_body_api.h"
#include "upe/upe_body.h"
#include "upe/upe_math_toolbox.h"
#include "upe/upe_certification.h"
#include "upe/upe_warm_start.h"
#include "upe/upe_unified_solver.h"
#include "upe/upe_gpu_compute.h"
#include "upe/upe_pbd.h"
#include "upe/upe_coupling.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

static void ExpGrowthRhs(float t, const float *y, float *dydt, void *user_data) {
    (void)t;
    (void)user_data;
    dydt[0] = y[0];
}

static float QuadraticBowlObjective(const float *x, float *gradient, uint32_t n, void *user_data) {
    (void)user_data;
    float value = 0.0f;
    if (n > 0) {
        float dx = x[0] - 3.0f;
        value += dx * dx;
        if (gradient) {
            gradient[0] = 2.0f * dx;
        }
    }
    if (n > 1) {
        float dy = x[1] + 1.0f;
        value += dy * dy;
        if (gradient) {
            gradient[1] = 2.0f * dy;
        }
    }
    for (uint32_t i = 2; i < n; ++i) {
        if (gradient) {
            gradient[i] = 2.0f * x[i];
        }
        value += x[i] * x[i];
    }
    return value;
}

static float SweepParabolaScore(float parameter, void *user_data) {
    const float center = user_data ? *(const float*)user_data : 2.0f;
    const float dx = parameter - center;
    return 5.0f - dx * dx;
}

static float TetVolumeFromPoints(UpeVec3 a, UpeVec3 b, UpeVec3 c, UpeVec3 d) {
    const float abx = b.x - a.x;
    const float aby = b.y - a.y;
    const float abz = b.z - a.z;
    const float acx = c.x - a.x;
    const float acy = c.y - a.y;
    const float acz = c.z - a.z;
    const float adx = d.x - a.x;
    const float ady = d.y - a.y;
    const float adz = d.z - a.z;
    const float cx = acy * adz - acz * ady;
    const float cy = acz * adx - acx * adz;
    const float cz = acx * ady - acy * adx;
    return fabsf(abx * cx + aby * cy + abz * cz) / 6.0f;
}

class ConfigTest : public ::testing::Test {};

TEST_F(ConfigTest, GetCapabilitiesBasic) {
    UpeCapabilities caps = UpeEngine_GetCapabilities();
    EXPECT_GT((int)caps.simd_width, 0);
    EXPECT_GT((int)caps.max_bodies, 0);
    EXPECT_GT((int)caps.max_shapes, 0);
    EXPECT_GT((int)caps.max_contacts, 0);
}

TEST_F(ConfigTest, VersionStringNonEmpty) {
    const char *version = UpeEngine_GetVersionString();
    EXPECT_NE(version, nullptr);
    EXPECT_STRNE(version, "");
    EXPECT_THAT(version, ::testing::HasSubstr("."));
}

TEST_F(ConfigTest, BuildConfigNonEmpty) {
    const char *config = UpeEngine_GetBuildConfig();
    EXPECT_NE(config, nullptr);
    EXPECT_STRNE(config, "");
}

TEST_F(ConfigTest, CoreCapabilitiesSet) {
    UpeCapabilities caps = UpeEngine_GetCapabilities();
    EXPECT_TRUE((caps.capability_bits & UPE_CAP_SCALAR_MATH) != 0u);
    EXPECT_TRUE((caps.capability_bits & UPE_CAP_BROADPHASE_SAP) != 0u);
    EXPECT_TRUE((caps.capability_bits & UPE_CAP_NARROWPHASE_SCALAR) != 0u);
    EXPECT_TRUE((caps.capability_bits & UPE_CAP_ADAPTIVE_RUNTIME) != 0u);
    EXPECT_TRUE((caps.capability_bits & UPE_CAP_SCIENTIFIC_RUNTIME) != 0u);
    EXPECT_TRUE((caps.capability_bits & UPE_CAP_NUMERICAL_TOOLBOX) != 0u);
}

TEST_F(ConfigTest, AdaptiveRuntimeConfigDefaultIsSane) {
    UpeAdaptiveRuntimeConfig config = UpeAdaptiveRuntimeConfig_Default();
    EXPECT_EQ(config.mode, UPE_ADAPTIVE_BALANCED);
    EXPECT_TRUE(config.enabled != 0);
    EXPECT_GT(config.max_solver_iterations, config.min_solver_iterations);
    EXPECT_GT(config.ccd_velocity_threshold, 0.0f);
}

TEST_F(ConfigTest, ScientificRuntimeConfigDefaultIsSane) {
    UpeScientificRuntimeConfig config = UpeScientificRuntimeConfig_Default();
    EXPECT_TRUE(config.enabled != 0);
    EXPECT_EQ(config.integrator_mode, UPE_INTEGRATOR_ADAPTIVE);
    EXPECT_TRUE(UpeUnits_IsValid(&config.units) != 0);
    EXPECT_GT(config.max_expected_linear_speed, 0.0f);
    EXPECT_GT(config.max_energy_drift_ratio, 0.0f);
    EXPECT_GT(config.error_budget.contact_margin_factor, 0.0f);
    EXPECT_GT(config.constants.standard_air_density_kg_per_m3, 0.0f);
}

TEST_F(ConfigTest, PhysicalConstantsDefaultIsSane) {
    UpePhysicalConstants constants = UpePhysicalConstants_SI();
    EXPECT_GT(constants.standard_gravity, 9.7f);
    EXPECT_LT(constants.standard_gravity, 9.9f);
    EXPECT_GT(constants.speed_of_light, 1000000.0f);
    EXPECT_GT(constants.standard_air_density_kg_per_m3, 1.0f);
}

TEST_F(ConfigTest, UnitConversionsAreConsistent) {
    UpeUnits units = UpeUnits_FromScales(0.1f, 2.0f, 0.5f);
    EXPECT_TRUE(UpeUnits_IsValid(&units) != 0);
    EXPECT_GT(UpeUnits_LengthToSI(&units, 3.0f), 0.29f);
    EXPECT_LT(UpeUnits_LengthToSI(&units, 3.0f), 0.31f);
    EXPECT_GT(UpeUnits_SpeedToSI(&units, 4.0f), 0.79f);
    EXPECT_LT(UpeUnits_SpeedToSI(&units, 4.0f), 0.81f);
    EXPECT_GT(UpeUnits_EnergyToSI(&units, 1.0f), 0.07f);
    EXPECT_LT(UpeUnits_EnergyToSI(&units, 1.0f), 0.09f);
}

TEST_F(ConfigTest, IntervalArithmeticAndReductionsAreSane) {
    UpeIntervalf a = UpeMath_Interval(1.0f, 2.0f);
    UpeIntervalf b = UpeMath_Interval(-0.5f, 4.0f);
    UpeIntervalf c = UpeMath_IntervalMul(a, b);
    EXPECT_LT(c.lo, -0.9f);
    EXPECT_GT(c.hi, 7.9f);
    EXPECT_TRUE(UpeMath_IntervalContains(c, 3.0f) != 0);

    const float values[] = {1.0f, 2.0f, 3.0f, 4.0f};
    UpeDeterministicReductionSummary summary = {};
    UpeMath_DeterministicReduceF32(values, 4u, &summary);
    EXPECT_GT((float)summary.sum, 9.9f);
    EXPECT_LT((float)summary.sum, 10.1f);
    EXPECT_GT((float)summary.mean, 2.4f);
    EXPECT_LT((float)summary.mean, 2.6f);
}

TEST_F(ConfigTest, NumericalAnalysisHelpersAreSane) {
    const float xs[] = {0.0f, 1.0f, 2.0f, 3.0f};
    const float ys[] = {1.0f, 3.0f, 5.0f, 7.0f};
    UpeLeastSquaresFit1D fit = {};
    EXPECT_TRUE(UpeMath_LeastSquaresFitLine(xs, ys, 4u, &fit) != 0);
    EXPECT_GT(fit.slope, 1.9f);
    EXPECT_LT(fit.slope, 2.1f);
    EXPECT_GT(fit.intercept, 0.9f);
    EXPECT_LT(fit.intercept, 1.1f);

    float interp = 0.0f;
    EXPECT_TRUE(UpeMath_BarycentricInterpolate1D(xs, ys, 4u, 1.5f, &interp) != 0);
    EXPECT_GT(interp, 3.9f);
    EXPECT_LT(interp, 4.1f);

    const float signal[] = {0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, -1.0f};
    float dominant_hz = UpeMath_DominantFrequencyEstimate(signal, 8u, 8.0f);
    EXPECT_GT(dominant_hz, 1.9f);
    EXPECT_LT(dominant_hz, 2.1f);

    float cond = UpeMath_ConditionNumber2x2(4.0f, 1.0f, 2.0f, 3.0f);
    EXPECT_GT(cond, 1.0f);
}

TEST_F(ConfigTest, SparseLinearAlgebraAndFftAreSane) {
    UpeSparseMatrix A = {};
    UpeSparse_Init(&A, 2u, 2u, 4u);
    UpeSparse_SetValue(&A, 0u, 0u, 4.0f);
    UpeSparse_SetValue(&A, 0u, 1u, 1.0f);
    UpeSparse_SetValue(&A, 1u, 0u, 1.0f);
    UpeSparse_SetValue(&A, 1u, 1u, 3.0f);

    float x_probe[] = {1.0f, 1.0f};
    float ax[] = {0.0f, 0.0f};
    UpeSparse_MatVecMul(&A, x_probe, ax);
    EXPECT_GT(ax[0], 4.9f);
    EXPECT_LT(ax[0], 5.1f);
    EXPECT_GT(ax[1], 3.9f);
    EXPECT_LT(ax[1], 4.1f);

    UpeSparseMatrix L = {};
    UpeMath_IncompleteCholesky(&A, &L);
    EXPECT_TRUE(L.values != nullptr);

    const float b[] = {1.0f, 2.0f};
    float x_cg[] = {0.0f, 0.0f};
    int cg_iters = UpeMath_ConjugateGradient(&A, b, x_cg, 32u, 1.0e-6f);
    EXPECT_GT(cg_iters, 0);
    EXPECT_GT(x_cg[0], 0.08f);
    EXPECT_LT(x_cg[0], 0.11f);
    EXPECT_GT(x_cg[1], 0.62f);
    EXPECT_LT(x_cg[1], 0.65f);

    float x_gmres[] = {0.0f, 0.0f};
    int gmres_iters = UpeMath_GMRES(&A, b, x_gmres, 32u, 1.0e-6f, 4u);
    EXPECT_GT(gmres_iters, 0);
    EXPECT_GT(x_gmres[0], 0.08f);
    EXPECT_LT(x_gmres[0], 0.11f);
    EXPECT_GT(x_gmres[1], 0.62f);
    EXPECT_LT(x_gmres[1], 0.65f);

    const float real[] = {1.0f, 1.0f, 1.0f, 1.0f};
    const float imag[] = {0.0f, 0.0f, 0.0f, 0.0f};
    float out_real[4] = {};
    float out_imag[4] = {};
    UpeMath_FFT(real, imag, out_real, out_imag, 4u);
    EXPECT_GT(out_real[0], 3.9f);
    EXPECT_LT(out_real[0], 4.1f);
    EXPECT_GT(out_real[1], -0.1f);
    EXPECT_LT(out_real[1], 0.1f);

    float inv_real[4] = {};
    float inv_imag[4] = {};
    UpeMath_IFFT(out_real, out_imag, inv_real, inv_imag, 4u);
    EXPECT_GT(inv_real[0], 0.9f);
    EXPECT_LT(inv_real[0], 1.1f);

    UpeSparse_Destroy(&L);
    UpeSparse_Destroy(&A);
}

TEST_F(ConfigTest, ODESteppersAndOptimizersAreSane) {
    float y_euler[1] = {1.0f};
    UpeMath_EulerStep(ExpGrowthRhs, 0.0f, y_euler, 1u, 0.1f, nullptr);
    EXPECT_GT(y_euler[0], 1.09f);
    EXPECT_LT(y_euler[0], 1.11f);

    float y_rk4[1] = {1.0f};
    UpeMath_RK4Step(ExpGrowthRhs, 0.0f, y_rk4, 1u, 0.1f, nullptr);
    EXPECT_GT(y_rk4[0], 1.104f);
    EXPECT_LT(y_rk4[0], 1.106f);

    float t = 0.0f;
    float dt = 0.1f;
    float y_adapt[1] = {1.0f};
    EXPECT_TRUE(UpeMath_RK45AdaptiveStep(ExpGrowthRhs, &t, y_adapt, 1u, &dt, 1.0e-5f, nullptr) != 0);
    EXPECT_GT(t, 0.0f);
    EXPECT_GT(y_adapt[0], 1.09f);

    float x_gd[2] = {0.0f, 0.0f};
    int gd_iters = UpeMath_GradientDescent(QuadraticBowlObjective, x_gd, 2u, 0.1f, 128u, 1.0e-4f, nullptr);
    EXPECT_GT(gd_iters, 0);
    EXPECT_GT(x_gd[0], 2.9f);
    EXPECT_LT(x_gd[0], 3.1f);
    EXPECT_GT(x_gd[1], -1.1f);
    EXPECT_LT(x_gd[1], -0.9f);

    float x_newton[2] = {0.0f, 0.0f};
    int newton_iters = UpeMath_Newton(QuadraticBowlObjective, x_newton, 2u, 32u, 1.0e-4f, nullptr);
    EXPECT_GT(newton_iters, 0);
    EXPECT_GT(x_newton[0], 2.9f);
    EXPECT_LT(x_newton[0], 3.1f);
    EXPECT_GT(x_newton[1], -1.1f);
    EXPECT_LT(x_newton[1], -0.9f);

    float x_lbfgs[2] = {0.0f, 0.0f};
    int lbfgs_iters = UpeMath_LBFGS(QuadraticBowlObjective, x_lbfgs, 2u, 64u, 1.0e-4f, 4u, nullptr);
    EXPECT_GT(lbfgs_iters, 0);
    EXPECT_GT(x_lbfgs[0], 2.9f);
    EXPECT_LT(x_lbfgs[0], 3.1f);
    EXPECT_GT(x_lbfgs[1], -1.1f);
    EXPECT_LT(x_lbfgs[1], -0.9f);
}

TEST_F(ConfigTest, StatisticalValidationHelpersAreSane) {
    const float observations[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    UpeStatMoments moments = {};
    EXPECT_TRUE(UpeMath_ComputeMomentsF32(observations, 5u, &moments) != 0);
    EXPECT_GT((float)moments.mean, 2.9f);
    EXPECT_LT((float)moments.mean, 3.1f);
    EXPECT_GT((float)moments.stddev, 1.3f);
    EXPECT_LT((float)moments.stddev, 1.5f);
    EXPECT_LT(moments.confidence95_low, 3.0f);
    EXPECT_GT(moments.confidence95_high, 3.0f);

    float ci_low = 0.0f;
    float ci_high = 0.0f;
    EXPECT_TRUE(UpeMath_ConfidenceInterval95(observations, 5u, &ci_low, &ci_high) != 0);
    EXPECT_LT(ci_low, 3.0f);
    EXPECT_GT(ci_high, 3.0f);

    const float predicted[] = {0.9f, 2.1f, 2.9f, 4.2f, 4.8f};
    float covariance = UpeMath_CovarianceF32(observations, predicted, 5u);
    EXPECT_GT(covariance, 1.5f);
    float rmse = UpeMath_RootMeanSquareErrorF32(predicted, observations, 5u);
    EXPECT_GT(rmse, 0.0f);
    EXPECT_LT(rmse, 0.25f);
}

TEST_F(ConfigTest, CertificationHarnessBasicsAreSane) {
    UpeTestSuite suite = {};
    UpeCert_InitSuite(&suite, "Infrastructure Certification");
    EXPECT_THAT(suite.name, ::testing::HasSubstr("Infrastructure"));

    UpeTestConfig drift = UpeCert_TestLongRunDrift();
    UpeTestConfig replay = UpeCert_TestReplay();
    EXPECT_GT(drift.step_count, 0u);
    EXPECT_GT(replay.max_duration_ms, 0u);

    UpeCert_AddTest(&suite, &drift);
    UpeCert_AddTest(&suite, &replay);
    EXPECT_EQ(suite.test_count, 2u);

    UpeCert_RunAll(&suite);
    EXPECT_EQ(suite.result_count, suite.test_count);
    EXPECT_EQ(suite.passed + suite.failed + suite.skipped, suite.result_count);
    EXPECT_THAT(suite.results[0].test_name, ::testing::HasSubstr("Long"));

    const char *report_path = "certification_test_report.json";
    UpeCert_ExportResults(&suite, report_path);
    FILE *report = fopen(report_path, "rb");
    EXPECT_NE(report, nullptr);
    if (report) {
        fclose(report);
        remove(report_path);
    }

    UpeCert_DestroySuite(&suite);
    EXPECT_EQ(suite.test_count, 0u);
    EXPECT_EQ(suite.result_count, 0u);
}

TEST_F(ConfigTest, MonteCarloAndCalibrationReportsAreSane) {
    const float samples[] = {0.95f, 1.05f, 1.10f, 0.98f, 1.02f, 1.08f, 0.99f};
    UpeMonteCarloSummary summary = {};
    EXPECT_TRUE(UpeCert_SummarizeMonteCarlo(samples, 7u, &summary) != 0);
    EXPECT_GT(summary.mean, 0.99f);
    EXPECT_LT(summary.mean, 1.05f);
    EXPECT_GT(summary.p95, summary.p50);
    EXPECT_GT(summary.stddev, 0.0f);

    const float reference[] = {2.0f, 4.0f, 6.0f, 8.0f, 10.0f};
    const float measured[] = {2.4f, 4.4f, 6.4f, 8.4f, 10.4f};
    UpeCalibrationReport report = {};
    EXPECT_TRUE(UpeCert_RunMeasurementCalibration(reference, measured, 5u, &report) != 0);
    EXPECT_GT(report.sample_count, 0u);
    EXPECT_GT(report.scale, 0.95f);
    EXPECT_LT(report.scale, 1.05f);
    EXPECT_GT(report.bias, -0.5f);
    EXPECT_LT(report.bias, -0.3f);
    EXPECT_LT(report.rmse_after, report.rmse_before);
    EXPECT_LT(report.confidence95_low, report.mean_corrected_error);
    EXPECT_GT(report.confidence95_high, report.mean_corrected_error);

    char calibration_json[512] = {};
    const uint32_t calibration_len = UpeCert_WriteCalibrationReportJson(&report, calibration_json, sizeof(calibration_json));
    EXPECT_GT(calibration_len, 0u);
    EXPECT_THAT(calibration_json, ::testing::HasSubstr("\"rmse_after\""));
}

TEST_F(ConfigTest, BootstrapAndParameterSweepAreSane) {
    const float samples[] = {0.9f, 1.0f, 1.1f, 0.95f, 1.05f, 1.08f, 0.97f};
    UpeBootstrapSummary bootstrap = {};
    EXPECT_TRUE(UpeMath_BootstrapMeanInterval(samples, 7u, 128u, &bootstrap) != 0);
    EXPECT_GT(bootstrap.mean, 0.95f);
    EXPECT_LT(bootstrap.mean, 1.05f);
    EXPECT_GT(bootstrap.resample_stddev, 0.0f);
    EXPECT_LT(bootstrap.confidence95_low, bootstrap.mean);
    EXPECT_GT(bootstrap.confidence95_high, bootstrap.mean);

    float center = 2.0f;
    UpeParameterSweepSummary sweep = {};
    EXPECT_TRUE(UpeCert_RunParameterSweep1D(0.0f, 4.0f, 17u, SweepParabolaScore, &center, &sweep) != 0);
    EXPECT_GT(sweep.best_parameter, 1.7f);
    EXPECT_LT(sweep.best_parameter, 2.3f);
    EXPECT_GT(sweep.best_score, 4.8f);
    EXPECT_LT(fabsf(sweep.estimated_sensitivity), 0.6f);
    EXPECT_GT(sweep.stddev_score, 0.0f);

    UpeSensitivityAnalysis1D sensitivity = {};
    EXPECT_TRUE(UpeCert_RunSensitivityAnalysis1D(1.5f, 0.25f, SweepParabolaScore, &center, &sensitivity) != 0);
    EXPECT_GT(sensitivity.gradient, 0.9f);
    EXPECT_LT(sensitivity.gradient, 1.1f);
    EXPECT_GT(sensitivity.curvature, -2.1f);
    EXPECT_LT(sensitivity.curvature, -1.9f);

    char sweep_json[512] = {};
    const uint32_t sweep_len = UpeCert_WriteParameterSweepJson(&sweep, sweep_json, sizeof(sweep_json));
    EXPECT_GT(sweep_len, 0u);
    EXPECT_THAT(sweep_json, ::testing::HasSubstr("\"best_parameter\""));
}

TEST_F(ConfigTest, RobustStatisticsAndConservationAuditAreSane) {
    const float values[] = {1.0f, 1.1f, 0.9f, 1.05f, 0.95f};
    float moving_average[5] = {};
    EXPECT_TRUE(UpeMath_MovingAverageF32(values, 5u, 3u, moving_average) != 0);
    EXPECT_GT(moving_average[2], 1.01f);
    EXPECT_LT(moving_average[2], 1.03f);

    float ema[5] = {};
    EXPECT_TRUE(UpeMath_ExponentialMovingAverageF32(values, 5u, 0.5f, ema) != 0);
    EXPECT_GT(ema[4], 0.97f);
    EXPECT_LT(ema[4], 1.01f);

    const float outlier_values[] = {1.0f, 1.0f, 1.0f, 1.0f, 10.0f};
    uint8_t outlier_mask[5] = {};
    EXPECT_TRUE(UpeMath_DetectOutlierMaskZScoreF32(outlier_values, 5u, 1.5f, outlier_mask) != 0);
    EXPECT_EQ(outlier_mask[4], 1u);

    float trimmed_mean = UpeMath_TrimmedMeanF32(outlier_values, 5u, 0.2f);
    EXPECT_GT(trimmed_mean, 0.99f);
    EXPECT_LT(trimmed_mean, 1.01f);

    float mad = UpeMath_MedianAbsoluteDeviationF32(values, 5u);
    EXPECT_GT(mad, 0.04f);
    EXPECT_LT(mad, 0.11f);

    const float xs[] = {1.0f, 2.0f, 3.0f, 4.0f};
    const float ys[] = {2.0f, 4.0f, 6.0f, 8.0f};
    float corr = UpeMath_CorrelationCoefficientF32(xs, ys, 4u);
    EXPECT_GT(corr, 0.99f);

    const float observed[] = {1.0f, 2.0f, 3.0f, 4.0f};
    const float predicted[] = {1.1f, 1.9f, 3.05f, 3.95f};
    float nrmse = UpeMath_NormalizedRMSEF32(predicted, observed, 4u);
    EXPECT_GT(nrmse, 0.0f);
    EXPECT_LT(nrmse, 0.06f);

    const float energy_series[] = {100.0f, 99.8f, 99.9f, 99.7f, 99.6f};
    UpeConservationAudit audit = {};
    EXPECT_TRUE(UpeCert_AnalyzeConservationSeries(energy_series, 5u, &audit) != 0);
    EXPECT_LT(audit.peak_relative_drift, 0.01f);
    EXPECT_GT(audit.stability_score, 0.8f);
    EXPECT_GT(audit.worst_sample_index, 0u);

    char audit_json[512] = {};
    const uint32_t audit_len = UpeCert_WriteConservationAuditJson(&audit, audit_json, sizeof(audit_json));
    EXPECT_GT(audit_len, 0u);
    EXPECT_THAT(audit_json, ::testing::HasSubstr("\"peak_relative_drift\""));
}

TEST_F(ConfigTest, SystemIdentificationAndSamplingReportsAreSane) {
    float samples[8] = {};
    EXPECT_TRUE(UpeCert_GenerateLatinHypercube1D(-1.0f, 1.0f, 8u, 42u, samples) != 0);
    for (uint32_t i = 0; i < 8u; ++i) {
        EXPECT_GT(samples[i], -1.01f);
        EXPECT_LT(samples[i], 1.01f);
    }

    UpeMonteCarloSummary summary = {};
    EXPECT_TRUE(UpeCert_SummarizeMonteCarlo(samples, 8u, &summary) != 0);
    char mc_json[512] = {};
    const uint32_t mc_len = UpeCert_WriteMonteCarloSummaryJson(&summary, mc_json, sizeof(mc_json));
    EXPECT_GT(mc_len, 0u);
    EXPECT_THAT(mc_json, ::testing::HasSubstr("\"p95\""));

    const float inputs[] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f};
    const float outputs[] = {1.0f, 3.0f, 5.0f, 7.0f, 9.0f};
    UpeSystemIdentificationReport report = {};
    EXPECT_TRUE(UpeCert_RunAffineSystemIdentification(inputs, outputs, 5u, &report) != 0);
    EXPECT_GT(report.scale, 1.9f);
    EXPECT_LT(report.scale, 2.1f);
    EXPECT_GT(report.bias, 0.9f);
    EXPECT_LT(report.bias, 1.1f);
    EXPECT_GT(report.r_squared, 0.99f);

    char ident_json[512] = {};
    const uint32_t ident_len = UpeCert_WriteSystemIdentificationJson(&report, ident_json, sizeof(ident_json));
    EXPECT_GT(ident_len, 0u);
    EXPECT_THAT(ident_json, ::testing::HasSubstr("\"r_squared\""));
}

TEST_F(ConfigTest, WeightedFittingAndRobustnessReportsAreSane) {
    const float weighted_values[] = {1.0f, 2.0f, 10.0f};
    const float weights[] = {1.0f, 2.0f, 1.0f};
    float weighted_mean = UpeMath_WeightedMeanF32(weighted_values, weights, 3u);
    EXPECT_GT(weighted_mean, 3.6f);
    EXPECT_LT(weighted_mean, 3.9f);

    const float xs[] = {0.0f, 1.0f, 2.0f, 3.0f};
    const float ys[] = {1.0f, 3.0f, 5.0f, 7.0f};
    const float fit_weights[] = {1.0f, 1.0f, 2.0f, 2.0f};
    UpeLeastSquaresFit1D weighted_fit = {};
    EXPECT_TRUE(UpeMath_WeightedLeastSquaresFitLine(xs, ys, fit_weights, 4u, &weighted_fit) != 0);
    EXPECT_GT(weighted_fit.slope, 1.9f);
    EXPECT_LT(weighted_fit.slope, 2.1f);
    EXPECT_GT(weighted_fit.intercept, 0.9f);
    EXPECT_LT(weighted_fit.intercept, 1.1f);

    const float quantile_values[] = {5.0f, 1.0f, 3.0f, 2.0f, 4.0f};
    float median = UpeMath_QuantileF32(quantile_values, 5u, 0.5f);
    EXPECT_GT(median, 2.9f);
    EXPECT_LT(median, 3.1f);

    uint32_t histogram[4] = {};
    EXPECT_TRUE(UpeMath_HistogramF32(quantile_values, 5u, 1.0f, 5.0f, 4u, histogram) != 0);
    EXPECT_EQ(histogram[0] + histogram[1] + histogram[2] + histogram[3], 5u);
    float entropy = UpeMath_ShannonEntropyFromHistogram(histogram, 4u);
    EXPECT_GT(entropy, 1.0f);

    const float sample_a[] = {0.95f, 1.00f, 1.05f, 1.10f, 0.98f};
    const float sample_b[] = {1.20f, 1.25f, 1.30f, 1.35f, 1.40f};
    float ks = UpeMath_KolmogorovSmirnovDistanceF32(sample_a, 5u, sample_b, 5u);
    EXPECT_GT(ks, 0.5f);

    UpeDistributionComparison comparison = {};
    EXPECT_TRUE(UpeCert_CompareSampleDistributions(sample_a, 5u, sample_b, 5u, &comparison) != 0);
    EXPECT_GT(comparison.mean_shift, 0.2f);
    EXPECT_GT(comparison.ks_distance, 0.5f);
    EXPECT_LT(comparison.overlap_score, 0.6f);

    char distribution_json[512] = {};
    const uint32_t distribution_len = UpeCert_WriteDistributionComparisonJson(&comparison, distribution_json, sizeof(distribution_json));
    EXPECT_GT(distribution_len, 0u);
    EXPECT_THAT(distribution_json, ::testing::HasSubstr("\"ks_distance\""));

    const float threshold_values[] = {1.1f, 1.2f, 0.9f, 1.3f, 1.4f};
    UpeThresholdRobustnessReport robustness = {};
    EXPECT_TRUE(UpeCert_AnalyzeThresholdRobustness(threshold_values, 5u, 1.0f, 1, &robustness) != 0);
    EXPECT_EQ(robustness.pass_count, 4u);
    EXPECT_GT(robustness.pass_rate, 0.79f);
    EXPECT_LT(robustness.pass_rate, 0.81f);

    char robustness_json[512] = {};
    const uint32_t robustness_len = UpeCert_WriteThresholdRobustnessJson(&robustness, robustness_json, sizeof(robustness_json));
    EXPECT_GT(robustness_len, 0u);
    EXPECT_THAT(robustness_json, ::testing::HasSubstr("\"pass_rate\""));
}

TEST_F(ConfigTest, ReliabilityCurveAndCalibrationMetricsAreSane) {
    const float reliability_values[] = {0.6f, 0.8f, 1.0f, 1.2f, 1.4f};
    UpeReliabilityCurvePoint points[4] = {};
    UpeReliabilityCurveSummary summary = {};
    EXPECT_TRUE(UpeCert_BuildReliabilityCurve(reliability_values, 5u, 0.7f, 1.3f, 4u, 1, points, &summary) != 0);
    EXPECT_EQ(summary.point_count, 4u);
    EXPECT_GT(summary.max_pass_rate, summary.min_pass_rate);
    EXPECT_GT(summary.monotonicity_score, 0.9f);

    char reliability_json[512] = {};
    const uint32_t reliability_len = UpeCert_WriteReliabilityCurveJson(&summary, points, reliability_json, sizeof(reliability_json));
    EXPECT_GT(reliability_len, 0u);
    EXPECT_THAT(reliability_json, ::testing::HasSubstr("\"mean_pass_rate\""));

    const float predicted[] = {1.0f, 2.0f, 3.0f, 4.0f};
    const float observed[] = {1.05f, 1.95f, 3.02f, 3.98f};
    UpeUncertaintyCalibrationReport calibration = {};
    EXPECT_TRUE(UpeCert_AnalyzeUncertaintyCalibration(predicted, observed, 4u, 0.1f, &calibration) != 0);
    EXPECT_LT(fabsf(calibration.bias), 0.1f);
    EXPECT_LT(calibration.nrmse, 0.03f);
    EXPECT_GT(calibration.coverage_ratio, 0.9f);
    EXPECT_GT(calibration.calibration_score, 0.8f);

    char calibration_json[512] = {};
    const uint32_t calibration_len = UpeCert_WriteUncertaintyCalibrationJson(&calibration, calibration_json, sizeof(calibration_json));
    EXPECT_GT(calibration_len, 0u);
    EXPECT_THAT(calibration_json, ::testing::HasSubstr("\"coverage_ratio\""));
}

TEST_F(ConfigTest, ClassificationOperatingPointsAreSane) {
    const float scores[] = {0.95f, 0.80f, 0.70f, 0.40f, 0.20f, 0.10f};
    const uint8_t labels[] = {1u, 1u, 0u, 1u, 0u, 0u};

    UpeConfusionMatrix matrix = {};
    EXPECT_TRUE(UpeCert_ComputeConfusionMatrix(scores, labels, 6u, 0.5f, &matrix) != 0);
    EXPECT_EQ(matrix.true_positive, 2u);
    EXPECT_EQ(matrix.false_positive, 1u);
    EXPECT_EQ(matrix.true_negative, 2u);
    EXPECT_EQ(matrix.false_negative, 1u);
    EXPECT_GT(matrix.precision, 0.6f);
    EXPECT_LT(matrix.precision, 0.7f);
    EXPECT_GT(matrix.recall, 0.6f);
    EXPECT_LT(matrix.recall, 0.7f);

    char matrix_json[512] = {};
    const uint32_t matrix_len = UpeCert_WriteConfusionMatrixJson(&matrix, matrix_json, sizeof(matrix_json));
    EXPECT_GT(matrix_len, 0u);
    EXPECT_THAT(matrix_json, ::testing::HasSubstr("\"f1_score\""));

    UpeRocPoint roc_points[5] = {};
    UpeRocCurveSummary roc = {};
    EXPECT_TRUE(UpeCert_BuildRocCurve(scores, labels, 6u, 0.2f, 0.9f, 5u, roc_points, &roc) != 0);
    EXPECT_EQ(roc.point_count, 5u);
    EXPECT_GT(roc.auc_estimate, 0.5f);
    EXPECT_LT(roc.auc_estimate, 1.01f);
    EXPECT_GT(roc.best_f1_score, 0.6f);

    char roc_json[512] = {};
    const uint32_t roc_len = UpeCert_WriteRocCurveJson(&roc, roc_points, roc_json, sizeof(roc_json));
    EXPECT_GT(roc_len, 0u);
    EXPECT_THAT(roc_json, ::testing::HasSubstr("\"auc_estimate\""));
}

TEST_F(ConfigTest, ProbabilisticForecastScoresAndPrecisionRecallAreSane) {
    const float probabilities[] = {0.95f, 0.80f, 0.70f, 0.40f, 0.20f, 0.10f};
    const uint8_t labels[] = {1u, 1u, 0u, 1u, 0u, 0u};

    UpeProbabilityScoreReport report = {};
    EXPECT_TRUE(UpeCert_AnalyzeProbabilityScores(probabilities, labels, 6u, 0.5f, 3u, &report) != 0);
    EXPECT_EQ(report.sample_count, 6u);
    EXPECT_GT(report.brier_score, 0.1f);
    EXPECT_LT(report.brier_score, 0.2f);
    EXPECT_GT(report.log_loss, 0.3f);
    EXPECT_LT(report.log_loss, 0.6f);
    EXPECT_GT(report.expected_calibration_error, 0.1f);
    EXPECT_LT(report.expected_calibration_error, 0.3f);
    EXPECT_GT(report.balanced_accuracy, 0.6f);
    EXPECT_LT(report.balanced_accuracy, 0.7f);
    EXPECT_GT(report.matthews_corrcoef, 0.3f);
    EXPECT_LT(report.matthews_corrcoef, 0.35f);

    char score_json[512] = {};
    const uint32_t score_len = UpeCert_WriteProbabilityScoreJson(&report, score_json, sizeof(score_json));
    EXPECT_GT(score_len, 0u);
    EXPECT_THAT(score_json, ::testing::HasSubstr("\"brier_score\""));

    UpePrecisionRecallPoint pr_points[5] = {};
    UpePrecisionRecallCurveSummary pr = {};
    EXPECT_TRUE(UpeCert_BuildPrecisionRecallCurve(probabilities, labels, 6u, 0.2f, 0.9f, 5u, pr_points, &pr) != 0);
    EXPECT_EQ(pr.point_count, 5u);
    EXPECT_GT(pr.average_precision_estimate, 0.5f);
    EXPECT_LT(pr.average_precision_estimate, 1.01f);
    EXPECT_GT(pr.best_f1_score, 0.7f);
    EXPECT_GT(pr.max_precision, 0.99f);
    EXPECT_GT(pr.max_recall, 0.99f);

    char pr_json[512] = {};
    const uint32_t pr_len = UpeCert_WritePrecisionRecallCurveJson(&pr, pr_points, pr_json, sizeof(pr_json));
    EXPECT_GT(pr_len, 0u);
    EXPECT_THAT(pr_json, ::testing::HasSubstr("\"average_precision_estimate\""));
}

TEST_F(ConfigTest, StepResponseAndTrackingQualityAreSane) {
    const float times[] = {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f};
    const float response[] = {0.0f, 0.35f, 0.78f, 1.05f, 1.02f, 1.00f, 0.99f, 1.00f};

    UpeStepResponseReport step = {};
    EXPECT_TRUE(UpeCert_AnalyzeStepResponse(times, response, 8u, 1.0f, 0.05f, &step) != 0);
    EXPECT_EQ(step.sample_count, 8u);
    EXPECT_GT(step.overshoot_ratio, 0.04f);
    EXPECT_LT(step.overshoot_ratio, 0.06f);
    EXPECT_GT(step.rise_time, 0.15f);
    EXPECT_LT(step.rise_time, 0.35f);
    EXPECT_GT(step.settling_time, 0.25f);
    EXPECT_LT(step.settling_time, 0.45f);
    EXPECT_LT(fabsf(step.steady_state_error), 0.02f);
    EXPECT_GT(step.oscillation_score, 0.0f);
    EXPECT_LT(step.oscillation_score, 0.5f);

    char step_json[512] = {};
    const uint32_t step_len = UpeCert_WriteStepResponseJson(&step, step_json, sizeof(step_json));
    EXPECT_GT(step_len, 0u);
    EXPECT_THAT(step_json, ::testing::HasSubstr("\"overshoot_ratio\""));

    const float reference[] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f};
    const float observed[] = {0.1f, 0.9f, 2.1f, 2.8f, 4.2f};
    UpeTrackingErrorReport tracking = {};
    EXPECT_TRUE(UpeCert_AnalyzeTrackingErrorSeries(reference, observed, times, 5u, &tracking) != 0);
    EXPECT_EQ(tracking.sample_count, 5u);
    EXPECT_GT(tracking.mean_abs_error, 0.13f);
    EXPECT_LT(tracking.mean_abs_error, 0.15f);
    EXPECT_GT(tracking.rmse, 0.13f);
    EXPECT_LT(tracking.rmse, 0.16f);
    EXPECT_GT(tracking.max_abs_error, 0.19f);
    EXPECT_LT(tracking.max_abs_error, 0.21f);
    EXPECT_GT(tracking.integral_abs_error, 0.05f);
    EXPECT_LT(tracking.integral_abs_error, 0.06f);
    EXPECT_GT(tracking.fit_r_squared, 0.98f);

    char tracking_json[512] = {};
    const uint32_t tracking_len = UpeCert_WriteTrackingErrorJson(&tracking, tracking_json, sizeof(tracking_json));
    EXPECT_GT(tracking_len, 0u);
    EXPECT_THAT(tracking_json, ::testing::HasSubstr("\"integral_abs_error\""));
}

TEST_F(ConfigTest, PdeDiscretizationAndTopologyAnalysisAreSane) {
    const float field[] = {0.0f, 1.0f, 0.0f, -1.0f, 0.0f};
    float laplacian[5] = {};
    EXPECT_TRUE(UpeMath_Laplacian1D(field, 5u, 1.0f, laplacian) != 0);
    EXPECT_LT(laplacian[1], -1.9f);
    EXPECT_GT(laplacian[3], 1.9f);

    float diffused[5] = {};
    EXPECT_TRUE(UpeMath_DiffusionStep1D(field, 5u, 0.1f, 0.5f, 1.0f, diffused) != 0);
    EXPECT_GT(diffused[1], 0.89f);
    EXPECT_LT(diffused[1], 0.91f);
    EXPECT_GT(diffused[3], -0.91f);
    EXPECT_LT(diffused[3], -0.89f);

    const float rhs[] = {0.0f, 1.0f, 1.0f, 1.0f, 0.0f};
    float solution[5] = {};
    EXPECT_TRUE(UpeMath_PoissonJacobi1D(rhs, 5u, 1.0f, 64u, solution) != 0);
    EXPECT_GT(solution[1], 1.4f);
    EXPECT_LT(solution[1], 1.6f);
    EXPECT_GT(solution[2], 1.9f);
    EXPECT_LT(solution[2], 2.1f);

    float cfl = UpeMath_CourantNumber1D(2.0f, 0.1f, 0.5f);
    EXPECT_GT(cfl, 0.39f);
    EXPECT_LT(cfl, 0.41f);

    float diffusion_number = UpeMath_DiffusionNumber1D(0.25f, 0.1f, 0.5f);
    EXPECT_GT(diffusion_number, 0.09f);
    EXPECT_LT(diffusion_number, 0.11f);

    float tetra_vertices[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };
    uint32_t tetra_indices[] = {
        0u, 2u, 1u,
        0u, 1u, 3u,
        0u, 3u, 2u,
        1u, 2u, 3u
    };
    UpeMesh mesh = {tetra_vertices, tetra_indices, 4u, 4u};
    UpeMeshTopologySummary topology = {};
    EXPECT_TRUE(UpeMesh_AnalyzeTopology(&mesh, &topology) != 0);
    EXPECT_EQ(topology.vertex_count, 4u);
    EXPECT_EQ(topology.triangle_count, 4u);
    EXPECT_EQ(topology.edge_count, 6u);
    EXPECT_EQ(topology.boundary_edge_count, 0u);
    EXPECT_EQ(topology.nonmanifold_edge_count, 0u);
    EXPECT_EQ(topology.connected_components, 1u);
    EXPECT_TRUE(topology.is_watertight != 0);
    EXPECT_TRUE(topology.is_orientable != 0);
    EXPECT_EQ(topology.euler_characteristic, 2);
}

TEST_F(ConfigTest, AdvancedPdeOperatorsAndMultigridToolsAreSane) {
    const float ramp[] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f};
    float advected[5] = {};
    EXPECT_TRUE(UpeMath_AdvectionStep1D(ramp, 5u, 1.0f, 0.5f, 1.0f, advected) != 0);
    EXPECT_GT(advected[1], 0.49f);
    EXPECT_LT(advected[1], 0.51f);
    EXPECT_GT(advected[3], 2.49f);
    EXPECT_LT(advected[3], 2.51f);

    const float current[] = {0.0f, 1.0f, 0.0f, -1.0f, 0.0f};
    const float previous[] = {0.0f, 1.0f, 0.0f, -1.0f, 0.0f};
    float wave_next[5] = {};
    EXPECT_TRUE(UpeMath_WaveStep1D(current, previous, 5u, 1.0f, 0.5f, 1.0f, 0.0f, wave_next) != 0);
    EXPECT_GT(wave_next[1], 0.49f);
    EXPECT_LT(wave_next[1], 0.51f);
    EXPECT_GT(wave_next[3], -0.51f);
    EXPECT_LT(wave_next[3], -0.49f);

    const float lower[] = {0.0f, -1.0f, -1.0f};
    const float diag[] = {2.0f, 2.0f, 2.0f};
    const float upper[] = {-1.0f, -1.0f, 0.0f};
    const float rhs[] = {1.0f, 0.0f, 1.0f};
    float solved[3] = {};
    EXPECT_TRUE(UpeMath_TridiagonalSolve(lower, diag, upper, rhs, 3u, solved) != 0);
    EXPECT_GT(solved[0], 0.99f);
    EXPECT_LT(solved[0], 1.01f);
    EXPECT_GT(solved[1], 0.99f);
    EXPECT_LT(solved[1], 1.01f);
    EXPECT_GT(solved[2], 0.99f);
    EXPECT_LT(solved[2], 1.01f);

    float coarse[3] = {};
    EXPECT_TRUE(UpeMath_RestrictHalfWeighting1D(ramp, 5u, coarse, 3u) != 0);
    EXPECT_GT(coarse[1], 1.99f);
    EXPECT_LT(coarse[1], 2.01f);

    const float coarse_seed[] = {0.0f, 2.0f, 4.0f};
    float fine[5] = {};
    EXPECT_TRUE(UpeMath_ProlongateLinear1D(coarse_seed, 3u, fine, 5u) != 0);
    EXPECT_GT(fine[1], 0.99f);
    EXPECT_LT(fine[1], 1.01f);
    EXPECT_GT(fine[3], 2.99f);
    EXPECT_LT(fine[3], 3.01f);
}

TEST_F(ConfigTest, ContinuumMaterialMathIsSane) {
    float stress = UpeMath_LinearElasticStress1D(0.01f, 200.0f);
    EXPECT_GT(stress, 1.99f);
    EXPECT_LT(stress, 2.01f);

    float shear = UpeMath_ShearModulus(200.0f, 0.25f);
    EXPECT_GT(shear, 79.9f);
    EXPECT_LT(shear, 80.1f);

    float bulk = UpeMath_BulkModulus(200.0f, 0.25f);
    EXPECT_GT(bulk, 133.2f);
    EXPECT_LT(bulk, 133.4f);

    float von_mises = UpeMath_VonMisesPlaneStress(10.0f, 5.0f, 3.0f);
    EXPECT_GT(von_mises, 10.0f);
    EXPECT_LT(von_mises, 10.2f);

    float relaxed = UpeMath_MaxwellStressStep(10.0f, 0.0f, 100.0f, 200.0f, 0.1f);
    EXPECT_GT(relaxed, 9.4f);
    EXPECT_LT(relaxed, 9.6f);

    float driven = UpeMath_MaxwellStressStep(10.0f, 0.02f, 100.0f, 200.0f, 0.1f);
    EXPECT_GT(driven, relaxed);
    EXPECT_LT(driven, 9.8f);
}

TEST_F(ConfigTest, FiniteVolumeAndConstitutiveMathAreSane) {
    float energy_density = UpeMath_ElasticEnergyDensity1D(0.02f, 100.0f);
    EXPECT_GT(energy_density, 0.019f);
    EXPECT_LT(energy_density, 0.021f);

    float yield_ratio = UpeMath_YieldUtilizationVonMises(10.0f, 5.0f, 3.0f, 20.0f);
    EXPECT_GT(yield_ratio, 0.50f);
    EXPECT_LT(yield_ratio, 0.52f);

    float upwind_flux = UpeMath_UpwindFlux1D(2.0f, 5.0f, 3.0f);
    EXPECT_GT(upwind_flux, 5.9f);
    EXPECT_LT(upwind_flux, 6.1f);

    float rusanov_flux = UpeMath_RusanovFlux1D(2.0f, 5.0f, 3.0f);
    EXPECT_GT(rusanov_flux, -1.1f);
    EXPECT_LT(rusanov_flux, -0.9f);

    const float interface_flux[] = {0.0f, 1.0f, 2.0f, 1.0f};
    float divergence[3] = {};
    EXPECT_TRUE(UpeMath_FiniteVolumeDivergence1D(interface_flux, 3u, 1.0f, divergence) != 0);
    EXPECT_GT(divergence[0], -1.01f);
    EXPECT_LT(divergence[0], -0.99f);
    EXPECT_GT(divergence[1], -1.01f);
    EXPECT_LT(divergence[1], -0.99f);
    EXPECT_GT(divergence[2], 0.99f);
    EXPECT_LT(divergence[2], 1.01f);

    const float cells[] = {1.0f, 2.0f, 3.0f, 4.0f};
    float updated[4] = {};
    EXPECT_TRUE(UpeMath_FiniteVolumeAdvectionStep1D(cells, 4u, 1.0f, 0.25f, 1.0f, updated) != 0);
    EXPECT_GT(updated[1], 1.74f);
    EXPECT_LT(updated[1], 1.76f);
    EXPECT_GT(updated[3], 3.74f);
    EXPECT_LT(updated[3], 3.76f);
}

TEST_F(ConfigTest, ImplicitPdeAndFemHelpersAreSane) {
    const float field[] = {0.0f, 1.0f, 0.0f, -1.0f, 0.0f};
    float implicit_step[5] = {};
    EXPECT_TRUE(UpeMath_CrankNicolsonDiffusion1D(field, 5u, 0.1f, 0.5f, 1.0f, implicit_step) != 0);
    EXPECT_GT(implicit_step[1], 0.90f);
    EXPECT_LT(implicit_step[1], 0.91f);
    EXPECT_GT(implicit_step[3], -0.91f);
    EXPECT_LT(implicit_step[3], -0.90f);

    float stiffness[4] = {};
    EXPECT_TRUE(UpeMath_BarElementStiffness1D(100.0f, 2.0f, 5.0f, stiffness) != 0);
    EXPECT_GT(stiffness[0], 39.9f);
    EXPECT_LT(stiffness[0], 40.1f);
    EXPECT_GT(stiffness[1], -40.1f);
    EXPECT_LT(stiffness[1], -39.9f);
    EXPECT_GT(stiffness[2], -40.1f);
    EXPECT_LT(stiffness[2], -39.9f);
    EXPECT_GT(stiffness[3], 39.9f);
    EXPECT_LT(stiffness[3], 40.1f);

    float mass[4] = {};
    EXPECT_TRUE(UpeMath_BarElementMass1D(3.0f, 2.0f, 5.0f, mass) != 0);
    EXPECT_GT(mass[0], 9.9f);
    EXPECT_LT(mass[0], 10.1f);
    EXPECT_GT(mass[1], 4.9f);
    EXPECT_LT(mass[1], 5.1f);
    EXPECT_GT(mass[2], 4.9f);
    EXPECT_LT(mass[2], 5.1f);
    EXPECT_GT(mass[3], 9.9f);
    EXPECT_LT(mass[3], 10.1f);
}

TEST_F(ConfigTest, ReconstructionAndLimiterMathAreSane) {
    float minmod = UpeMath_MinmodLimiter(2.0f, 1.0f);
    EXPECT_GT(minmod, 0.99f);
    EXPECT_LT(minmod, 1.01f);

    float superbee = UpeMath_SuperbeeLimiter(2.0f, 1.0f);
    EXPECT_GT(superbee, 1.99f);
    EXPECT_LT(superbee, 2.01f);

    const float cells[] = {1.0f, 2.0f, 4.0f, 7.0f, 11.0f};
    float left[5] = {};
    float right[5] = {};
    EXPECT_TRUE(UpeMath_MusclReconstruct1D(cells, 5u, left, right) != 0);
    EXPECT_GT(left[2], 2.9f);
    EXPECT_LT(left[2], 3.1f);
    EXPECT_GT(right[2], 4.9f);
    EXPECT_LT(right[2], 5.1f);
}

TEST_F(ConfigTest, StructuralDynamicsAndPlasticityHelpersAreSane) {
    float plastic = UpeMath_PerfectPlasticStress1D(15.0f, 12.0f);
    EXPECT_GT(plastic, 11.9f);
    EXPECT_LT(plastic, 12.1f);

    float drucker_prager = UpeMath_DruckerPragerYieldFunction(2.0f, 5.0f, 0.2f, 3.0f);
    EXPECT_GT(drucker_prager, 2.39f);
    EXPECT_LT(drucker_prager, 2.41f);

    float force_a = 0.0f;
    float force_b = 0.0f;
    EXPECT_TRUE(UpeMath_BarElementInternalForce1D(100.0f, 2.0f, 5.0f, 0.0f, 0.1f, &force_a, &force_b) != 0);
    EXPECT_GT(force_a, -4.1f);
    EXPECT_LT(force_a, -3.9f);
    EXPECT_GT(force_b, 3.9f);
    EXPECT_LT(force_b, 4.1f);

    float displacement = 1.0f;
    float velocity = 0.0f;
    float acceleration = -4.0f;
    EXPECT_TRUE(UpeMath_NewmarkBetaStep1D(1.0f, 0.0f, 4.0f, 0.0f, 0.1f, 0.25f, 0.5f,
                                          &displacement, &velocity, &acceleration) != 0);
    EXPECT_GT(displacement, 0.97f);
    EXPECT_LT(displacement, 0.99f);
    EXPECT_GT(velocity, -0.5f);
    EXPECT_LT(velocity, -0.3f);
    EXPECT_GT(acceleration, -4.1f);
    EXPECT_LT(acceleration, -3.8f);
}

TEST_F(ConfigTest, MultigridAndModalMetricsAreSane) {
    const float rhs[] = {0.0f, 1.0f, 1.0f, 1.0f, 0.0f};
    float solution[5] = {};
    EXPECT_TRUE(UpeMath_PoissonGaussSeidel1D(rhs, 5u, 1.0f, 32u, solution) != 0);
    EXPECT_GT(solution[1], 1.4f);
    EXPECT_LT(solution[1], 1.6f);
    EXPECT_GT(solution[2], 1.9f);
    EXPECT_LT(solution[2], 2.1f);

    float residual[5] = {};
    EXPECT_TRUE(UpeMath_PoissonResidual1D(solution, rhs, 5u, 1.0f, residual) != 0);
    EXPECT_LT(fabsf(residual[1]), 0.05f);
    EXPECT_LT(fabsf(residual[2]), 0.05f);
    EXPECT_LT(fabsf(residual[3]), 0.05f);

    float natural_frequency = UpeMath_NaturalFrequency1D(9.0f, 1.0f);
    EXPECT_GT(natural_frequency, 2.99f);
    EXPECT_LT(natural_frequency, 3.01f);

    float damping_ratio = UpeMath_DampingRatio1D(1.0f, 1.2f, 9.0f);
    EXPECT_GT(damping_ratio, 0.19f);
    EXPECT_LT(damping_ratio, 0.21f);
}

TEST_F(ConfigTest, TwoGridAndStructuralConstitutiveHelpersAreSane) {
    const float rhs[] = {0.0f, 1.0f, 1.0f, 1.0f, 0.0f};
    float two_grid[5] = {};
    EXPECT_TRUE(UpeMath_TwoGridPoissonCorrection1D(rhs, 5u, 1.0f, 8u, 8u, two_grid) != 0);
    EXPECT_GT(two_grid[1], 1.3f);
    EXPECT_LT(two_grid[1], 1.6f);
    EXPECT_GT(two_grid[2], 1.8f);
    EXPECT_LT(two_grid[2], 2.1f);

    float load[2] = {};
    EXPECT_TRUE(UpeMath_BarElementLoad1D(6.0f, 5.0f, load) != 0);
    EXPECT_GT(load[0], 14.9f);
    EXPECT_LT(load[0], 15.1f);
    EXPECT_GT(load[1], 14.9f);
    EXPECT_LT(load[1], 15.1f);

    float kelvin_voigt = UpeMath_KelvinVoigtStress1D(0.02f, 0.5f, 100.0f, 4.0f);
    EXPECT_GT(kelvin_voigt, 3.9f);
    EXPECT_LT(kelvin_voigt, 4.1f);

    float critical_dt = UpeMath_CriticalTimeStepSpringMass(1.0f, 9.0f);
    EXPECT_GT(critical_dt, 0.66f);
    EXPECT_LT(critical_dt, 0.67f);
}

TEST_F(ConfigTest, StabilityDispersionAndAssemblyHelpersAreSane) {
    float courant_wave = UpeMath_CourantWaveStability1D(2.0f, 0.25f, 1.0f);
    EXPECT_GT(courant_wave, 0.49f);
    EXPECT_LT(courant_wave, 0.51f);

    float phase_ratio = UpeMath_PhaseVelocityRatio1D(0.5f, 1.0f);
    EXPECT_GT(phase_ratio, 0.95f);
    EXPECT_LT(phase_ratio, 1.01f);

    float dispersion = UpeMath_NumericalDispersion1D(0.5f, 1.0f);
    EXPECT_GT(dispersion, -0.05f);
    EXPECT_LT(dispersion, 0.05f);

    const float element_stiffness[] = {10.0f, 20.0f};
    float assembled[9] = {};
    EXPECT_TRUE(UpeMath_AssembleBarChainStiffness1D(element_stiffness, 2u, assembled, 3u) != 0);
    EXPECT_GT(assembled[0], 9.9f);
    EXPECT_LT(assembled[0], 10.1f);
    EXPECT_GT(assembled[1], -10.1f);
    EXPECT_LT(assembled[1], -9.9f);
    EXPECT_GT(assembled[4], 29.9f);
    EXPECT_LT(assembled[4], 30.1f);
    EXPECT_GT(assembled[8], 19.9f);
    EXPECT_LT(assembled[8], 20.1f);
}

TEST_F(ConfigTest, CouplingCoordinatorTracksExecutionAndConvergence) {
    UpeCouplingCoordinator coordinator = {};
    UpeCoupling_Init(&coordinator, 4u);

    UpeCoupling_AddInterface(&coordinator, UPE_DOMAIN_RIGID, UPE_DOMAIN_FLUID, UPE_COUPLING_IMPLICIT);
    UpeCoupling_AddInterface(&coordinator, UPE_DOMAIN_FLUID, UPE_DOMAIN_THERMAL, UPE_COUPLING_SEMI_IMPLICIT);

    coordinator.interfaces[0].coupling_strength = 0.8f;
    coordinator.interfaces[0].max_iterations = 6u;
    coordinator.interfaces[0].tolerance = 1.0e-3f;
    coordinator.interfaces[1].coupling_strength = 0.4f;

    UpeCoupling_BuildExecutionOrder(&coordinator);
    EXPECT_EQ(coordinator.num_domains_active, 3u);
    EXPECT_EQ(coordinator.execution_order[0], UPE_DOMAIN_RIGID);
    EXPECT_EQ(coordinator.execution_order[1], UPE_DOMAIN_FLUID);
    EXPECT_EQ(coordinator.execution_order[2], UPE_DOMAIN_THERMAL);

    UpeCoupling_Step(&coordinator, 0.1f);
    EXPECT_GE(coordinator.total_coupling_iterations, 3u);
    EXPECT_LE(coordinator.total_coupling_iterations, 8u);
    EXPECT_GT(coordinator.avg_coupling_residual, 0.0f);
    EXPECT_LT(coordinator.avg_coupling_residual, 0.02f);

    UpeCoupling_Destroy(&coordinator);
    EXPECT_EQ(coordinator.num_interfaces, 0u);
    EXPECT_EQ(coordinator.max_interfaces, 0u);
    EXPECT_GT(coordinator.avg_coupling_residual, -1.0e-6f);
    EXPECT_LT(coordinator.avg_coupling_residual, 1.0e-6f);
}

TEST_F(ConfigTest, ModalStructuralSpectralHelpersAreSane) {
    const float matrix[] = {
        4.0f, 1.0f, 0.0f,
        1.0f, 3.0f, 0.0f,
        0.0f, 0.0f, 2.0f
    };
    float eigenvalue = 0.0f;
    float eigenvector[3] = {};
    EXPECT_TRUE(UpeMath_PowerIterationSymmetric3x3(matrix, 12u, &eigenvalue, eigenvector) != 0);
    EXPECT_GT(eigenvalue, 4.60f);
    EXPECT_LT(eigenvalue, 4.65f);
    EXPECT_GT(fabsf(eigenvector[0]), 0.84f);
    EXPECT_LT(fabsf(eigenvector[0]), 0.90f);
    EXPECT_GT(fabsf(eigenvector[1]), 0.43f);
    EXPECT_LT(fabsf(eigenvector[1]), 0.53f);

    float alpha = 0.0f;
    float beta = 0.0f;
    EXPECT_TRUE(UpeMath_RayleighDampingCoefficients(0.05f, 10.0f, 0.05f, 20.0f, &alpha, &beta) != 0);
    EXPECT_GT(alpha, 0.66f);
    EXPECT_LT(alpha, 0.68f);
    EXPECT_GT(beta, 0.0032f);
    EXPECT_LT(beta, 0.0034f);

    float buckling = UpeMath_EulerBucklingLoadPinnedPinned(200.0f, 0.02f, 2.0f);
    EXPECT_GT(buckling, 9.85f);
    EXPECT_LT(buckling, 9.90f);
}

TEST_F(ConfigTest, BeamAndEigenHelpersAreSane) {
    const float matrix[] = {
        2.0f, 1.0f,
        1.0f, 2.0f
    };
    float spectral_radius = UpeMath_GershgorinSpectralRadius(matrix, 2u);
    EXPECT_GT(spectral_radius, 2.99f);
    EXPECT_LT(spectral_radius, 3.01f);

    float eigenvalue = 0.0f;
    float eigenvector[2] = {};
    EXPECT_TRUE(UpeMath_PowerIterationSymmetric(matrix, 2u, 12u, &eigenvalue, eigenvector) != 0);
    EXPECT_GT(eigenvalue, 2.99f);
    EXPECT_LT(eigenvalue, 3.01f);
    EXPECT_GT(fabsf(eigenvector[0]), 0.69f);
    EXPECT_LT(fabsf(eigenvector[0]), 0.73f);
    EXPECT_GT(fabsf(eigenvector[1]), 0.69f);
    EXPECT_LT(fabsf(eigenvector[1]), 0.73f);

    float damping = UpeMath_RayleighDamping1D(0.1f, 0.02f, 2.0f, 3.0f);
    EXPECT_GT(damping, 0.25f);
    EXPECT_LT(damping, 0.27f);

    float beam[16] = {};
    EXPECT_TRUE(UpeMath_BeamElementStiffnessEulerBernoulli(3.0f, 2.0f, beam) != 0);
    EXPECT_GT(beam[0], 4.49f);
    EXPECT_LT(beam[0], 4.51f);
    EXPECT_GT(beam[1], 4.49f);
    EXPECT_LT(beam[1], 4.51f);
    EXPECT_GT(beam[2], -4.51f);
    EXPECT_LT(beam[2], -4.49f);
    EXPECT_GT(beam[5], 5.99f);
    EXPECT_LT(beam[5], 6.01f);
    EXPECT_GT(beam[7], 2.99f);
    EXPECT_LT(beam[7], 3.01f);
}

TEST_F(ConfigTest, InvariantAuditAndStateEstimationAreSane) {
    const float energy_like_series[] = {10.0f, 9.95f, 9.98f, 9.92f, 9.90f};
    UpeInvariantAuditReport invariant = {};
    EXPECT_TRUE(UpeCert_AnalyzeInvariantSeries(energy_like_series, 5u, 10.0f, 1, &invariant) != 0);
    EXPECT_EQ(invariant.sample_count, 5u);
    EXPECT_GT(invariant.max_abs_drift, 0.09f);
    EXPECT_LT(invariant.max_abs_drift, 0.11f);
    EXPECT_GT(invariant.mean_abs_drift, 0.04f);
    EXPECT_LT(invariant.mean_abs_drift, 0.06f);
    EXPECT_GT(invariant.monotonic_violation_ratio, 0.0f);
    EXPECT_LT(invariant.monotonic_violation_ratio, 0.3f);
    EXPECT_GT(invariant.invariant_score, 0.7f);

    char invariant_json[512] = {};
    const uint32_t invariant_len = UpeCert_WriteInvariantAuditJson(&invariant, invariant_json, sizeof(invariant_json));
    EXPECT_GT(invariant_len, 0u);
    EXPECT_THAT(invariant_json, ::testing::HasSubstr("\"invariant_score\""));

    const float measurements[] = {1.0f, 1.2f, 0.9f, 1.1f, 1.05f};
    UpeStateEstimateReport estimate = {};
    EXPECT_TRUE(UpeCert_RunScalarStateEstimator(measurements, 5u, 0.01f, 0.04f, &estimate) != 0);
    EXPECT_EQ(estimate.sample_count, 5u);
    EXPECT_GT(estimate.final_estimate, 1.0f);
    EXPECT_LT(estimate.final_estimate, 1.1f);
    EXPECT_GT(estimate.final_covariance, 0.0f);
    EXPECT_LT(estimate.final_covariance, 0.05f);
    EXPECT_GT(estimate.innovation_consistency, 0.0f);
    EXPECT_LT(estimate.innovation_consistency, 1.0f);
    EXPECT_GT(estimate.smoothing_gain, 0.0f);
    EXPECT_LT(estimate.smoothing_gain, 1.0f);

    char estimate_json[512] = {};
    const uint32_t estimate_len = UpeCert_WriteStateEstimateJson(&estimate, estimate_json, sizeof(estimate_json));
    EXPECT_GT(estimate_len, 0u);
    EXPECT_THAT(estimate_json, ::testing::HasSubstr("\"final_estimate\""));
}

TEST_F(ConfigTest, LinearSystemControlMetricsAreSane) {
    const float a_matrix[] = {
        -0.4f, 1.0f,
        -2.0f, -0.3f
    };
    const float b_vector[] = {0.0f, 1.0f};
    const float c_vector[] = {1.0f, 0.0f};

    UpeLinearSystem2AuditReport report = {};
    EXPECT_TRUE(UpeCert_AnalyzeLinearSystem2(a_matrix, b_vector, c_vector, &report) != 0);
    EXPECT_LT(report.trace, 0.0f);
    EXPECT_GT(report.determinant, 0.0f);
    EXPECT_GT(report.spectral_radius_bound, 0.0f);
    EXPECT_LT(report.spectral_radius_bound, 2.0f);
    EXPECT_GT(report.damping_indicator, 0.0f);
    EXPECT_TRUE(report.stable_linearized != 0);
    EXPECT_TRUE(report.controllable != 0);
    EXPECT_TRUE(report.observable != 0);
    EXPECT_GT(report.controllability_measure, 0.1f);
    EXPECT_GT(report.observability_measure, 0.1f);

    char json[512] = {};
    const uint32_t json_len = UpeCert_WriteLinearSystemAuditJson(&report, json, sizeof(json));
    EXPECT_GT(json_len, 0u);
    EXPECT_THAT(json, ::testing::HasSubstr("\"stable_linearized\""));
}

TEST_F(ConfigTest, ScalarMpcPlanningIsSane) {
    UpeScalarMpcReport report = {};
    EXPECT_TRUE(UpeCert_PlanScalarMpc(5.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.2f, 2.0f, 10u, &report) != 0);
    EXPECT_EQ(report.horizon_steps, 10u);
    EXPECT_GT(report.feedback_gain, 0.1f);
    EXPECT_LT(report.feedback_gain, 1.0f);
    EXPECT_LT(report.first_control, -1.0f);
    EXPECT_GT(report.first_control, -6.0f);
    EXPECT_LT(fabsf(report.predicted_final_state), 0.2f);
    EXPECT_LT(fabsf(report.terminal_error), 0.2f);
    EXPECT_GT(report.mean_abs_control, 0.1f);
    EXPECT_GT(report.predicted_cost, 0.0f);
    EXPECT_TRUE(report.stabilizing != 0);

    char json[512] = {};
    const uint32_t json_len = UpeCert_WriteScalarMpcJson(&report, json, sizeof(json));
    EXPECT_GT(json_len, 0u);
    EXPECT_THAT(json, ::testing::HasSubstr("\"feedback_gain\""));
}

TEST_F(ConfigTest, ObserverAndTrajectoryAuditsAreSane) {
    const float observer_times[] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f};
    const float measurements[] = {0.0f, 1.1f, 1.9f, 3.05f, 4.0f};

    UpeAlphaBetaTrackerReport tracker = {};
    EXPECT_TRUE(UpeCert_RunAlphaBetaTracker(measurements, observer_times, 5u, 0.65f, 0.2f, &tracker) != 0);
    EXPECT_EQ(tracker.sample_count, 5u);
    EXPECT_GT(tracker.final_position, 3.8f);
    EXPECT_LT(tracker.final_position, 4.1f);
    EXPECT_GT(tracker.final_velocity, 0.8f);
    EXPECT_LT(tracker.final_velocity, 1.2f);
    EXPECT_GT(tracker.mean_abs_residual, 0.0f);
    EXPECT_LT(tracker.mean_abs_residual, 0.35f);
    EXPECT_GT(tracker.tracking_score, 0.6f);
    EXPECT_LT(tracker.tracking_score, 1.01f);

    char tracker_json[512] = {};
    const uint32_t tracker_json_len = UpeCert_WriteAlphaBetaTrackerJson(&tracker, tracker_json, sizeof(tracker_json));
    EXPECT_GT(tracker_json_len, 0u);
    EXPECT_THAT(tracker_json, ::testing::HasSubstr("\"tracking_score\""));

    const float trajectory_times[] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    const float trajectory_positions[] = {0.0f, 0.8f, 1.7f, 2.7f, 3.6f, 4.5f};

    UpeScalarTrajectoryAuditReport trajectory = {};
    EXPECT_TRUE(UpeCert_AnalyzeScalarTrajectory(trajectory_positions, trajectory_times, 6u, &trajectory) != 0);
    EXPECT_EQ(trajectory.sample_count, 6u);
    EXPECT_GT(trajectory.duration, 4.9f);
    EXPECT_LT(trajectory.duration, 5.1f);
    EXPECT_GT(trajectory.displacement, 4.4f);
    EXPECT_LT(trajectory.displacement, 4.6f);
    EXPECT_GT(trajectory.path_length, 4.4f);
    EXPECT_LT(trajectory.path_length, 4.6f);
    EXPECT_GT(trajectory.mean_speed, 0.85f);
    EXPECT_LT(trajectory.mean_speed, 0.95f);
    EXPECT_GT(trajectory.max_speed, 0.95f);
    EXPECT_LT(trajectory.max_speed, 1.05f);
    EXPECT_LT(trajectory.max_acceleration, 0.15f);
    EXPECT_GT(trajectory.smoothness_score, 0.75f);
    EXPECT_GT(trajectory.monotonic_progress_ratio, 0.99f);

    char trajectory_json[512] = {};
    const uint32_t trajectory_json_len = UpeCert_WriteScalarTrajectoryAuditJson(&trajectory, trajectory_json, sizeof(trajectory_json));
    EXPECT_GT(trajectory_json_len, 0u);
    EXPECT_THAT(trajectory_json, ::testing::HasSubstr("\"smoothness_score\""));
}

TEST_F(ConfigTest, TrapezoidalTrajectoryPlanningIsSane) {
    UpeTrapezoidalTrajectoryPlan plan = {};
    EXPECT_TRUE(UpeCert_PlanTrapezoidalTrajectory1D(0.0f, 10.0f, 3.0f, 2.0f, &plan) != 0);
    EXPECT_GT(plan.displacement, 9.9f);
    EXPECT_LT(plan.displacement, 10.1f);
    EXPECT_GT(plan.accel_time, 1.4f);
    EXPECT_LT(plan.accel_time, 1.6f);
    EXPECT_GT(plan.cruise_time, 1.7f);
    EXPECT_LT(plan.cruise_time, 1.9f);
    EXPECT_GT(plan.total_time, 4.7f);
    EXPECT_LT(plan.total_time, 4.9f);
    EXPECT_GT(plan.peak_velocity, 2.9f);
    EXPECT_LT(plan.peak_velocity, 3.1f);
    EXPECT_GT(plan.mean_speed, 2.0f);
    EXPECT_LT(plan.mean_speed, 2.2f);
    EXPECT_GT(plan.feasibility_score, 0.8f);
    EXPECT_TRUE(plan.triangular_profile == 0);

    char json[512] = {};
    const uint32_t json_len = UpeCert_WriteTrapezoidalTrajectoryJson(&plan, json, sizeof(json));
    EXPECT_GT(json_len, 0u);
    EXPECT_THAT(json, ::testing::HasSubstr("\"total_time\""));
}

TEST_F(ConfigTest, ConstantVelocityKalmanIsSane) {
    const float times[] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    const float measurements[] = {0.0f, 0.95f, 2.05f, 3.0f, 4.1f, 4.95f};

    UpeConstantVelocityKalmanReport report = {};
    EXPECT_TRUE(UpeCert_RunConstantVelocityKalman1D(measurements, times, 6u, 0.02f, 0.05f, &report) != 0);
    EXPECT_EQ(report.sample_count, 6u);
    EXPECT_GT(report.final_position, 4.7f);
    EXPECT_LT(report.final_position, 5.1f);
    EXPECT_GT(report.final_velocity, 0.8f);
    EXPECT_LT(report.final_velocity, 1.2f);
    EXPECT_GT(report.final_position_covariance, 0.0f);
    EXPECT_LT(report.final_position_covariance, 0.2f);
    EXPECT_GT(report.final_velocity_covariance, 0.0f);
    EXPECT_LT(report.final_velocity_covariance, 0.5f);
    EXPECT_GT(report.mean_abs_innovation, 0.0f);
    EXPECT_LT(report.mean_abs_innovation, 0.4f);
    EXPECT_GT(report.normalized_innovation_score, 0.5f);
    EXPECT_LT(report.normalized_innovation_score, 1.01f);

    char json[512] = {};
    const uint32_t json_len = UpeCert_WriteConstantVelocityKalmanJson(&report, json, sizeof(json));
    EXPECT_GT(json_len, 0u);
    EXPECT_THAT(json, ::testing::HasSubstr("\"final_velocity\""));
}

TEST_F(ConfigTest, FormerStubbedSystemsNowBehaveSanely) {
    UpeBodyDesc body_desc = {};
    body_desc.type = UPE_BODY_DYNAMIC;
    body_desc.position = {0.0f, 0.0f, 0.0f};
    body_desc.rotation = UpeQuat_Identity();
    body_desc.mass = 1.0f;

    UpeBody body_a = {};
    UpeBody body_b = {};
    UpeBody_Init(&body_a, 1u, &body_desc);
    body_desc.position = {0.0f, 1.0f, 0.0f};
    UpeBody_Init(&body_b, 2u, &body_desc);

    UpeCachedManifold manifold = {};
    manifold.is_valid = 1;
    manifold.point_count = 1u;
    manifold.normal = {0.0f, 1.0f, 0.0f};
    manifold.frame_created = 10u;
    manifold.frame_last_updated = 11u;
    manifold.points[0].is_active = 1;
    manifold.points[0].local_point_a = {0.0f, 0.0f, 0.0f};
    manifold.points[0].local_point_b = {0.01f, 0.0f, 0.0f};
    manifold.points[0].normal_impulse = 1.25f;
    manifold.points[0].tangent_impulse[0] = 0.15f;
    manifold.points[0].tangent_impulse[1] = 0.05f;

    UpeVec3 contacts_a[1] = {{0.0f, 0.0f, 0.0f}};
    UpeVec3 contacts_b[1] = {{0.01f, 0.0f, 0.0f}};
    EXPECT_TRUE(UpeWarmStart_ValidateManifoldReuse(&manifold, contacts_a, contacts_b, 1u, 0.1f) != 0);
    UpeWarmStart_ResetStats(nullptr);
    uint32_t impulses_reused = UpeWarmStart_ApplyManifoldImpulses(nullptr, &manifold, &body_a, &body_b, 0.1f);
    EXPECT_EQ(impulses_reused, 1u);
    EXPECT_GT(fabsf(body_a.velocity.y), 1.0f);
    EXPECT_GT(fabsf(body_b.velocity.y), 1.0f);

    UpeWarmStartStats warm_stats = {};
    UpeWarmStart_GetStats(nullptr, &warm_stats);
    EXPECT_EQ(warm_stats.manifolds_warmed, 1u);
    EXPECT_EQ(warm_stats.total_impulses_reused, 1u);

    UpeUnifiedState unified = {};
    UpeUnified_Init(&unified);
    UpeUnified_AddSolver(&unified, UPE_UNIFIED_SOLVER_CONSTRAINT);
    UpeUnified_AddSolver(&unified, UPE_UNIFIED_SOLVER_SPH);
    UpeUnified_DispatchRigidBodies(&unified, &body_a);
    UpeUnified_DispatchFluids(&unified, contacts_a);
    UpeUnified_Step(&unified, 1.0f / 60.0f);
    EXPECT_EQ(unified.frame, 1u);
    EXPECT_GT(unified.solvers[0].iterations_total + unified.solvers[1].iterations_total, 0u);
    EXPECT_GT(unified.max_position_error + unified.max_velocity_error + unified.max_energy_error, 0.0f);
    UpeUnified_Destroy(&unified);

    UpeGpuComputePipeline *pipeline = UpeGpuComputePipeline_Create(UPE_GPU_BACKEND_COMPUTE_SHADER);
    EXPECT_NE(pipeline, nullptr);
    if (pipeline) {
        float px[] = {0.0f, 1.0f};
        float py[] = {0.0f, 0.0f};
        float pz[] = {0.0f, 0.0f};
        float vx[] = {1.0f, -2.0f};
        float vy[] = {0.0f, 0.5f};
        float vz[] = {0.0f, 0.0f};
        float inv_masses[] = {1.0f, 1.0f};
        UpeGpuBodyBuffer body_buffer = {px, py, pz, vx, vy, vz, inv_masses, 2u};
        EXPECT_TRUE(UpeGpuComputePipeline_ExecuteKernel(pipeline, UPE_GPU_COMPUTE_PARTICLE_UPDATE, 2u, &body_buffer) != 0);
        EXPECT_GT(px[0], 0.0f);
        EXPECT_LT(px[1], 1.0f);

        UpeGpuPipelineMetrics gpu_metrics = {};
        UpeGpuComputePipeline_GetMetrics(pipeline, &gpu_metrics);
        EXPECT_GE(gpu_metrics.total_time_ms, 0.0f);
        UpeGpuComputePipeline_Destroy(pipeline);
    }

    UpeTelemetryBuffer *telemetry = UpeTelemetry_Create(8u);
    EXPECT_NE(telemetry, nullptr);
    if (telemetry) {
        UpeTelemetry_Record(telemetry, 1u, UPE_TELEMETRY_FRAME_TIME_US, 123u);
        char *json = UpeTelemetry_DumpJson(telemetry);
        EXPECT_NE(json, nullptr);
        if (json) {
            EXPECT_THAT(json, ::testing::HasSubstr("\"thread_id\""));
            free(json);
        }
        UpeTelemetry_Destroy(telemetry);
    }

    UpePbdSystem pbd = {};
    UpePbd_Init(&pbd, 8u, 16u);
    pbd.gravity = {0.0f, 0.0f, 0.0f};
    pbd.damping = 1.0f;
    pbd.solver_iterations = 8u;

    const uint32_t p0 = UpePbd_AddParticle(&pbd, {0.0f, 0.0f, 0.0f}, 1.0f, 0.02f);
    const uint32_t p1 = UpePbd_AddParticle(&pbd, {1.0f, 0.0f, 0.0f}, 1.0f, 0.02f);
    const uint32_t p2 = UpePbd_AddParticle(&pbd, {0.0f, 1.0f, 0.0f}, 1.0f, 0.02f);
    const uint32_t p3 = UpePbd_AddParticle(&pbd, {0.0f, 0.0f, 1.0f}, 1.0f, 0.02f);
    EXPECT_NE(UpePbd_AddVolumeConstraint(&pbd, p0, p1, p2, p3, 0.0f), 0xFFFFFFFFu);

    const float initial_volume = TetVolumeFromPoints(pbd.particles[p0].position,
                                                     pbd.particles[p1].position,
                                                     pbd.particles[p2].position,
                                                     pbd.particles[p3].position);
    pbd.particles[p3].position = {0.0f, 0.0f, 0.1f};
    UpePbd_Step(&pbd, 1.0f / 60.0f);
    const float final_volume = TetVolumeFromPoints(pbd.particles[p0].position,
                                                   pbd.particles[p1].position,
                                                   pbd.particles[p2].position,
                                                   pbd.particles[p3].position);
    EXPECT_GT(final_volume, initial_volume * 0.3f);
    EXPECT_GT(final_volume, 0.0f);
    UpePbd_Destroy(&pbd);
}

class DescriptorTest : public ::testing::Test {};

TEST_F(DescriptorTest, WorldDescDefaultAndValidate) {
    UpeWorldDesc desc = UpeWorldDesc_Default();
    EXPECT_GT(desc.fixed_dt, 0.0f);
    EXPECT_LT(desc.fixed_dt, 1.0f);
    EXPECT_GT((int)desc.solver_iterations, 0);
    EXPECT_TRUE(desc.enable_adaptive_runtime != 0);
    EXPECT_EQ(UpeWorldDesc_Validate(&desc), UPE_OK);

    desc.fixed_dt = -1.0f;
    EXPECT_EQ(UpeWorldDesc_Validate(&desc), UPE_ERR_INVALID_ARGUMENT);
}

TEST_F(DescriptorTest, AdaptiveWorldConfigurationAutoTunesSolverLoad) {
    UpeWorldDesc desc = UpeWorldDesc_Default();
    desc.fixed_dt = 1.0f / 60.0f;
    desc.gravity[0] = 0.0f;
    desc.gravity[1] = 0.0f;
    desc.gravity[2] = 0.0f;
    desc.solver_iterations = 2;
    desc.enable_adaptive_runtime = 1;
    desc.adaptive_mode = UPE_ADAPTIVE_STABLE;
    desc.adaptive_min_solver_iterations = 1;
    desc.adaptive_max_solver_iterations = 6;
    desc.integrator_mode = UPE_INTEGRATOR_VELOCITY_VERLET;
    desc.enable_scientific_diagnostics = 1;
    desc.max_expected_linear_speed = 100.0f;
    desc.max_energy_drift_ratio = 1.0f;
    desc.units = UpeUnits_FromScales(0.5f, 2.0f, 1.0f);
    desc.enable_sleeping = 0;

    UpeWorld *world = UpeWorld_Create(&desc);
    EXPECT_NE(world, nullptr);
    if (!world) {
        return;
    }

    UpeAdaptiveRuntimeConfig applied = {};
    UpeWorld_GetAdaptiveRuntimeConfig(world, &applied);
    EXPECT_EQ(applied.mode, UPE_ADAPTIVE_STABLE);
    EXPECT_EQ(applied.max_solver_iterations, 6u);

    UpeScientificRuntimeConfig scientific = {};
    UpeWorld_GetScientificRuntimeConfig(world, &scientific);
    EXPECT_EQ(scientific.integrator_mode, UPE_INTEGRATOR_VELOCITY_VERLET);
    EXPECT_GT(scientific.units.length_scale, 0.0f);

    for (int i = 0; i < 3; ++i) {
        UpeBodyDesc body_desc = {};
        body_desc.type = UPE_BODY_DYNAMIC;
        body_desc.position = {0.0f, 0.0f, 0.0f};
        body_desc.rotation = UpeQuat_Identity();
        body_desc.mass = 1.0f;
        uint32_t body_id = UpeWorld_CreateBody(world, &body_desc);
        EXPECT_NE(body_id, 0xFFFFFFFFu);
        if (body_id == 0xFFFFFFFFu) {
            continue;
        }

        UpeShapeDesc shape_desc = {};
        shape_desc.type = UPE_SHAPE_SPHERE;
        shape_desc.body_id = body_id;
        shape_desc.sphere.center = {0.0f, 0.0f, 0.0f};
        shape_desc.sphere.radius = 0.75f;
        uint32_t shape_id = UpeWorld_CreateShape(world, &shape_desc);
        EXPECT_NE(shape_id, 0xFFFFFFFFu);
    }

    UpeWorld_Step(world);

    UpeWorldStepMetrics metrics = {};
    const int metrics_ok = UpeWorld_GetStepMetrics(world, &metrics);
    EXPECT_TRUE(metrics_ok != 0);
    if (metrics_ok != 0) {
        EXPECT_GT(metrics.contacts, 0u);
        EXPECT_GE(metrics.solver_iterations, 2u);
        EXPECT_LE(metrics.solver_iterations, 6u);
        EXPECT_EQ(metrics.integrator_mode, (uint32_t)UPE_INTEGRATOR_VELOCITY_VERLET);
        EXPECT_GT(metrics.peak_linear_speed, -0.001f);
        EXPECT_GT(metrics.total_kinetic_energy_si, -0.001f);
        EXPECT_GT(metrics.total_energy_si, -1000000.0f);
        EXPECT_GT(metrics.courant_number, -0.001f);
        EXPECT_GT(metrics.froude_number, -0.001f);
        EXPECT_GT(metrics.reynolds_estimate, -0.001f);
        EXPECT_GT(metrics.uncertainty_margin_si, -0.001f);
        EXPECT_GT(metrics.unit_length_scale, 0.0f);
        EXPECT_GT(metrics.unit_mass_scale, 0.0f);
        EXPECT_GT(metrics.unit_time_scale, 0.0f);
    }

    UpeWorldValidationMetrics validation = {};
    const int validation_ok = UpeWorld_GetValidationMetrics(world, &validation);
    EXPECT_TRUE(validation_ok != 0);
    if (validation_ok != 0) {
        EXPECT_GE(validation.plausibility_score, 0.0f);
        EXPECT_LE(validation.plausibility_score, 1.0f);
        EXPECT_GE(validation.conservation_score, 0.0f);
        EXPECT_LE(validation.conservation_score, 1.0f);
        EXPECT_GE(validation.contact_quality_score, 0.0f);
        EXPECT_LE(validation.contact_quality_score, 1.0f);
        EXPECT_GE(validation.timestep_quality_score, 0.0f);
        EXPECT_LE(validation.timestep_quality_score, 1.0f);
        EXPECT_GE(validation.uncertainty_ratio, 0.0f);
    }

    char validation_summary[256] = {};
    const uint32_t summary_len = UpeWorld_FormatValidationSummary(&validation, validation_summary, sizeof(validation_summary));
    EXPECT_GT(summary_len, 0u);
    EXPECT_THAT(validation_summary, ::testing::HasSubstr("plausibility"));
    EXPECT_EQ(metrics.validation_flags, validation.validation_flags);
    EXPECT_GE(metrics.plausibility_score, 0.0f);
    EXPECT_LE(metrics.plausibility_score, 1.0f);
    EXPECT_GE(metrics.conservation_score, 0.0f);
    EXPECT_LE(metrics.conservation_score, 1.0f);

    UpeWorld_Destroy(world);
}

TEST_F(DescriptorTest, CertificationWorldProbesAreSane) {
    UpeWorldDesc desc = UpeWorldDesc_Default();
    desc.fixed_dt = 1.0f / 120.0f;
    desc.gravity[0] = 0.0f;
    desc.gravity[1] = -9.81f;
    desc.gravity[2] = 0.0f;
    desc.enable_adaptive_runtime = 1;
    desc.enable_scientific_diagnostics = 1;

    UpeWorld *world = UpeWorld_Create(&desc);
    EXPECT_NE(world, nullptr);
    if (!world) {
        return;
    }

    UpeBodyDesc body_desc = {};
    body_desc.type = UPE_BODY_DYNAMIC;
    body_desc.position = {0.0f, 2.0f, 0.0f};
    body_desc.rotation = UpeQuat_Identity();
    body_desc.mass = 2.0f;
    uint32_t body_id = UpeWorld_CreateBody(world, &body_desc);
    EXPECT_NE(body_id, 0xFFFFFFFFu);

    UpeWorld_Step(world);

    float total_energy = UpeCert_ComputeTotalEnergy(world);
    float kinetic_energy = UpeCert_ComputeKineticEnergy(world);
    float potential_energy = UpeCert_ComputePotentialEnergy(world);
    EXPECT_GT(total_energy, -1000000.0f);
    EXPECT_GT(kinetic_energy, -0.001f);
    EXPECT_GT(potential_energy, -1000000.0f);

    uint64_t state_hash = UpeCert_HashWorldState(world);
    EXPECT_NE(state_hash, 0u);

    uint64_t frame_hashes[3] = {};
    UpeCert_HashPerFrame(world, frame_hashes, 3u);
    EXPECT_NE(frame_hashes[0], 0u);
    EXPECT_NE(frame_hashes[2], 0u);

    char audit_json[512] = {};
    const uint32_t audit_len = UpeCert_WriteValidationAuditJson(world, audit_json, sizeof(audit_json));
    EXPECT_GT(audit_len, 0u);
    EXPECT_THAT(audit_json, ::testing::HasSubstr("\"plausibility_score\""));
    EXPECT_THAT(audit_json, ::testing::HasSubstr("\"validation_flags\""));

    UpeWorld_Destroy(world);
}

TEST_F(DescriptorTest, SolverDescDefaultAndValidate) {
    UpeSolverDesc solver = UpeSolverDesc_Default();
    EXPECT_EQ(solver.algorithm, UPE_SOLVER_PGS);
    EXPECT_GT((int)solver.iterations, 0);
    EXPECT_EQ(UpeSolverDesc_Validate(&solver), UPE_OK);

    solver.iterations = 0;
    EXPECT_EQ(UpeSolverDesc_Validate(&solver), UPE_ERR_INVALID_ARGUMENT);
}

TEST_F(DescriptorTest, BodyDescExDefaultAndValidate) {
    UpeBodyDesc_Ex body = UpeBodyDesc_ExDefault();
    EXPECT_EQ(body.rotation[3], 1.0f);
    EXPECT_EQ(UpeBodyDesc_ExValidate(&body), UPE_OK);

    body.mass = -1.0f;
    EXPECT_EQ(UpeBodyDesc_ExValidate(&body), UPE_ERR_INVALID_ARGUMENT);
}

TEST_F(DescriptorTest, EnumValuesStable) {
    EXPECT_TRUE((int)UPE_SOLVER_PGS != (int)UPE_SOLVER_XPBD);
    EXPECT_TRUE((int)UPE_BROADPHASE_SAP != (int)UPE_BROADPHASE_BVH);
}

class ErrorTest : public ::testing::Test {
public:
    void SetUp() override {
        UpeError_ClearLastError();
        UpeError_SetSeverityThreshold(UPE_SEVERITY_TRACE);
        UpeError_ResetDiagnosticStats();
    }
};

static int g_diagnostic_callback_count = 0;

static void TestDiagnosticCallback(UpeSeverity severity, const char *module, const char *message) {
    (void)severity;
    (void)module;
    (void)message;
    g_diagnostic_callback_count += 1;
}

TEST_F(ErrorTest, CodeAndSeverityNames) {
    EXPECT_STREQ(UpeError_CodeName(UPE_OK), "OK");
    EXPECT_STREQ(UpeError_CodeName(UPE_ERR_INVALID_ARGUMENT), "INVALID_ARGUMENT");
    EXPECT_STREQ(UpeError_SeverityName(UPE_SEVERITY_ERROR), "ERROR");
}

TEST_F(ErrorTest, RecordAndReadLastError) {
    UpeError_Record(UPE_ERR_INVALID_ARGUMENT,
                    "TestApi",
                    "tests",
                    123,
                    UPE_SEVERITY_ERROR,
                    "invalid value %d",
                    7);

    UpeErrorContext ctx = UpeError_GetLastError();
    EXPECT_EQ(ctx.error_code, UPE_ERR_INVALID_ARGUMENT);
    EXPECT_EQ(ctx.line_number, 123u);
    EXPECT_THAT(ctx.error_message, ::testing::HasSubstr("invalid value"));
}

TEST_F(ErrorTest, DiagnosticStatsAndCallback) {
    g_diagnostic_callback_count = 0;
    UpeError_SetDiagnosticCallback(TestDiagnosticCallback);

    UpeError_Record(UPE_ERR_INVALID_ARGUMENT,
                    "StatsApi",
                    "tests",
                    44,
                    UPE_SEVERITY_ERROR,
                    "stats-message");

    UpeDiagnosticStats stats = UpeError_GetDiagnosticStats();
    EXPECT_EQ(stats.total_errors, 1u);
    EXPECT_EQ(g_diagnostic_callback_count, 1);

    UpeError_SetDiagnosticCallback(nullptr);
}

class TelemetryTest : public ::testing::Test {};

TEST_F(TelemetryTest, CreateRecordAndStats) {
    UpeTelemetryBuffer *buffer = UpeTelemetry_Create(256);
    EXPECT_NE(buffer, nullptr);

    UpeTelemetry_Record(buffer, 0, UPE_TELEMETRY_SOLVER_ITERATIONS, 4);
    UpeTelemetry_Record(buffer, 1, UPE_TELEMETRY_SOLVER_ITERATIONS, 5);
    UpeTelemetry_Record(buffer, 2, UPE_TELEMETRY_SOLVER_ITERATIONS, 3);

    EXPECT_EQ(UpeTelemetry_GetEntryCount(buffer), 3u);

    UpeTelemetryStats stats = UpeTelemetry_GetStats(buffer, UPE_TELEMETRY_SOLVER_ITERATIONS);
    EXPECT_EQ(stats.count, 3u);
    EXPECT_EQ(stats.min_value, 3u);
    EXPECT_EQ(stats.max_value, 5u);
    EXPECT_EQ(stats.sum_value, 12u);

    UpeTelemetry_Destroy(buffer);
}

TEST_F(TelemetryTest, DumpJsonHasExpectedFields) {
    UpeTelemetryBuffer *buffer = UpeTelemetry_Create(64);
    EXPECT_NE(buffer, nullptr);

    UpeTelemetry_Record(buffer, 0, UPE_TELEMETRY_BODIES_AWAKE, 10);
    char *json = UpeTelemetry_DumpJson(buffer);
    EXPECT_NE(json, nullptr);
    EXPECT_THAT(json, ::testing::HasSubstr("telemetry"));
    EXPECT_THAT(json, ::testing::HasSubstr("bodies_awake"));

    free(json);
    UpeTelemetry_Destroy(buffer);
}
