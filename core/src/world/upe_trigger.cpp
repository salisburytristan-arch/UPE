#include "upe/upe_trigger.h"

void UpeTrigger_Init(UpeTrigger *trigger, uint32_t body_id, uint32_t layer) {
    if (!trigger) {
        return;
    }
    trigger->body_id = body_id;
    trigger->layer = layer;
    trigger->active = 1;
}

int UpeTrigger_ShouldReport(const UpeTrigger *trigger, uint32_t other_layer) {
    if (!trigger || !trigger->active) {
        return 0;
    }
    return (trigger->layer & other_layer) != 0;
}
