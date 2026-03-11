#include "test_framework.h"
#include "upe/upe_trigger.h"

TEST(TestTriggerInit) {
    UpeTrigger trigger;
    UpeTrigger_Init(&trigger, 42, 0x01);
    REQUIRE(trigger.body_id == 42);
    REQUIRE(trigger.layer == 0x01);
    REQUIRE(trigger.active == 1);
}

TEST(TestTriggerShouldReport) {
    UpeTrigger trigger;
    UpeTrigger_Init(&trigger, 1, 0x02);
    REQUIRE(UpeTrigger_ShouldReport(&trigger, 0x02));
    REQUIRE(!UpeTrigger_ShouldReport(&trigger, 0x04));
}
