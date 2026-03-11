#ifndef UPE_ANIMATION_STATE_MACHINE_H
#define UPE_ANIMATION_STATE_MACHINE_H

#include "upe_animation.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*================================================
  ANIMATION STATE MACHINE (ASM)
  
  Sophisticated hierarchical state machine for
  character control. Features:
  - State transition graphs
  - Blending between animation states
  - Parameter-driven transitions (speed, direction, etc.)
  - Transition conditions and callbacks
  - Hierarchical states (sub-machines)
  - Smooth multi-parameter blending
================================================*/

/* Maximum states and transitions per machine */
#define UPE_MAX_ASM_STATES 32
#define UPE_MAX_ASM_TRANSITIONS 64
#define UPE_MAX_ASM_PARAMETERS 16

/* ASM parameter types */
typedef enum {
    UPE_ASM_PARAM_FLOAT = 0,     /* Continuous value (speed, angle) */
    UPE_ASM_PARAM_INT = 1,       /* Integer (step count) */
    UPE_ASM_PARAM_BOOL = 2       /* Boolean flag (jumping, falling) */
} UpeASMParameterType;

/* Transition condition types */
typedef enum {
    UPE_TRANS_COND_GREATER = 0,  /* param > threshold */
    UPE_TRANS_COND_LESS = 1,     /* param < threshold */
    UPE_TRANS_COND_EQUAL = 2,    /* param == threshold */
    UPE_TRANS_COND_TRUE = 3,     /* bool param is true */
    UPE_TRANS_COND_FALSE = 4,    /* bool param is false */
    UPE_TRANS_COND_ANY = 5       /* Any value (always true) */
} UpeASMTransitionCondition;

/* Parameter storage */
typedef struct {
    uint32_t param_id;
    char name[64];
    UpeASMParameterType type;
    
    /* Value storage */
    float value_float;
    int32_t value_int;
    bool value_bool;
} UpeASMParameter;

/* Transition condition */
typedef struct {
    uint32_t parameter_id;
    UpeASMTransitionCondition condition;
    float threshold;
} UpeASMTransitionCond;

/* State transition */
typedef struct {
    uint32_t transition_id;
    uint32_t from_state_id;
    uint32_t to_state_id;
    UpeASMTransitionCond conditions[4];  /* Up to 4 AND conditions */
    uint32_t condition_count;
    float blend_duration;                 /* Blend time when transitioning */
    bool can_interrupt;                   /* Can transition interrupt current blend */
} UpeASMTransition;

/* Animation state */
typedef struct {
    uint32_t state_id;
    char name[64];
    UpeAnimationClip *animation;
    bool loop;
    float speed_multiplier;               /* Animation playback speed */
    
    /* Entry/exit callbacks (for events) */
    uint32_t on_enter_event_id;
    uint32_t on_exit_event_id;
    
    /* Sub-state machine (for hierarchical animation) */
    struct UpeAnimationStateMachine *sub_machine;
} UpeASMState;

/* Animation state machine */
typedef struct UpeAnimationStateMachine {
    uint32_t machine_id;
    char name[64];
    
    /* States and transitions */
    UpeASMState states[UPE_MAX_ASM_STATES];
    uint32_t state_count;
    uint32_t current_state_id;
    uint32_t next_state_id;
    
    UpeASMTransition transitions[UPE_MAX_ASM_TRANSITIONS];
    uint32_t transition_count;
    
    /* Runtime state */
    float current_blend_time;
    float current_blend_duration;
    bool is_transitioning;
    
    /* Parameters (controls animation) */
    UpeASMParameter parameters[UPE_MAX_ASM_PARAMETERS];
    uint32_t parameter_count;
    
    /* Animation controller */
    UpeAnimationController *controller;
    
    /* Parent machine (for hierarchy) */
    struct UpeAnimationStateMachine *parent_machine;
    uint32_t parent_state_id;
} UpeAnimationStateMachine;

/*================================================
  API FUNCTIONS - MACHINE LIFECYCLE
================================================*/

/* Create and destroy state machine */
UpeAnimationStateMachine* UpeASM_Create(const char *name, UpeAnimationController *controller);
void UpeASM_Destroy(UpeAnimationStateMachine *machine);

/* State management */
uint32_t UpeASM_AddState(UpeAnimationStateMachine *machine, const char *name,
                         UpeAnimationClip *animation, bool loop);
void UpeASM_SetInitialState(UpeAnimationStateMachine *machine, uint32_t state_id);
UpeASMState* UpeASM_GetState(UpeAnimationStateMachine *machine, uint32_t state_id);

