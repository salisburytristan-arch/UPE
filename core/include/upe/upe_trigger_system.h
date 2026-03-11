#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Trigger / Sensor System (10.0.0.0)
  
  Non-solid volumes for gameplay (enter/exit/stay) with deterministic event ordering.
  Support "field triggers" (temperature zone, radiation zone, sound zone).
*/

// Trigger types
typedef enum UpeTriggerType {
    UPE_TRIGGER_BOX = 0,
    UPE_TRIGGER_SPHERE = 1,
    UPE_TRIGGER_CAPSULE = 2,
    UPE_TRIGGER_CONVEX = 3,
    UPE_TRIGGER_FIELD = 4        // Field-based (temperature, radiation, etc.)
} UpeTriggerType;

// Trigger events
typedef enum UpeTriggerEvent {
    UPE_TRIGGER_ENTER = 0,
    UPE_TRIGGER_STAY = 1,
    UPE_TRIGGER_EXIT = 2
} UpeTriggerEvent;

// Field trigger conditions
typedef enum UpeFieldCondition {
    UPE_FIELD_ABOVE_THRESHOLD = 0,
    UPE_FIELD_BELOW_THRESHOLD = 1,
    UPE_FIELD_IN_RANGE = 2,
    UPE_FIELD_GRADIENT_HIGH = 3  // High rate of change
} UpeFieldCondition;

// Trigger definition
typedef struct UpeTrigger {
    uint32_t id;
    char name[64];
    
    UpeTriggerType type;
    
    // Geometric trigger parameters
    UpeVec3 position;
    UpeQuat rotation;
    float params[4];             // Dimensions (radius, half-extents, etc.)
    
    // Field trigger parameters (if type == UPE_TRIGGER_FIELD)
    uint32_t field_id;           // Which field to monitor
    UpeFieldCondition condition;
    float threshold_min;
    float threshold_max;
    
    // Filtering
    uint32_t layer_mask;         // Which layers trigger this
    int active;                  // Can be toggled on/off
    
    // Current state
    uint32_t *bodies_inside;     // List of body IDs currently inside
    uint32_t body_count;
    uint32_t body_capacity;
    
    // Callback userdata
    void *user_data;
    
} UpeTrigger;

// Trigger event (for deterministic event queue)
typedef struct UpeTriggerEventData {
    uint32_t trigger_id;
    uint32_t body_id;
    UpeTriggerEvent event_type;
    
    uint32_t frame_number;       // When it happened
    float event_time;            // Sub-frame timing (for CCD)
    
    // Event data
    UpeVec3 contact_point;       // Where body entered/exited
    UpeVec3 body_velocity;
    
    // Field value (for field triggers)
    float field_value;
    
} UpeTriggerEventData;

// Trigger callback
typedef void (*UpeTriggerCallback)(const UpeTriggerEventData *event, void *user_data);

// Trigger system
typedef struct UpeTriggerSystem {
    UpeTrigger *triggers;
    uint32_t trigger_count;
    uint32_t trigger_capacity;
    
    // Event queue (deterministically sorted)
    UpeTriggerEventData *event_queue;
    uint32_t event_count;
    uint32_t event_capacity;
    
    // Callbacks
    UpeTriggerCallback *callbacks;
    uint32_t callback_count;
    
    // Field system reference (for field triggers)
    void *field_system;
    
    // Broadphase for triggers (separate from collision)
    void *trigger_broadphase;
    
} UpeTriggerSystem;

// Initialization
void UpeTriggerSystem_Init(UpeTriggerSystem *sys, void *field_system);
void UpeTriggerSystem_Destroy(UpeTriggerSystem *sys);

// Trigger creation
uint32_t UpeTrigger_CreateBox(UpeTriggerSystem *sys, UpeVec3 center, UpeVec3 half_extents,
                              UpeQuat rotation, const char *name);
uint32_t UpeTrigger_CreateSphere(UpeTriggerSystem *sys, UpeVec3 center, float radius,
                                 const char *name);
uint32_t UpeTrigger_CreateCapsule(UpeTriggerSystem *sys, UpeVec3 point_a, UpeVec3 point_b,
                                  float radius, const char *name);

// Field trigger (monitors field value)
uint32_t UpeTrigger_CreateField(UpeTriggerSystem *sys, uint32_t field_id,
                                UpeFieldCondition condition, float threshold_min,
                                float threshold_max, const char *name);

// Trigger management
void UpeTrigger_Remove(UpeTriggerSystem *sys, uint32_t trigger_id);
void UpeTrigger_SetActive(UpeTriggerSystem *sys, uint32_t trigger_id, int active);
void UpeTrigger_SetLayerMask(UpeTriggerSystem *sys, uint32_t trigger_id, uint32_t layer_mask);

// Callback registration
void UpeTrigger_RegisterCallback(UpeTriggerSystem *sys, UpeTriggerCallback callback);

// Update (called each frame)
void UpeTrigger_Update(UpeTriggerSystem *sys, void *world);

// Explicit testing (for manual queries)
int UpeTrigger_TestPoint(const UpeTrigger *trigger, UpeVec3 point);
int UpeTrigger_TestBody(const UpeTrigger *trigger, uint32_t body_id, void *world);

// Event processing
void UpeTriggerSystem_ProcessEvents(UpeTriggerSystem *sys);
void UpeTriggerSystem_ClearEvents(UpeTriggerSystem *sys);

// Deterministic event sorting (by time, then ID)
void UpeTriggerSystem_SortEvents(UpeTriggerSystem *sys);

// Query
const UpeTrigger* UpeTrigger_Get(const UpeTriggerSystem *sys, uint32_t trigger_id);
uint32_t UpeTrigger_FindByName(const UpeTriggerSystem *sys, const char *name);

// Statistics
void UpeTrigger_PrintStats(const UpeTriggerSystem *sys);

#ifdef __cplusplus
}
#endif
