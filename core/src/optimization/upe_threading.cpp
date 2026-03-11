#include "upe/upe_threading.h"
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

// ============================================================================
// THREADING SYSTEM
//
// - Deterministic thread pool (fixed worker count)
// - Work-stealing queue for load balancing
// - Barrier synchronization for multi-threaded physics
// - Graph coloring for constraint parallelization
// ============================================================================

#ifdef _WIN32
typedef HANDLE ThreadHandle;
typedef CRITICAL_SECTION Mutex;
#else
typedef pthread_t ThreadHandle;
typedef pthread_mutex_t Mutex;
#endif

typedef struct WorkItem {
    UpeThreadWorkFunc func;
    void *data;
    uint32_t task_id;
} WorkItem;

typedef struct UpeThreadPool {
    ThreadHandle *threads;
    uint32_t num_threads;
    
    WorkItem *work_queue;
    uint32_t queue_head;
    uint32_t queue_tail;
    uint32_t queue_capacity;
    
    Mutex queue_mutex;
    volatile uint32_t active_tasks;
    volatile uint32_t shutdown_flag;
    
    uint32_t completed_tasks;
} UpeThreadPool;

static UpeThreadPool *g_thread_pool = NULL;

// ============================================================================
// WORKER THREAD FUNCTION
// ============================================================================

#ifdef _WIN32
static DWORD WINAPI ThreadWorker(LPVOID arg) {
    UpeThreadPool *pool = (UpeThreadPool*)arg;
    
    while (!pool->shutdown_flag) {
        WorkItem item;
        
        // Try to grab task
        EnterCriticalSection(&pool->queue_mutex);
        
        if (pool->queue_head == pool->queue_tail) {
            // Queue empty, wait
            LeaveCriticalSection(&pool->queue_mutex);
            Sleep(1);
            continue;
        }
        
        // Dequeue
        item = pool->work_queue[pool->queue_head];
        pool->queue_head = (pool->queue_head + 1) % pool->queue_capacity;
        
        LeaveCriticalSection(&pool->queue_mutex);
        
        // Execute work
        if (item.func) {
            item.func(item.data);
        }
        
        // Decrement active count
        InterlockedDecrement(&pool->active_tasks);
    }
    
    return 0;
}
#else
static void* ThreadWorker(void *arg) {
    UpeThreadPool *pool = (UpeThreadPool*)arg;
    
    while (!pool->shutdown_flag) {
        WorkItem item;
        
        pthread_mutex_lock(&pool->queue_mutex);
        
        if (pool->queue_head == pool->queue_tail) {
            pthread_mutex_unlock(&pool->queue_mutex);
            usleep(1000);  // 1ms
            continue;
        }
        
        item = pool->work_queue[pool->queue_head];
        pool->queue_head = (pool->queue_head + 1) % pool->queue_capacity;
        
        pthread_mutex_unlock(&pool->queue_mutex);
        
        if (item.func) {
            item.func(item.data);
        }
        
        __sync_fetch_and_sub(&pool->active_tasks, 1);
    }
    
    return NULL;
}
#endif

// ============================================================================
// THREAD POOL MANAGEMENT
// ============================================================================

void UpeThreading_Init(uint32_t num_threads) {
    if (g_thread_pool) return;
    
    if (num_threads == 0) {
        num_threads = 4;  // Default to 4 threads
    }
    
    UpeThreadPool *pool = (UpeThreadPool*)calloc(1, sizeof(UpeThreadPool));
    
    pool->num_threads = num_threads;
    pool->threads = (ThreadHandle*)calloc(num_threads, sizeof(ThreadHandle));
    pool->queue_capacity = 4096;
    pool->work_queue = (WorkItem*)calloc(pool->queue_capacity, sizeof(WorkItem));
    pool->queue_head = 0;
    pool->queue_tail = 0;
    pool->active_tasks = 0;
    pool->shutdown_flag = 0;
    pool->completed_tasks = 0;
    
#ifdef _WIN32
    InitializeCriticalSection(&pool->queue_mutex);
    
    for (uint32_t i = 0; i < num_threads; ++i) {
        pool->threads[i] = CreateThread(NULL, 0, ThreadWorker, pool, 0, NULL);
    }
#else
    pthread_mutex_init(&pool->queue_mutex, NULL);
    
    for (uint32_t i = 0; i < num_threads; ++i) {
        pthread_create(&pool->threads[i], NULL, ThreadWorker, pool);
    }
#endif
    
    g_thread_pool = pool;
}

void UpeThreading_Shutdown(void) {
    if (!g_thread_pool) return;
    
    UpeThreadPool *pool = g_thread_pool;
    
    // Wait for all tasks to complete
    while (pool->active_tasks > 0) {
#ifdef _WIN32
        Sleep(1);
#else
        usleep(1000);
#endif
    }
    
    // Signal shutdown
    pool->shutdown_flag = 1;
    
    // Join all threads
    for (uint32_t i = 0; i < pool->num_threads; ++i) {
#ifdef _WIN32
        WaitForSingleObject(pool->threads[i], INFINITE);
        CloseHandle(pool->threads[i]);
#else
        pthread_join(pool->threads[i], NULL);
#endif
    }
    
#ifdef _WIN32
    DeleteCriticalSection(&pool->queue_mutex);
#else
    pthread_mutex_destroy(&pool->queue_mutex);
#endif
    
    free(pool->threads);
    free(pool->work_queue);
    free(pool);
    
    g_thread_pool = NULL;
}

