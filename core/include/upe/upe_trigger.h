#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeTrigger {
    uint32_t body_id;
    uint32_t layer;
    int active;
} UpeTrigger;

typedef struct UpeTriggerEvent {
    uint32_t trigger_id;
    uint32_t other_id;
    int is_enter;
} UpeTriggerEvent;

void UpeTrigger_Init(UpeTrigger *trigger, uint32_t body_id, uint32_t layer);
int UpeTrigger_ShouldReport(const UpeTrigger *trigger, uint32_t other_layer);

#ifdef __cplusplus
}
#endif
