#include "test_framework.h"
#include "upe/upe_trigger_system.h"
#include "upe/upe_api.h"
#include "upe/upe_world_body_api.h"

TEST(TestTriggerEventOrderDeterministic) {
    UpeTriggerSystem sys = {};
    UpeTriggerSystem_Init(&sys, NULL);

    sys.event_count = 4;

    sys.event_queue[0].frame_number = 2;
    sys.event_queue[0].event_time = 0.0f;
    sys.event_queue[0].trigger_id = 3;

    sys.event_queue[1].frame_number = 1;
    sys.event_queue[1].event_time = 0.5f;
    sys.event_queue[1].trigger_id = 2;

    sys.event_queue[2].frame_number = 1;
    sys.event_queue[2].event_time = 0.5f;
    sys.event_queue[2].trigger_id = 1;

    sys.event_queue[3].frame_number = 1;
    sys.event_queue[3].event_time = 0.1f;
    sys.event_queue[3].trigger_id = 9;

    UpeTriggerSystem_SortEvents(&sys);

    REQUIRE(sys.event_queue[0].frame_number == 1);
    REQUIRE(sys.event_queue[0].event_time == 0.1f);
    REQUIRE(sys.event_queue[0].trigger_id == 9);

    REQUIRE(sys.event_queue[1].frame_number == 1);
    REQUIRE(sys.event_queue[1].event_time == 0.5f);
    REQUIRE(sys.event_queue[1].trigger_id == 1);

    REQUIRE(sys.event_queue[2].frame_number == 1);
    REQUIRE(sys.event_queue[2].event_time == 0.5f);
    REQUIRE(sys.event_queue[2].trigger_id == 2);

    REQUIRE(sys.event_queue[3].frame_number == 2);
    REQUIRE(sys.event_queue[3].event_time == 0.0f);
    REQUIRE(sys.event_queue[3].trigger_id == 3);

    UpeTriggerSystem_Destroy(&sys);
}

TEST(TestTriggerSubFrameTime) {
    UpeWorldDesc desc = {};
    desc.fixed_dt = 1.0f / 60.0f;
    UpeWorld *world = UpeWorld_Create(&desc);
    REQUIRE(world != nullptr);

    UpeTriggerSystem sys = {};
    UpeTriggerSystem_Init(&sys, NULL);

    uint32_t trigger_id = UpeTrigger_CreateSphere(&sys, {0.0f, 0.0f, 0.0f}, 0.5f, "fast");
    REQUIRE(trigger_id != 0xFFFFFFFFu);

    UpeBodyDesc body_desc = {};
    body_desc.type = UPE_BODY_DYNAMIC;
    body_desc.position = {0.0f, 0.0f, 0.0f};
    body_desc.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    body_desc.mass = 1.0f;
    body_desc.collision_layer = 1u;
    body_desc.collision_mask = 0xFFFFFFFFu;
    uint32_t body_id = UpeWorld_CreateBody(world, &body_desc);
    REQUIRE(body_id != 0xFFFFFFFFu);

    UpeBody *body = UpeWorld_GetBody(world, body_id);
    REQUIRE(body != nullptr);
    body->velocity = {40.0f, 0.0f, 0.0f};

    UpeTrigger_Update(&sys, world);

    REQUIRE(sys.event_count == 1);
    REQUIRE(sys.event_queue[0].event_type == UPE_TRIGGER_ENTER);
    REQUIRE(sys.event_queue[0].event_time > 0.0f);
    REQUIRE(sys.event_queue[0].event_time < desc.fixed_dt + 1.0e-4f);

    UpeTriggerSystem_Destroy(&sys);
    UpeWorld_Destroy(world);
}
