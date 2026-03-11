#ifndef UPE_RELEASE_SYSTEM_H
#define UPE_RELEASE_SYSTEM_H

#include <stdint.h>

// ============================================================================
// RELEASE SYSTEM HEADER
//
// - Semantic versioning (MAJOR.MINOR.PATCH)
// - Build profiles (Debug/Release/Shipped)
// - Platform compatibility matrix
// - Pre-release checklist automation
// - Performance profiling instrumentation
// - Legal & license management
// ============================================================================

typedef enum UpeReleaseType {
    UPE_BUILD_DEBUG = 0,      // Full checks, assertions, debug info
    UPE_BUILD_RELEASE = 1,    // Optimized, determinism verified
    UPE_BUILD_SHIPPED = 2     // Maximum optimization, minimal overhead
} UpeReleaseType;

typedef enum UpeReleaseCheckResult {
    UPE_CHECK_PASSED = 0,
    UPE_CHECK_FAILED = 1,
    UPE_CHECK_PARTIAL = 2,
    UPE_CHECK_INCONCLUSIVE = 3
} UpeReleaseCheckResult;

typedef struct UpeReleaseConfig {
    uint32_t version_major;
    uint32_t version_minor;
    uint32_t version_patch;
    
    UpeReleaseType build_type;
    uint32_t optimization_level;  // 0-3
    uint32_t is_published;
    uint32_t determinism_verified;
    uint32_t determinism_check;
    uint32_t enable_assertions;
} UpeReleaseConfig;

typedef struct UpeReleasePlatformCheck {
    char platform_name[64];
    uint32_t passed;
    uint32_t skipped;
    uint64_t verified_at;
    char verification_details[256];
} UpeReleasePlatformCheck;

typedef struct UpeReleaseChecklist {
    uint32_t all_phases_complete;
    uint32_t zero_critical_bugs;
    uint32_t less_than_10_p1_bugs;
    uint32_t documentation_complete;
    uint32_t examples_polished;
    uint32_t performance_targets_met;
    uint32_t determinism_certified;
    uint32_t legal_reviewed;
} UpeReleaseChecklist;

// Version management
void UpeRelease_SetVersion(uint32_t major, uint32_t minor, uint32_t patch);
void UpeRelease_GetVersion(uint32_t *major, uint32_t *minor, uint32_t *patch);
void UpeRelease_GetVersionString(char *buffer, uint32_t buffer_size);
uint32_t UpeRelease_IsVersionCompatible(uint32_t major, uint32_t minor);

// Build configuration
void UpeRelease_SetBuildType(UpeReleaseType type);
UpeReleaseType UpeRelease_GetBuildType(void);

// Platform verification
UpeReleaseCheckResult UpeRelease_CheckPlatforms(
    UpeReleasePlatformCheck *checks, uint32_t num_checks);
void UpeRelease_RecordPlatformCheck(
    UpeReleasePlatformCheck *check,
    const char *platform, uint32_t passed, const char *message);

// Pre-release checklist
void UpeRelease_InitChecklist(UpeReleaseChecklist *checklist);
uint32_t UpeRelease_VerifyChecklist(const UpeReleaseChecklist *checklist);
uint32_t UpeRelease_GetChecklistProgress(const UpeReleaseChecklist *checklist);

// Performance profiling
uint32_t UpeRelease_ProfileBegin(const char *marker_name);
void UpeRelease_ProfileEnd(uint32_t marker_id);
void UpeRelease_PrintProfileReport(void);

// ABI & API stability
const char* UpeRelease_GetABIVersion(void);
void UpeRelease_CheckAPIStability(const char *header_path);

// Legal & license
const char* UpeRelease_GetLicenseText(void);
void UpeRelease_PrintLegalInfo(void);

#endif // UPE_RELEASE_SYSTEM_H
