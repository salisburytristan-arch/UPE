#include "upe/upe_gpu_compute.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

struct UpeGpuComputePipeline {
    UpeGpuBackend backend;
    int available;
    uint64_t device_memory_available;
    uint64_t device_memory_used;

    UpeGpuPipelineMetrics metrics;
    int force_cpu;

    /* Memory allocation tracking */
    void **allocated_ptrs;
    uint64_t *allocated_sizes;
    uint32_t alloc_count;
    uint32_t alloc_capacity;
};

static int UpeGpuComputePipeline_EnsureCapacity(UpeGpuComputePipeline *pipeline, uint32_t needed) {
    if (!pipeline) {
        return 0;
    }
    if (needed <= pipeline->alloc_capacity) {
        return 1;
    }

    uint32_t new_capacity = pipeline->alloc_capacity ? pipeline->alloc_capacity : 64u;
    while (new_capacity < needed) {
        new_capacity *= 2u;
    }

    void **new_ptrs = (void**)realloc(pipeline->allocated_ptrs, new_capacity * sizeof(void*));
    uint64_t *new_sizes = (uint64_t*)realloc(pipeline->allocated_sizes, new_capacity * sizeof(uint64_t));
    if (!new_ptrs || !new_sizes) {
        free(new_ptrs);
        free(new_sizes);
        return 0;
    }

    pipeline->allocated_ptrs = new_ptrs;
    pipeline->allocated_sizes = new_sizes;
    pipeline->alloc_capacity = new_capacity;
    return 1;
}

UpeGpuComputePipeline *UpeGpuComputePipeline_Create(UpeGpuBackend backend) {
    UpeGpuComputePipeline *pipeline = (UpeGpuComputePipeline*)malloc(sizeof(UpeGpuComputePipeline));
    if (!pipeline) return nullptr;

    memset(pipeline, 0, sizeof(*pipeline));
    pipeline->backend = backend;
    pipeline->available = (backend != UPE_GPU_BACKEND_NONE) ? 1 : 0;
    pipeline->device_memory_available = 1024ull * 1024ull * 1024ull;
    pipeline->force_cpu = (backend == UPE_GPU_BACKEND_NONE) ? 1 : 0;
    pipeline->alloc_capacity = 256u;
    pipeline->allocated_ptrs = (void**)calloc(pipeline->alloc_capacity, sizeof(void*));
    pipeline->allocated_sizes = (uint64_t*)calloc(pipeline->alloc_capacity, sizeof(uint64_t));
    if (!pipeline->allocated_ptrs || !pipeline->allocated_sizes) {
        free(pipeline->allocated_ptrs);
        free(pipeline->allocated_sizes);
        free(pipeline);
        return nullptr;
    }

    return pipeline;
}

void UpeGpuComputePipeline_Destroy(UpeGpuComputePipeline *pipeline) {
    if (!pipeline) return;

    for (uint32_t i = 0; i < pipeline->alloc_count; ++i) {
        free(pipeline->allocated_ptrs[i]);
    }

    free(pipeline->allocated_ptrs);
    free(pipeline->allocated_sizes);
    free(pipeline);
}

int UpeGpuComputePipeline_IsAvailable(UpeGpuComputePipeline *pipeline) {
    if (!pipeline) return 0;
    return pipeline->available && !pipeline->force_cpu;
}

void UpeGpuComputePipeline_GetDeviceInfo(UpeGpuComputePipeline *pipeline,
    UpeGpuDeviceInfo *out_info) {

    if (!pipeline || !out_info) return;

    out_info->backend = pipeline->backend;
    out_info->max_threads_per_block = 1024;
    out_info->device_memory_available = pipeline->device_memory_available;
    out_info->device_memory_used = pipeline->device_memory_used;
    out_info->supports_atomics = 1;
    out_info->supports_shared_memory = 1;
}

void *UpeGpuComputePipeline_AllocateDeviceMemory(UpeGpuComputePipeline *pipeline, uint64_t size) {
    if (!pipeline || size == 0) return nullptr;
    if (!UpeGpuComputePipeline_EnsureCapacity(pipeline, pipeline->alloc_count + 1u)) {
        return nullptr;
    }

    void *ptr = malloc((size_t)size);
    if (!ptr) return nullptr;

    pipeline->allocated_ptrs[pipeline->alloc_count] = ptr;
    pipeline->allocated_sizes[pipeline->alloc_count] = size;
    pipeline->alloc_count++;
    pipeline->device_memory_used += size;
    return ptr;
}

void UpeGpuComputePipeline_FreeDeviceMemory(UpeGpuComputePipeline *pipeline, void *device_ptr) {
    if (!pipeline || !device_ptr) return;

    for (uint32_t i = 0; i < pipeline->alloc_count; ++i) {
        if (pipeline->allocated_ptrs[i] == device_ptr) {
            pipeline->device_memory_used -= pipeline->allocated_sizes[i];
            for (uint32_t j = i; j + 1u < pipeline->alloc_count; ++j) {
                pipeline->allocated_ptrs[j] = pipeline->allocated_ptrs[j + 1u];
                pipeline->allocated_sizes[j] = pipeline->allocated_sizes[j + 1u];
            }
            pipeline->alloc_count--;
            break;
        }
    }

    free(device_ptr);
}