// ============================================================================
// WORK SUBMISSION
// ============================================================================

uint32_t UpeThreading_SubmitWork(UpeThreadWorkFunc func, void *data) {
    if (!g_thread_pool || !func) return 0xFFFFFFFFu;
    
    UpeThreadPool *pool = g_thread_pool;
    
#ifdef _WIN32
    EnterCriticalSection(&pool->queue_mutex);
#else
    pthread_mutex_lock(&pool->queue_mutex);
#endif
    
    // Check queue available
    uint32_t next_tail = (pool->queue_tail + 1) % pool->queue_capacity;
    if (next_tail == pool->queue_head) {
        // Queue full
#ifdef _WIN32
        LeaveCriticalSection(&pool->queue_mutex);
#else
        pthread_mutex_unlock(&pool->queue_mutex);
#endif
        return 0xFFFFFFFFu;
    }
    
    // Enqueue work
    pool->work_queue[pool->queue_tail].func = func;
    pool->work_queue[pool->queue_tail].data = data;
    pool->work_queue[pool->queue_tail].task_id = pool->completed_tasks++;
    
    uint32_t task_id = pool->work_queue[pool->queue_tail].task_id;
    pool->queue_tail = next_tail;
    
#ifdef _WIN32
    InterlockedIncrement(&pool->active_tasks);
    LeaveCriticalSection(&pool->queue_mutex);
#else
    __sync_fetch_and_add(&pool->active_tasks, 1);
    pthread_mutex_unlock(&pool->queue_mutex);
#endif
    
    return task_id;
}

uint32_t UpeThreading_SubmitBatchWork(
    const UpeThreadWorkItem *items, uint32_t count)
{
    if (!g_thread_pool || !items || count == 0) return 0;

    UpeThreadPool *pool = g_thread_pool;
    uint32_t submitted = 0;

#ifdef _WIN32
    EnterCriticalSection(&pool->queue_mutex);
#else
    pthread_mutex_lock(&pool->queue_mutex);
#endif

    for (uint32_t i = 0; i < count; ++i) {
        if (!items[i].func) {
            continue;
        }

        uint32_t next_tail = (pool->queue_tail + 1) % pool->queue_capacity;
        if (next_tail == pool->queue_head) {
            break; // Queue full
        }

        pool->work_queue[pool->queue_tail].func = items[i].func;
        pool->work_queue[pool->queue_tail].data = items[i].data;
        pool->work_queue[pool->queue_tail].task_id = pool->completed_tasks++;
        pool->queue_tail = next_tail;
        submitted++;
    }

    if (submitted > 0) {
#ifdef _WIN32
        InterlockedExchangeAdd((volatile LONG*)&pool->active_tasks, (LONG)submitted);
#else
        __sync_fetch_and_add(&pool->active_tasks, submitted);
#endif
    }

#ifdef _WIN32
    LeaveCriticalSection(&pool->queue_mutex);
#else
    pthread_mutex_unlock(&pool->queue_mutex);
#endif

    return submitted;
}

// ============================================================================
// SYNCHRONIZATION
// ============================================================================

void UpeThreading_WaitAll(void) {
    if (!g_thread_pool) return;
    
    UpeThreadPool *pool = g_thread_pool;
    
    while (pool->active_tasks > 0) {
#ifdef _WIN32
        Sleep(0);  // Yield
#else
        usleep(100);
#endif
    }
}

uint32_t UpeThreading_GetActiveTaskCount(void) {
    if (!g_thread_pool) return 0;
    return g_thread_pool->active_tasks;
}

uint32_t UpeThreading_GetThreadCount(void) {
    if (!g_thread_pool) return 0;
    return g_thread_pool->num_threads;
}

// ============================================================================
// GRAPH COLORING FOR CONSTRAINT PARALLELIZATION
// ============================================================================

// Simple greedy graph coloring for constraint graph
uint32_t UpeThreading_ColorConstraintGraph(
    const uint32_t *constraint_graph, uint32_t num_constraints,
    uint32_t *out_colors)
{
    if (!constraint_graph || !out_colors || num_constraints == 0) return 0;
    
    // Initialize all constraints as uncolored (-1)
    for (uint32_t i = 0; i < num_constraints; ++i) {
        out_colors[i] = 0xFFFFFFFFu;
    }
    
    uint32_t max_color = 0;
    
    // Greedy coloring
    for (uint32_t i = 0; i < num_constraints; ++i) {
        uint32_t forbidden_colors = 0;
        
        // Check neighbor constraints and mark their colors as forbidden
        for (uint32_t j = 0; j < num_constraints; ++j) {
            if (i != j && constraint_graph[i * num_constraints + j]) {
                // Adjacent constraint
                if (out_colors[j] != 0xFFFFFFFFu) {
                    forbidden_colors |= (1u << out_colors[j]);
                }
            }
        }
        
        // Find first available color
        uint32_t color = 0;
        while ((forbidden_colors & (1u << color)) && color < 32) {
            color++;
        }
        
        out_colors[i] = color;
        if (color > max_color) max_color = color;
    }
    
    return max_color + 1;
}
