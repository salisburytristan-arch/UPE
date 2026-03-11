#include "gtest/gtest.h"
#include "upe/upe_config.h"
#include "upe/upe_error.h"
#include "upe/upe_telemetry.h"
#include "upe/upe_descriptor.h"

#include <stdlib.h>

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
}

class DescriptorTest : public ::testing::Test {};

TEST_F(DescriptorTest, WorldDescDefaultAndValidate) {
    UpeWorldDesc desc = UpeWorldDesc_Default();
    EXPECT_GT(desc.fixed_dt, 0.0f);
    EXPECT_LT(desc.fixed_dt, 1.0f);
    EXPECT_GT((int)desc.solver_iterations, 0);
    EXPECT_EQ(UpeWorldDesc_Validate(&desc), UPE_OK);

    desc.fixed_dt = -1.0f;
    EXPECT_EQ(UpeWorldDesc_Validate(&desc), UPE_ERR_INVALID_ARGUMENT);
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
