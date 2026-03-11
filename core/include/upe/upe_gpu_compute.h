#pragma once

#include <stdint.h>
#include "upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* GPU Compute Pipeline
 * 
 * Architecture ready for GPU acceleration:
 * - GPU-friendly data layouts (structure-of-arrays instead of array-of-structs)
 * - Compute shader abstractions for different constraint solvers
 * - Staged data transfer (CPU <-> GPU)
 * - Fallback to CPU when GPU unavailable
 * - Query system for GPU availability
 */

typedef enum {
    UPE_GPU_BACKEND_NONE = 0,
    UPE_GPU_BACKEND_COMPUTE_SHADER = 1,  /* DirectCompute / OpenGL compute */
    UPE_GPU_BACKEND_CUDA = 2,            /* NVIDIA CUDA */
    UPE_GPU_BACKEND_HIP = 3,             /* AMD HIP */
} UpeGpuBackend;

typedef enum {
    UPE_GPU_COMPUTE_BROADPHASE = 0,
    UPE_GPU_COMPUTE_NARROWPHASE = 1,
    UPE_GPU_COMPUTE_CONSTRAINT_SOLVE = 2,
    UPE_GPU_COMPUTE_PARTICLE_UPDATE = 3,
} UpeGpuComputeTask;

typedef struct {
    UpeGpuBackend backend;
    uint32_t max_threads_per_block;
    uint64_t device_memory_available;
    uint64_t device_memory_used;
    int supports_atomics;
    int supports_shared_memory;
} UpeGpuDeviceInfo;

typedef struct UpeGpuComputePipeline UpeGpuComputePipeline;

/* Initialize GPU pipeline (returns nullptr if GPU not available) */
UpeGpuComputePipeline *UpeGpuComputePipeline_Create(UpeGpuBackend backend);
void UpeGpuComputePipeline_Destroy(UpeGpuComputePipeline *pipeline);

/* Query GPU availability and properties */
int UpeGpuComputePipeline_IsAvailable(UpeGpuComputePipeline *pipeline);
void UpeGpuComputePipeline_GetDeviceInfo(UpeGpuComputePipeline *pipeline, 
    UpeGpuDeviceInfo *out_info);

/* GPU memory management */
void *UpeGpuComputePipeline_AllocateDeviceMemory(UpeGpuComputePipeline *pipeline, uint64_t size);
void UpeGpuComputePipeline_FreeDeviceMemory(UpeGpuComputePipeline *pipeline, void *device_ptr);

/* Data transfer */
int UpeGpuComputePipeline_UploadData(UpeGpuComputePipeline *pipeline,
    void *host_data, void *device_ptr, uint64_t size);
int UpeGpuComputePipeline_DownloadData(UpeGpuComputePipeline *pipeline,
    void *device_ptr, void *host_data, uint64_t size);

/* GPU-friendly data structures (structure-of-arrays) */
typedef struct {
    float *positions_x;
    float *positions_y;
    float *positions_z;
    float *velocities_x;
    float *velocities_y;
    float *velocities_z;
    float *inv_masses;
    uint32_t count;
} UpeGpuBodyBuffer;

typedef struct {
    uint32_t *body_a_indices;
    uint32_t *body_b_indices;
    float *contact_points_x;
    float *contact_points_y;
    float *contact_points_z;
    float *normals_x;
    float *normals_y;
    float *normals_z;
    float *penetrations;
    float *friction;
    uint32_t count;
} UpeGpuConstraintBuffer;

/* Compute shader execution */
typedef int (*UpeGpuComputeKernel)(UpeGpuComputePipeline *pipeline, 
    uint32_t num_threads, void *kernel_params);

int UpeGpuComputePipeline_ExecuteKernel(UpeGpuComputePipeline *pipeline,
    UpeGpuComputeTask task, uint32_t num_threads, void *kernel_params);

/* Performance profiling */
typedef struct {
    float upload_time_ms;
    float compute_time_ms;
    float download_time_ms;
    float total_time_ms;
    int gpu_utilization_percent;
} UpeGpuPipelineMetrics;

void UpeGpuComputePipeline_GetMetrics(UpeGpuComputePipeline *pipeline,
    UpeGpuPipelineMetrics *out_metrics);

/* Force fallback to CPU (for debugging) */
void UpeGpuComputePipeline_SetForceCpu(UpeGpuComputePipeline *pipeline, int force);

#ifdef __cplusplus
}
#endif
