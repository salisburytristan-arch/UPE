#include "upe/upe_error.h"
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <mutex>

static thread_local UpeErrorContext g_last_error = {};
static std::atomic<UpeDiagnosticCallback> g_diagnostic_callback{nullptr};
static std::atomic<uint64_t> g_error_counts[UPE_SEVERITY_CRITICAL + 1] = {};
static std::atomic<uint64_t> g_total_errors{0};
static std::atomic<uint64_t> g_last_critical_time_us{0};
static std::mutex g_critical_message_mutex;
static char g_last_critical_message[128] = {};
static std::atomic<int> g_global_severity_threshold{(int)UPE_SEVERITY_WARNING};

/* Error code name lookup table */
static const char *kErrorCodeNames[] = {
    "OK",
    "INVALID_ARGUMENT",
    "INVALID_HANDLE",
    "FEATURE_DISABLED",
    "RESOURCE_EXHAUSTED",
    "CONCURRENT_MODIFICATION",
    "VERSION_MISMATCH",
    "UNSUPPORTED_OPERATION",
    "INTERNAL_ALLOC",
    "INTERNAL_STATE",
    "INTERNAL_NAN",
    "INTERNAL_TIMEOUT",
};

static const char *kSeverityNames[] = {
    "TRACE",
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR",
    "CRITICAL",
};

static uint64_t UpeError_NowMicros(void) {
    const auto now = std::chrono::time_point_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now());
    return (uint64_t)now.time_since_epoch().count();
}

static int UpeError_CodeIndex(UpeError error_code) {
    switch (error_code) {
        case UPE_OK: return 0;
        case UPE_ERR_INVALID_ARGUMENT: return 1;
        case UPE_ERR_INVALID_HANDLE: return 2;
        case UPE_ERR_FEATURE_DISABLED: return 3;
        case UPE_ERR_RESOURCE_EXHAUSTED: return 4;
        case UPE_ERR_CONCURRENT_MODIFICATION: return 5;
        case UPE_ERR_VERSION_MISMATCH: return 6;
        case UPE_ERR_UNSUPPORTED_OPERATION: return 7;
        case UPE_ERR_INTERNAL_ALLOC: return 8;
        case UPE_ERR_INTERNAL_STATE: return 9;
        case UPE_ERR_INTERNAL_NAN: return 10;
        case UPE_ERR_INTERNAL_TIMEOUT: return 11;
        default: return -1;
    }
}

UpeErrorContext UpeError_GetLastError(void) {
    return g_last_error;
}

void UpeError_ClearLastError(void) {
    g_last_error = {};
    g_last_error.error_code = UPE_OK;
}

void UpeError_SetDiagnosticCallback(UpeDiagnosticCallback callback) {
    atomic_store(&g_diagnostic_callback, callback);
}

const char *UpeError_CodeName(UpeError error_code) {
    int index = UpeError_CodeIndex(error_code);
    if (index < 0 || index >= (int)(sizeof(kErrorCodeNames) / sizeof(kErrorCodeNames[0]))) {
        return "UNKNOWN";
    }
    return kErrorCodeNames[index];
}

const char *UpeError_SeverityName(UpeSeverity severity) {
    if (severity < 0 || severity > UPE_SEVERITY_CRITICAL) {
        return "UNKNOWN";
    }
    return kSeverityNames[severity];
}

void UpeError_SetSeverityThreshold(UpeSeverity threshold) {
    g_global_severity_threshold.store((int)threshold, std::memory_order_relaxed);
}

UpeSeverity UpeError_GetSeverityThreshold(void) {
    int value = g_global_severity_threshold.load(std::memory_order_relaxed);
    if (value < (int)UPE_SEVERITY_TRACE) {
        value = (int)UPE_SEVERITY_TRACE;
    }
    if (value > (int)UPE_SEVERITY_CRITICAL) {
        value = (int)UPE_SEVERITY_CRITICAL;
    }
    return (UpeSeverity)value;
}

UpeDiagnosticStats UpeError_GetDiagnosticStats(void) {
    UpeDiagnosticStats stats = {};
    for (int i = 0; i <= UPE_SEVERITY_CRITICAL; i++) {
        stats.error_count[i] = g_error_counts[i].load(std::memory_order_relaxed);
    }
    stats.total_errors = g_total_errors.load(std::memory_order_relaxed);
    stats.last_critical_time_us = g_last_critical_time_us.load(std::memory_order_relaxed);

    {
        std::lock_guard<std::mutex> lock(g_critical_message_mutex);
        std::snprintf(stats.last_critical_message,
                      sizeof(stats.last_critical_message),
                      "%s",
                      g_last_critical_message);
    }
    
    return stats;
}

void UpeError_ResetDiagnosticStats(void) {
    for (int i = 0; i <= UPE_SEVERITY_CRITICAL; i++) {
        g_error_counts[i].store(0, std::memory_order_relaxed);
    }
    g_total_errors.store(0, std::memory_order_relaxed);
    g_last_critical_time_us.store(0, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lock(g_critical_message_mutex);
        g_last_critical_message[0] = '\0';
    }
}

/* Internal function: record and log an error (called from API boundary functions) */
void UpeError_Record(UpeError error_code, const char *api_call, const char *module,
                     uint32_t line_number, UpeSeverity severity, const char *format, ...) {
    g_last_error.error_code = error_code;
    g_last_error.api_call = api_call;
    g_last_error.module = module;
    g_last_error.line_number = line_number;
    g_last_error.timestamp_us = (int64_t)UpeError_NowMicros();

    va_list args;
    va_start(args, format);
    vsnprintf(g_last_error.error_message, sizeof(g_last_error.error_message), format, args);
    va_end(args);

    if (severity <= UPE_SEVERITY_CRITICAL) {
        g_error_counts[(int)severity].fetch_add(1, std::memory_order_relaxed);
    }
    g_total_errors.fetch_add(1, std::memory_order_relaxed);

    if (severity == UPE_SEVERITY_CRITICAL) {
        g_last_critical_time_us.store(UpeError_NowMicros(), std::memory_order_relaxed);
        std::lock_guard<std::mutex> lock(g_critical_message_mutex);
        const int max_copy = (int)sizeof(g_last_critical_message) - 1;
        std::snprintf(g_last_critical_message,
                      sizeof(g_last_critical_message),
                      "%.*s",
                      std::max(0, max_copy),
                      g_last_error.error_message);
    }

    if ((int)severity >= g_global_severity_threshold.load(std::memory_order_relaxed)) {
        UpeDiagnosticCallback callback = g_diagnostic_callback.load(std::memory_order_relaxed);
        if (callback != nullptr) {
            char full_message[512];
            snprintf(full_message, sizeof(full_message),
                    "[%s] %s::%s (line %u): %s",
                    UpeError_SeverityName(severity),
                    module ? module : "unknown",
                    api_call ? api_call : "unknown",
                    line_number,
                    g_last_error.error_message);
            callback(severity, module, full_message);
        }
    }
}
