#include "test_framework.h"
#include "upe/upe_task.h"

struct TaskCounter {
    int value;
};

static void IncrementTask(void *user) {
    TaskCounter *counter = (TaskCounter *)user;
    counter->value += 1;
}

TEST(TestTaskSystemRun) {
    UpeTaskSystemDesc desc = {};
    desc.max_tasks = 4;

    UpeTaskSystem *system = UpeTaskSystem_Create(&desc);
    REQUIRE(system != nullptr);

    TaskCounter counter = {};
    REQUIRE(UpeTaskSystem_Enqueue(system, IncrementTask, &counter));
    REQUIRE(UpeTaskSystem_Enqueue(system, IncrementTask, &counter));

    UpeTaskSystem_Run(system);
    REQUIRE(counter.value == 2);
    REQUIRE(UpeTaskSystem_Pending(system) == 0);

    UpeTaskSystem_Destroy(system);
}
