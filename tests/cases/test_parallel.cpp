#include "test_framework.h"
#include "../../core/src/tools/upe_parallel.h"

static void TestTaskFunc(void* data, uint32_t start, uint32_t count) {
    int* counter = (int*)data;
    for (uint32_t i = start; i < start + count; i++) {
        (*counter)++;
    }
}

TEST(TestParallel_TaskGraph) {
    UpeTaskGraph* graph = UpeTaskGraph_Create(10, 1);
    
    int counter = 0;
    uint32_t task_id = UpeTaskGraph_AddTask(graph, TestTaskFunc, &counter, 0, 5);
    REQUIRE(task_id == 0);
    
    UpeTaskGraph_Execute(graph);
    REQUIRE(counter == 5);
    
    UpeTaskGraph_Destroy(graph);
}

TEST(TestParallel_Dependencies) {
    UpeTaskGraph* graph = UpeTaskGraph_Create(10, 1);
    
    int counter = 0;
    uint32_t task_a = UpeTaskGraph_AddTask(graph, TestTaskFunc, &counter, 0, 10);
    uint32_t task_b = UpeTaskGraph_AddTask(graph, TestTaskFunc, &counter, 0, 20);
    
    // task_b depends on task_a
    UpeTaskGraph_AddDependency(graph, task_b, task_a);
    
    UpeTaskGraph_Execute(graph);
    REQUIRE(counter == 30);
    
    UpeTaskGraph_Destroy(graph);
}
