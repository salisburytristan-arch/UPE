#include "upe/upe_config.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/**
 * Build-time profile selector
 * Normally set via CMake, defaults to UPE_PROFILE_GAME
 */
#ifndef UPE_PROFILE
#define UPE_PROFILE UPE_PROFILE_GAME
#endif

/**
 * Build-time determinism mode
 * Set via CMake: UPE_EXPERIMENTAL_Q48_16=ON enables fixed-point
 */
#ifndef UPE_DETERMINISM_MODE
#ifdef UPE_EXPERIMENTAL_Q48_16
#define UPE_DETERMINISM_MODE UPE_DETERMINISM_FIXED_POINT
#else
#define UPE_DETERMINISM_MODE UPE_DETERMINISM_DISABLED
#endif
#endif

/**
 * Version constant: must match CMakeLists.txt project(Upe VERSION ...)
 */
static const UpeVersion kUpeEngineVersion = {1, 0, 0, 0};

/**
 * Build configuration string
 */
static const char kUpeBuildConfig[] =
#if UPE_PROFILE == UPE_PROFILE_GAME
    "Profile=Game; "
#elif UPE_PROFILE == UPE_PROFILE_CINEMATIC
    "Profile=Cinematic; "
#elif UPE_PROFILE == UPE_PROFILE_SCIENTIFIC
    "Profile=Scientific; "
#endif
#if UPE_DETERMINISM_MODE == UPE_DETERMINISM_FIXED_POINT
    "Determinism=FixedPoint(Q48.16); "
#elif UPE_DETERMINISM_MODE == UPE_DETERMINISM_COMPATIBLE
    "Determinism=Compatible; "
#else
    "Determinism=Disabled; "
#endif
#ifdef __clang__
    "Compiler=Clang; "
#elif defined(_MSC_VER)
    "Compiler=MSVC; "
#elif defined(__GNUC__)
    "Compiler=GCC; "
#endif
#ifdef NDEBUG
    "Optimization=Release; "
#else
    "Optimization=Debug; "
#endif
    "More info: see engine logs";

UpeCapabilities UpeEngine_GetCapabilities(void) {
    UpeCapabilities caps = {};
    caps.version = kUpeEngineVersion;
    caps.capability_bits = 0;
    caps.simd_width = 1;
    caps.max_bodies = 65536;
    caps.max_shapes = 262144;
    caps.max_contacts = 1048576;
    caps.max_constraints = 8388608;
    caps.threading_model = 0;
    caps.determinism_mode = (UpeDeterminismMode)UPE_DETERMINISM_MODE;
    caps.profile = (UpeProfile)UPE_PROFILE;

    /* Populate capability bits based on compile-time flags */
    caps.capability_bits |= UPE_CAP_SCALAR_MATH;
    
#ifdef UPE_EXPERIMENTAL_Q48_16
    caps.capability_bits |= UPE_CAP_FIXED_POINT_Q48_16;
    caps.capability_bits |= UPE_CAP_REPLAY_DETERMINISM;
#endif

    caps.capability_bits |= UPE_CAP_BROADPHASE_SAP;
    caps.capability_bits |= UPE_CAP_NARROWPHASE_GJK;
    caps.capability_bits |= UPE_CAP_NARROWPHASE_SCALAR;
    caps.capability_bits |= UPE_CAP_CONSTRAINTS_JACOBIAN;
    caps.capability_bits |= UPE_CAP_FLUIDS_SPH;
    caps.capability_bits |= UPE_CAP_CLOTH_XPBD;
    caps.capability_bits |= UPE_CAP_VEHICLES;
    caps.capability_bits |= UPE_CAP_CHARACTER_CONTROLLER;
    caps.capability_bits |= UPE_CAP_2D_MODE;
    caps.capability_bits |= UPE_CAP_CCD;
    caps.capability_bits |= UPE_CAP_MANIFOLD_CACHE;
    caps.capability_bits |= UPE_CAP_FIELD_FORCES;
    caps.capability_bits |= UPE_CAP_SCIENCE_CATALOG;
    caps.capability_bits |= UPE_CAP_SCIENCE_TOPICS;

#ifdef UPE_ENABLE_SIMD
#if defined(__AVX512F__)
    caps.simd_width = 16;
#elif defined(__AVX2__)
    caps.simd_width = 8;
#elif defined(__SSE4_1__) || defined(__SSE2__)
    caps.simd_width = 4;
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
    caps.simd_width = 4;
#else
    caps.simd_width = 1;
#endif
#endif

#ifdef UPE_ENABLE_THREADING
    caps.threading_model = 1;
#endif

    return caps;
}

const char *UpeEngine_GetVersionString(void) {
    static char version_str[32] = {0};
    if (version_str[0] == '\0') {
        snprintf(version_str, sizeof(version_str), "%u.%u.%u",
                 kUpeEngineVersion.major, kUpeEngineVersion.minor, kUpeEngineVersion.patch);
    }
    return version_str;
}

const char *UpeEngine_GetBuildConfig(void) {
    return kUpeBuildConfig;
}
