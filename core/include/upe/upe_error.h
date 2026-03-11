#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Standardized error codes for all public APIs
 * 0 = success, negative = API client error, positive = engine internal error
 */
typedef enum {
    /* Success */
    UPE_OK = 0,
    
    /* Client errors (negative) */
    UPE_ERR_INVALID_ARGUMENT = -1,       /**< Null pointer, invalid descriptor, out-of-range value */
    UPE_ERR_INVALID_HANDLE = -2,         /**< Stale/invalid body/shape/joint handle */
    UPE_ERR_FEATURE_DISABLED = -3,       /**< Feature not compiled in (check capabilities) */
    UPE_ERR_RESOURCE_EXHAUSTED = -4,     /**< Out of memory for operation */
    UPE_ERR_CONCURRENT_MODIFICATION = -5, /**< Cannot modify world during step */
    UPE_ERR_VERSION_MISMATCH = -6,       /**< ABI or serialization version incompatible */
    UPE_ERR_UNSUPPORTED_OPERATION = -7,  /**< Operation not supported by profile or build */
    
    /* Internal errors (positive) */
    UPE_ERR_INTERNAL_ALLOC = 1,           /**< Internal allocation failed (should not occur) */
    UPE_ERR_INTERNAL_STATE = 2,           /**< Internal state corruption (invariant violated) */
    UPE_ERR_INTERNAL_NAN = 3,             /**< NaN/Inf detected in state (numerical instability) */
    UPE_ERR_INTERNAL_TIMEOUT = 4,         /**< Computation timeout (too many iterations) */
} UpeError;

/**
 * Error context: enriched error information
 * Thread-local storage updated on every API call that errors
 */
typedef struct {
    UpeError error_code;
    uint32_t line_number;
    const char *api_call;      /**< Pointer to static string, do not free */
    const char *module;        /**< Pointer to static string, e.g. "collision/narrowphase" */
    char error_message[256];   /**< Human-readable error message (null-terminated) */
    int64_t timestamp_us;      /**< Microsecond timestamp of error */
} UpeErrorContext;

/**
 * Error severity threshold: determines which errors are logged/reported
 */
typedef enum {
    UPE_SEVERITY_TRACE = 0,    /**< Extremely detailed (only in debug) */
    UPE_SEVERITY_DEBUG = 1,    /**< Debug-level information */
    UPE_SEVERITY_INFO = 2,     /**< Informational (performance notes, stats) */
    UPE_SEVERITY_WARNING = 3,  /**< Non-fatal issues (numerical warnings, slow paths) */
    UPE_SEVERITY_ERROR = 4,    /**< API/logic errors (client caller should fix) */
    UPE_SEVERITY_CRITICAL = 5, /**< Internal fatal issues (engine invariant violated) */
} UpeSeverity;

/**
 * Diagnostic logger callback: user supplies for custom logging
 * Thread-safe; called from any thread
 * message is null-terminated; do not retain pointer
 */
typedef void (*UpeDiagnosticCallback)(UpeSeverity severity, const char *module, const char *message);

/**
 * Get the most recent error context for the calling thread
 * Returns copy; safe to inspect at any time
 */
UpeErrorContext UpeError_GetLastError(void);

/**
 * Clear the error context for the calling thread
 */
void UpeError_ClearLastError(void);

/**
 * Set a custom diagnostic callback for logging
 * If callback is NULL, disables logging
 * Thread-safe; can be changed at runtime
 */
void UpeError_SetDiagnosticCallback(UpeDiagnosticCallback callback);

/**
 * Get human-readable error code name
 * Returns pointer to static string, do not free
 */
const char *UpeError_CodeName(UpeError error_code);

/**
 * Get human-readable severity name
 */
const char *UpeError_SeverityName(UpeSeverity severity);

/**
 * Severity threshold filter: errors below this level are not logged
 * Default is UPE_SEVERITY_WARNING
 * Thread-safe
 */
void UpeError_SetSeverityThreshold(UpeSeverity threshold);
UpeSeverity UpeError_GetSeverityThreshold(void);

/**
 * Diagnostic telemetry: track frequency of errors
 * For performance tuning + stability analysis
 */
typedef struct {
    uint64_t error_count[8];       /**< Histogram of UpeError codes */
    uint64_t total_errors;         /**< Total errors logged since startup */
    uint64_t last_critical_time_us; /**< Timestamp of last critical error */
    char last_critical_message[128]; /**< Last critical error message */
} UpeDiagnosticStats;

/**
 * Get diagnostic statistics (thread-safe snapshot)
 */
UpeDiagnosticStats UpeError_GetDiagnosticStats(void);

/**
 * Reset diagnostic statistics
 */
void UpeError_ResetDiagnosticStats(void);

/**
 * Internal error recording helper used by engine modules.
 */
void UpeError_Record(UpeError error_code,
                     const char *api_call,
                     const char *module,
                     uint32_t line_number,
                     UpeSeverity severity,
                     const char *format,
                     ...);

#ifdef __cplusplus
}
#endif
