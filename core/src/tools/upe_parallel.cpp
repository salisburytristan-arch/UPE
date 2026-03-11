#include "upe_parallel.h"
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <thread>
#include <atomic>
#include <algorithm>

struct UpeTask {
    UpeTaskFunction func;
    void* data;
    uint32_t start;
    uint32_t count;
    uint32_t* dependencies;
    uint32_t dependency_count;
    uint32_t completed;
};

UpeTaskGraph* UpeTaskGraph_Create(uint32_t max_tasks, uint32_t worker_count) {
    UpeTaskGraph* graph = (UpeTaskGraph*)malloc(sizeof(UpeTaskGraph));
    graph->task_capacity = max_tasks;
    graph->task_count = 0;
    graph->tasks = (UpeTask*)malloc(sizeof(UpeTask) * max_tasks);
    graph->worker_count = worker_count;
    graph->deterministic_mode = 1;
    
    for (uint32_t i = 0; i < max_tasks; i++) {
        graph->tasks[i].dependencies = NULL;
        graph->tasks[i].dependency_count = 0;
        graph->tasks[i].completed = 0;
    }
    
    return graph;
}

void UpeTaskGraph_Destroy(UpeTaskGraph* graph) {
    if (!graph) return;
    
    for (uint32_t i = 0; i < graph->task_capacity; i++) {
        if (graph->tasks[i].dependencies) {
            free(graph->tasks[i].dependencies);
        }
    }
    
    free(graph->tasks);
    free(graph);
}

uint32_t UpeTaskGraph_AddTask(UpeTaskGraph* graph, UpeTaskFunction func, 
                               void* data, uint32_t start, uint32_t count) {
    if (graph->task_count >= graph->task_capacity) {
        return UINT32_MAX;
    }
    
    uint32_t task_id = graph->task_count++;
    UpeTask* task = &graph->tasks[task_id];
    task->func = func;
    task->data = data;
    task->start = start;
    task->count = count;
    task->completed = 0;
    
    return task_id;
}

void UpeTaskGraph_AddDependency(UpeTaskGraph* graph, uint32_t task_id, uint32_t depends_on) {
    if (task_id >= graph->task_count || depends_on >= graph->task_count) {
        return;
    }
    
    UpeTask* task = &graph->tasks[task_id];
    
    // Realloc dependencies array
    uint32_t new_count = task->dependency_count + 1;
    task->dependencies = (uint32_t*)realloc(task->dependencies, sizeof(uint32_t) * new_count);
    task->dependencies[task->dependency_count] = depends_on;
    task->dependency_count = new_count;
}

static int UpeTask_CanExecute(UpeTaskGraph* graph, uint32_t task_id) {
    UpeTask* task = &graph->tasks[task_id];
    
    for (uint32_t i = 0; i < task->dependency_count; i++) {
        uint32_t dep_id = task->dependencies[i];
        if (!graph->tasks[dep_id].completed) {
            return 0;
        }
    }
    
    return 1;
}

void UpeTaskGraph_Execute(UpeTaskGraph* graph) {
    if (!graph || graph->task_count == 0) {
        return;
    }

    for (uint32_t i = 0; i < graph->task_count; ++i) {
        graph->tasks[i].completed = 0;
    }

    uint32_t completed_count = 0;

    while (completed_count < graph->task_count) {
        std::vector<uint32_t> ready;
        ready.reserve(graph->task_count - completed_count);

        for (uint32_t i = 0; i < graph->task_count; i++) {
            UpeTask* task = &graph->tasks[i];

            if (task->completed) {
                continue;
            }
            if (!UpeTask_CanExecute(graph, i)) {
                continue;
            }
            ready.push_back(i);
        }

        if (ready.empty()) {
            break;
        }

        std::sort(ready.begin(), ready.end());

        if (graph->worker_count <= 1 || graph->deterministic_mode) {
            for (uint32_t task_id : ready) {
                UpeTask* task = &graph->tasks[task_id];
                task->func(task->data, task->start, task->count);
                task->completed = 1;
                completed_count++;
            }
            continue;
        }

        std::atomic<uint32_t> next_index(0);
        uint32_t worker_count = graph->worker_count;
        if (worker_count > ready.size()) {
            worker_count = (uint32_t)ready.size();
        }

        std::vector<std::thread> workers;
        workers.reserve(worker_count);
        for (uint32_t worker = 0; worker < worker_count; ++worker) {
            workers.emplace_back([&]() {
                for (;;) {
                    uint32_t idx = next_index.fetch_add(1, std::memory_order_relaxed);
                    if (idx >= ready.size()) {
                        break;
                    }
                    UpeTask* task = &graph->tasks[ready[idx]];
                    task->func(task->data, task->start, task->count);
                }
            });
        }

        for (std::thread &worker : workers) {
            worker.join();
        }

        for (uint32_t task_id : ready) {
            if (!graph->tasks[task_id].completed) {
                graph->tasks[task_id].completed = 1;
                completed_count++;
            }
        }
    }
}

void UpeTaskGraph_Reset(UpeTaskGraph* graph) {
    graph->task_count = 0;
    
    for (uint32_t i = 0; i < graph->task_capacity; i++) {
        graph->tasks[i].completed = 0;
        graph->tasks[i].dependency_count = 0;
        if (graph->tasks[i].dependencies) {
            free(graph->tasks[i].dependencies);
            graph->tasks[i].dependencies = NULL;
        }
    }
}