/* State configuration */
void UpeASM_SetStateSpeed(UpeAnimationStateMachine *machine, uint32_t state_id, float speed);
void UpeASM_SetStateLoop(UpeAnimationStateMachine *machine, uint32_t state_id, bool loop);

/*================================================
  API FUNCTIONS - TRANSITIONS
================================================*/

/* Transition creation */
uint32_t UpeASM_AddTransition(UpeAnimationStateMachine *machine, uint32_t from_state,
                               uint32_t to_state, float blend_duration);

/* Add transition conditions (all must be TRUE to trigger) */
void UpeASM_SetTransitionCondition(UpeAnimationStateMachine *machine, uint32_t trans_id,
                                   uint32_t param_id, UpeASMTransitionCondition cond,
                                   float threshold);

/* Transition properties */
void UpeASM_SetTransitionCanInterrupt(UpeAnimationStateMachine *machine, uint32_t trans_id,
                                      bool can_interrupt);

/*================================================
  API FUNCTIONS - PARAMETERS
================================================*/

/* Parameter creation and access */
uint32_t UpeASM_AddFloatParameter(UpeAnimationStateMachine *machine, const char *name, float initial);
uint32_t UpeASM_AddIntParameter(UpeAnimationStateMachine *machine, const char *name, int32_t initial);
uint32_t UpeASM_AddBoolParameter(UpeAnimationStateMachine *machine, const char *name, bool initial);

/* Parameter queries */
uint32_t UpeASM_GetParameterID(UpeAnimationStateMachine *machine, const char *name);
float UpeASM_GetFloatParameter(UpeAnimationStateMachine *machine, uint32_t param_id);
int32_t UpeASM_GetIntParameter(UpeAnimationStateMachine *machine, uint32_t param_id);
bool UpeASM_GetBoolParameter(UpeAnimationStateMachine *machine, uint32_t param_id);

/* Parameter updates */
void UpeASM_SetFloatParameter(UpeAnimationStateMachine *machine, uint32_t param_id, float value);
void UpeASM_SetIntParameter(UpeAnimationStateMachine *machine, uint32_t param_id, int32_t value);
void UpeASM_SetBoolParameter(UpeAnimationStateMachine *machine, uint32_t param_id, bool value);

/*================================================
  API FUNCTIONS - EXECUTION
================================================*/

/* Update state machine (evaluate transitions, update animation) */
void UpeASM_Update(UpeAnimationStateMachine *machine, float dt, UpeWorld *physics_world);

/* Query current state */
uint32_t UpeASM_GetCurrentState(UpeAnimationStateMachine *machine);
const char* UpeASM_GetCurrentStateName(UpeAnimationStateMachine *machine);

/* Transition queries */
bool UpeASM_IsTransitioning(UpeAnimationStateMachine *machine);
float UpeASM_GetTransitionProgress(UpeAnimationStateMachine *machine);

/*================================================
  API FUNCTIONS - HIERARCHICAL MACHINES
================================================*/

/* Create sub-state machine (for complex characters) */
uint32_t UpeASM_AddSubMachine(UpeAnimationStateMachine *parent_machine,
                               uint32_t parent_state_id,
                               UpeAnimationStateMachine *sub_machine);

/*================================================
  COMMON ANIMATION PATTERNS (Presets)
================================================*/

/* Create locomotion state machine (idle, walk, run, sprint) */
UpeAnimationStateMachine* UpeASM_CreateLocomotionMachine(UpeAnimationController *controller,
                                                          UpeAnimationClip *idle,
                                                          UpeAnimationClip *walk,
                                                          UpeAnimationClip *run,
                                                          UpeAnimationClip *sprint);

/* Create jump state machine (falling states) */
UpeAnimationStateMachine* UpeASM_CreateJumpMachine(UpeAnimationController *controller,
                                                   UpeAnimationClip *jump_start,
                                                   UpeAnimationClip *falling,
                                                   UpeAnimationClip *landing);

/* Create combat state machine (idle, attack, parry, dodge) */
UpeAnimationStateMachine* UpeASM_CreateCombatMachine(UpeAnimationController *controller,
                                                      UpeAnimationClip *idle,
                                                      UpeAnimationClip *attack,
                                                      UpeAnimationClip *parry,
                                                      UpeAnimationClip *dodge);

#ifdef __cplusplus
}
#endif

#endif /* UPE_ANIMATION_STATE_MACHINE_H */