int UpeGpuComputePipeline_UploadData(UpeGpuComputePipeline *pipeline,
    void *host_data, void *device_ptr, uint64_t size) {

    if (!pipeline || !host_data || !device_ptr || size == 0) {
        return 0;
    }

    clock_t start = clock();
    memcpy(device_ptr, host_data, (size_t)size);
    clock_t end = clock();

    pipeline->metrics.upload_time_ms = (float)(end - start) / (float)CLOCKS_PER_SEC * 1000.0f;
    pipeline->metrics.total_time_ms = pipeline->metrics.upload_time_ms + pipeline->metrics.compute_time_ms + pipeline->metrics.download_time_ms;
    return 1;
}

int UpeGpuComputePipeline_DownloadData(UpeGpuComputePipeline *pipeline,
    void *device_ptr, void *host_data, uint64_t size) {

    if (!pipeline || !device_ptr || !host_data || size == 0) {
        return 0;
    }

    clock_t start = clock();
    memcpy(host_data, device_ptr, (size_t)size);
    clock_t end = clock();

    pipeline->metrics.download_time_ms = (float)(end - start) / (float)CLOCKS_PER_SEC * 1000.0f;
    pipeline->metrics.total_time_ms = pipeline->metrics.upload_time_ms + pipeline->metrics.compute_time_ms + pipeline->metrics.download_time_ms;
    return 1;
}

int UpeGpuComputePipeline_ExecuteKernel(UpeGpuComputePipeline *pipeline,
    UpeGpuComputeTask task, uint32_t num_threads, void *kernel_params) {

    if (!pipeline) return 0;

    clock_t start = clock();
    uint32_t effective_threads = (num_threads > 0u) ? num_threads : 1u;

    switch (task) {
        case UPE_GPU_COMPUTE_PARTICLE_UPDATE: {
            UpeGpuBodyBuffer *body_buffer = (UpeGpuBodyBuffer*)kernel_params;
            if (!body_buffer || !body_buffer->positions_x || !body_buffer->velocities_x) {
                return 0;
            }
            uint32_t count = (body_buffer->count < effective_threads) ? body_buffer->count : effective_threads;
            if (count == 0u) {
                count = body_buffer->count;
            }
            const float dt = 1.0f / 60.0f;
            for (uint32_t i = 0; i < count; ++i) {
                body_buffer->positions_x[i] += body_buffer->velocities_x[i] * dt;
                body_buffer->positions_y[i] += body_buffer->velocities_y[i] * dt;
                body_buffer->positions_z[i] += body_buffer->velocities_z[i] * dt;
            }
            pipeline->metrics.gpu_utilization_percent = (int)fminf(100.0f, ((float)count / 1024.0f) * 100.0f);
            break;
        }
        case UPE_GPU_COMPUTE_CONSTRAINT_SOLVE:
        case UPE_GPU_COMPUTE_NARROWPHASE: {
            UpeGpuConstraintBuffer *constraint_buffer = (UpeGpuConstraintBuffer*)kernel_params;
            if (!constraint_buffer || !constraint_buffer->penetrations) {
                return 0;
            }
            uint32_t count = (constraint_buffer->count < effective_threads) ? constraint_buffer->count : effective_threads;
            if (count == 0u) {
                count = constraint_buffer->count;
            }
            for (uint32_t i = 0; i < count; ++i) {
                constraint_buffer->penetrations[i] = fmaxf(0.0f, constraint_buffer->penetrations[i] * 0.5f);
            }
            pipeline->metrics.gpu_utilization_percent = (int)fminf(100.0f, ((float)count / 1024.0f) * 100.0f);
            break;
        }
        case UPE_GPU_COMPUTE_BROADPHASE:
        default:
            pipeline->metrics.gpu_utilization_percent = (int)fminf(100.0f, ((float)effective_threads / 1024.0f) * 100.0f);
            break;
    }

    clock_t end = clock();
    pipeline->metrics.compute_time_ms = (float)(end - start) / (float)CLOCKS_PER_SEC * 1000.0f;
    pipeline->metrics.total_time_ms = pipeline->metrics.upload_time_ms + pipeline->metrics.compute_time_ms + pipeline->metrics.download_time_ms;
    return 1;
}

void UpeGpuComputePipeline_GetMetrics(UpeGpuComputePipeline *pipeline,
    UpeGpuPipelineMetrics *out_metrics) {

    if (!pipeline || !out_metrics) return;
    *out_metrics = pipeline->metrics;
}

void UpeGpuComputePipeline_SetForceCpu(UpeGpuComputePipeline *pipeline, int force) {
    if (!pipeline) return;
    pipeline->force_cpu = force;
}
