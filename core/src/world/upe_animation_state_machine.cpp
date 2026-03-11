#include "upe/upe_animation_state_machine.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*================================================
  STATE MACHINE LIFECYCLE
================================================*/

UpeAnimationStateMachine* UpeASM_Create(const char *name, UpeAnimationController *controller) {
    UpeAnimationStateMachine *machine = (UpeAnimationStateMachine*)
        malloc(sizeof(UpeAnimationStateMachine));
    if (!machine) return NULL;
    
    machine->machine_id = 0;
    strncpy(machine->name, name, 63);
    machine->name[63] = '\0';
    machine->controller = controller;
    machine->parent_machine = NULL;
    machine->parent_state_id = UINT32_MAX;
    
    machine->state_count = 0;
    machine->current_state_id = UINT32_MAX;
    machine->next_state_id = UINT32_MAX;
    machine->transition_count = 0;
    machine->parameter_count = 0;
    
    machine->current_blend_time = 0.0f;
    machine->current_blend_duration = 0.0f;
    machine->is_transitioning = false;
    
    memset(machine->states, 0, sizeof(machine->states));
    memset(machine->transitions, 0, sizeof(machine->transitions));
    memset(machine->parameters, 0, sizeof(machine->parameters));
    
    return machine;
}

void UpeASM_Destroy(UpeAnimationStateMachine *machine) {
    if (machine) {
        for (uint32_t i = 0; i < machine->state_count; i++) {
            if (machine->states[i].sub_machine) {
                UpeASM_Destroy(machine->states[i].sub_machine);
            }
        }
        free(machine);
    }
}

/*================================================
  STATE MANAGEMENT
================================================*/

uint32_t UpeASM_AddState(UpeAnimationStateMachine *machine, const char *name,
                         UpeAnimationClip *animation, bool loop) {
    if (!machine || machine->state_count >= UPE_MAX_ASM_STATES) {
        return UINT32_MAX;
    }
    
    uint32_t state_id = machine->state_count;
    UpeASMState *state = &machine->states[state_id];
    
    state->state_id = state_id;
    strncpy(state->name, name, 63);
    state->name[63] = '\0';
    state->animation = animation;
    state->loop = loop;
    state->speed_multiplier = 1.0f;
    state->on_enter_event_id = UINT32_MAX;
    state->on_exit_event_id = UINT32_MAX;
    state->sub_machine = NULL;
    
    machine->state_count++;
    
    /* First state is initial state */
    if (machine->current_state_id == UINT32_MAX) {
        machine->current_state_id = state_id;
    }
    
    return state_id;
}

void UpeASM_SetInitialState(UpeAnimationStateMachine *machine, uint32_t state_id) {
    if (machine && state_id < machine->state_count) {
        machine->current_state_id = state_id;
        machine->next_state_id = UINT32_MAX;
        machine->is_transitioning = false;
    }
}

UpeASMState* UpeASM_GetState(UpeAnimationStateMachine *machine, uint32_t state_id) {
    if (!machine || state_id >= machine->state_count) return NULL;
    return &machine->states[state_id];
}

void UpeASM_SetStateSpeed(UpeAnimationStateMachine *machine, uint32_t state_id, float speed) {
    if (!machine || state_id >= machine->state_count) return;
    machine->states[state_id].speed_multiplier = fmaxf(0.1f, speed);
}

void UpeASM_SetStateLoop(UpeAnimationStateMachine *machine, uint32_t state_id, bool loop) {
    if (!machine || state_id >= machine->state_count) return;
    machine->states[state_id].loop = loop;
}

/*================================================
  TRANSITION MANAGEMENT
================================================*/

uint32_t UpeASM_AddTransition(UpeAnimationStateMachine *machine, uint32_t from_state,
                               uint32_t to_state, float blend_duration) {
    if (!machine || machine->transition_count >= UPE_MAX_ASM_TRANSITIONS ||
        from_state >= machine->state_count || to_state >= machine->state_count) {
        return UINT32_MAX;
    }
    
    uint32_t trans_id = machine->transition_count;
    UpeASMTransition *trans = &machine->transitions[trans_id];
    
    trans->transition_id = trans_id;
    trans->from_state_id = from_state;
    trans->to_state_id = to_state;
    trans->blend_duration = fmaxf(0.0f, blend_duration);
    trans->condition_count = 0;
    trans->can_interrupt = false;
    
    machine->transition_count++;
    return trans_id;
}

void UpeASM_SetTransitionCondition(UpeAnimationStateMachine *machine, uint32_t trans_id,
                                   uint32_t param_id, UpeASMTransitionCondition cond,
                                   float threshold) {
    if (!machine || trans_id >= machine->transition_count || 
        param_id >= machine->parameter_count) {
        return;
    }
    
    UpeASMTransition *trans = &machine->transitions[trans_id];
    if (trans->condition_count >= 4) return;  /* Max 4 conditions per transition */
    
    UpeASMTransitionCond *cond_obj = &trans->conditions[trans->condition_count];
    cond_obj->parameter_id = param_id;
    cond_obj->condition = cond;
    cond_obj->threshold = threshold;
    
    trans->condition_count++;
}

void UpeASM_SetTransitionCanInterrupt(UpeAnimationStateMachine *machine, uint32_t trans_id,
                                      bool can_interrupt) {
    if (!machine || trans_id >= machine->transition_count) return;
    machine->transitions[trans_id].can_interrupt = can_interrupt;
}

/*================================================
  PARAMETER MANAGEMENT
================================================*/

uint32_t UpeASM_AddFloatParameter(UpeAnimationStateMachine *machine, const char *name, float initial) {
    if (!machine || machine->parameter_count >= UPE_MAX_ASM_PARAMETERS) {
        return UINT32_MAX;
    }
    
    uint32_t param_id = machine->parameter_count;
    UpeASMParameter *param = &machine->parameters[param_id];
    
    param->param_id = param_id;
    strncpy(param->name, name, 63);
    param->name[63] = '\0';
    param->type = UPE_ASM_PARAM_FLOAT;
    param->value_float = initial;
    param->value_int = 0;
    param->value_bool = false;
    
    machine->parameter_count++;
    return param_id;
}

uint32_t UpeASM_AddIntParameter(UpeAnimationStateMachine *machine, const char *name, int32_t initial) {
    if (!machine || machine->parameter_count >= UPE_MAX_ASM_PARAMETERS) {
        return UINT32_MAX;
    }
    
    uint32_t param_id = machine->parameter_count;
    UpeASMParameter *param = &machine->parameters[param_id];
    
    param->param_id = param_id;
    strncpy(param->name, name, 63);
    param->name[63] = '\0';
    param->type = UPE_ASM_PARAM_INT;
    param->value_float = 0.0f;
    param->value_int = initial;
    param->value_bool = false;
    
    machine->parameter_count++;
    return param_id;
}

uint32_t UpeASM_AddBoolParameter(UpeAnimationStateMachine *machine, const char *name, bool initial) {
    if (!machine || machine->parameter_count >= UPE_MAX_ASM_PARAMETERS) {
        return UINT32_MAX;
    }
    
    uint32_t param_id = machine->parameter_count;
    UpeASMParameter *param = &machine->parameters[param_id];
    
    param->param_id = param_id;
    strncpy(param->name, name, 63);
    param->name[63] = '\0';
    param->type = UPE_ASM_PARAM_BOOL;
    param->value_float = 0.0f;
    param->value_int = 0;
    param->value_bool = initial;
    
    machine->parameter_count++;
    return param_id;
}

uint32_t UpeASM_GetParameterID(UpeAnimationStateMachine *machine, const char *name) {
    if (!machine || !name) return UINT32_MAX;
    
    for (uint32_t i = 0; i < machine->parameter_count; i++) {
        if (strcmp(machine->parameters[i].name, name) == 0) {
            return i;
        }
    }
    return UINT32_MAX;
}

float UpeASM_GetFloatParameter(UpeAnimationStateMachine *machine, uint32_t param_id) {
    if (!machine || param_id >= machine->parameter_count) return 0.0f;
    return machine->parameters[param_id].value_float;
}

int32_t UpeASM_GetIntParameter(UpeAnimationStateMachine *machine, uint32_t param_id) {
    if (!machine || param_id >= machine->parameter_count) return 0;
    return machine->parameters[param_id].value_int;
}

bool UpeASM_GetBoolParameter(UpeAnimationStateMachine *machine, uint32_t param_id) {
    if (!machine || param_id >= machine->parameter_count) return false;
    return machine->parameters[param_id].value_bool;
}

void UpeASM_SetFloatParameter(UpeAnimationStateMachine *machine, uint32_t param_id, float value) {
    if (machine && param_id < machine->parameter_count) {
        machine->parameters[param_id].value_float = value;
    }
}

void UpeASM_SetIntParameter(UpeAnimationStateMachine *machine, uint32_t param_id, int32_t value) {
    if (machine && param_id < machine->parameter_count) {
        machine->parameters[param_id].value_int = value;
    }
}

void UpeASM_SetBoolParameter(UpeAnimationStateMachine *machine, uint32_t param_id, bool value) {
    if (machine && param_id < machine->parameter_count) {
        machine->parameters[param_id].value_bool = value;
    }
}

/*================================================
  TRANSITION CONDITION EVALUATION
================================================*/

static bool UpeASM_EvaluateCondition(UpeAnimationStateMachine *machine,
                                     const UpeASMTransitionCond *cond) {
    if (!machine || !cond || cond->parameter_id >= machine->parameter_count) {
        return false;
    }
    
    UpeASMParameter *param = &machine->parameters[cond->parameter_id];
    
    switch (cond->condition) {
        case UPE_TRANS_COND_GREATER:
            if (param->type == UPE_ASM_PARAM_FLOAT) {
                return param->value_float > cond->threshold;
            } else if (param->type == UPE_ASM_PARAM_INT) {
                return param->value_int > (int32_t)cond->threshold;
            }
            break;
            
        case UPE_TRANS_COND_LESS:
            if (param->type == UPE_ASM_PARAM_FLOAT) {
                return param->value_float < cond->threshold;
            } else if (param->type == UPE_ASM_PARAM_INT) {
                return param->value_int < (int32_t)cond->threshold;
            }
            break;
            
        case UPE_TRANS_COND_EQUAL:
            if (param->type == UPE_ASM_PARAM_FLOAT) {
                return fabsf(param->value_float - cond->threshold) < 0.001f;
            } else if (param->type == UPE_ASM_PARAM_INT) {
                return param->value_int == (int32_t)cond->threshold;
            }
            break;
            
        case UPE_TRANS_COND_TRUE:
            return param->type == UPE_ASM_PARAM_BOOL && param->value_bool;
            
        case UPE_TRANS_COND_FALSE:
            return param->type == UPE_ASM_PARAM_BOOL && !param->value_bool;
            
        case UPE_TRANS_COND_ANY:
            return true;
            
        default:
            break;
    }
    
    return false;
}

static bool UpeASM_EvaluateTransition(UpeAnimationStateMachine *machine,
                                      const UpeASMTransition *trans) {
    /* All conditions must be true (AND logic) */
    for (uint32_t i = 0; i < trans->condition_count; i++) {
        if (!UpeASM_EvaluateCondition(machine, &trans->conditions[i])) {
            return false;
        }
    }
    return true;
}

/*================================================
  STATE MACHINE EXECUTION
================================================*/

void UpeASM_Update(UpeAnimationStateMachine *machine, float dt, UpeWorld *physics_world) {
    if (!machine || !machine->controller) return;
    
    /* Update animation controller */
    UpeAnimationController_Update(machine->controller, dt, physics_world);
    
    /* Handle active transition blend */
    if (machine->is_transitioning) {
        machine->current_blend_time += dt;
        
        if (machine->current_blend_time >= machine->current_blend_duration) {
            /* Transition complete */
            machine->current_state_id = machine->next_state_id;
            machine->next_state_id = UINT32_MAX;
            machine->is_transitioning = false;
            machine->current_blend_time = 0.0f;
            
            /* Play animation for new state */
            if (machine->current_state_id < machine->state_count) {
                UpeASMState *state = &machine->states[machine->current_state_id];
                if (state->animation) {
                    UpeAnimationController_PlayAnimation(machine->controller,
                                                        state->animation, state->loop);
                    UpeAnimationController_SetPlaybackSpeed(machine->controller,
                                                           state->speed_multiplier);
                }
            }
        }
    }
    
    /* Evaluate transitions from current state */
    if (machine->current_state_id < machine->state_count) {
        for (uint32_t i = 0; i < machine->transition_count; i++) {
            UpeASMTransition *trans = &machine->transitions[i];
            
            /* Check if transition is available from current state */
            if (trans->from_state_id != machine->current_state_id) continue;
            
            /* Check if transitioning and can't interrupt */
            if (machine->is_transitioning && !trans->can_interrupt) continue;
            
            /* Evaluate transition conditions */
            if (UpeASM_EvaluateTransition(machine, trans)) {
                /* Trigger transition */
                machine->next_state_id = trans->to_state_id;
                machine->current_blend_duration = trans->blend_duration;
                machine->current_blend_time = 0.0f;
                machine->is_transitioning = true;
                
                /* Immediately switch if no blend */
                if (trans->blend_duration <= 0.0f) {
                    machine->current_state_id = machine->next_state_id;
                    machine->next_state_id = UINT32_MAX;
                    machine->is_transitioning = false;
                    
                    if (machine->current_state_id < machine->state_count) {
                        UpeASMState *state = &machine->states[machine->current_state_id];
                        if (state->animation) {
                            UpeAnimationController_PlayAnimation(machine->controller,
                                                                state->animation, state->loop);
                            UpeAnimationController_SetPlaybackSpeed(machine->controller,
                                                                   state->speed_multiplier);
                        }
                    }
                } else {
                    /* Start blending to next animation */
                    if (machine->next_state_id < machine->state_count) {
                        UpeASMState *state = &machine->states[machine->next_state_id];
                        if (state->animation) {
                            UpeAnimationController_BlendAnimation(machine->controller,
                                                                 state->animation,
                                                                 trans->blend_duration);
                        }
                    }
                }
                break;  /* Only one transition per frame */
            }
        }
    }
}

/*================================================
  QUERIES
================================================*/

uint32_t UpeASM_GetCurrentState(UpeAnimationStateMachine *machine) {
    if (!machine) return UINT32_MAX;
    return machine->current_state_id;
}

const char* UpeASM_GetCurrentStateName(UpeAnimationStateMachine *machine) {
    if (!machine || machine->current_state_id >= machine->state_count) {
        return "INVALID";
    }
    return machine->states[machine->current_state_id].name;
}

bool UpeASM_IsTransitioning(UpeAnimationStateMachine *machine) {
    if (!machine) return false;
    return machine->is_transitioning;
}

float UpeASM_GetTransitionProgress(UpeAnimationStateMachine *machine) {
    if (!machine || !machine->is_transitioning || machine->current_blend_duration <= 0.0f) {
        return 0.0f;
    }
    return machine->current_blend_time / machine->current_blend_duration;
}

/*================================================
  HIERARCHICAL MACHINES
================================================*/

uint32_t UpeASM_AddSubMachine(UpeAnimationStateMachine *parent_machine,
                               uint32_t parent_state_id,
                               UpeAnimationStateMachine *sub_machine) {
    if (!parent_machine || !sub_machine || parent_state_id >= parent_machine->state_count) {
        return UINT32_MAX;
    }
    
    parent_machine->states[parent_state_id].sub_machine = sub_machine;
    sub_machine->parent_machine = parent_machine;
    sub_machine->parent_state_id = parent_state_id;
    
    return parent_state_id;
}

/*================================================
  ANIMATION PATTERN PRESETS
================================================*/

UpeAnimationStateMachine* UpeASM_CreateLocomotionMachine(UpeAnimationController *controller,
                                                          UpeAnimationClip *idle,
                                                          UpeAnimationClip *walk,
                                                          UpeAnimationClip *run,
                                                          UpeAnimationClip *sprint) {
    if (!controller || !idle || !walk || !run || !sprint) return NULL;
    
    UpeAnimationStateMachine *state_machine = UpeASM_Create("Locomotion", controller);
    if (!state_machine) return NULL;
    
    /* Create states */
    uint32_t idle_state = UpeASM_AddState(state_machine, "Idle", idle, true);
    uint32_t walk_state = UpeASM_AddState(state_machine, "Walk", walk, true);
    uint32_t run_state = UpeASM_AddState(state_machine, "Run", run, true);
    uint32_t sprint_state = UpeASM_AddState(state_machine, "Sprint", sprint, true);
    
    /* Create speed parameter */
    uint32_t speed_param = UpeASM_AddFloatParameter(state_machine, "Speed", 0.0f);
    
    /* Create transitions with speed-based conditions */
    uint32_t trans;
    
    /* Idle -> Walk (speed > 0.1) */
    trans = UpeASM_AddTransition(state_machine, idle_state, walk_state, 0.3f);
    UpeASM_SetTransitionCondition(state_machine, trans, speed_param, UPE_TRANS_COND_GREATER, 0.1f);
    
    /* Walk -> Idle (speed < 0.05) */
    trans = UpeASM_AddTransition(state_machine, walk_state, idle_state, 0.3f);
    UpeASM_SetTransitionCondition(state_machine, trans, speed_param, UPE_TRANS_COND_LESS, 0.05f);
    
    /* Walk -> Run (speed > 2.0) */
    trans = UpeASM_AddTransition(state_machine, walk_state, run_state, 0.2f);
    UpeASM_SetTransitionCondition(state_machine, trans, speed_param, UPE_TRANS_COND_GREATER, 2.0f);
    
    /* Run -> Walk (speed < 1.5) */
    trans = UpeASM_AddTransition(state_machine, run_state, walk_state, 0.2f);
    UpeASM_SetTransitionCondition(state_machine, trans, speed_param, UPE_TRANS_COND_LESS, 1.5f);
    
    /* Run -> Sprint (speed > 5.0) */
    trans = UpeASM_AddTransition(state_machine, run_state, sprint_state, 0.1f);
    UpeASM_SetTransitionCondition(state_machine, trans, speed_param, UPE_TRANS_COND_GREATER, 5.0f);
    
    /* Sprint -> Run (speed < 4.0) */
    trans = UpeASM_AddTransition(state_machine, sprint_state, run_state, 0.15f);
    UpeASM_SetTransitionCondition(state_machine, trans, speed_param, UPE_TRANS_COND_LESS, 4.0f);
    
    /* Set animation speeds */
    UpeASM_SetStateSpeed(state_machine, walk_state, 1.0f);
    UpeASM_SetStateSpeed(state_machine, run_state, 1.5f);
    UpeASM_SetStateSpeed(state_machine, sprint_state, 2.0f);
    
    return state_machine;
}

UpeAnimationStateMachine* UpeASM_CreateJumpMachine(UpeAnimationController *controller,
                                                   UpeAnimationClip *jump_start,
                                                   UpeAnimationClip *falling,
                                                   UpeAnimationClip *landing) {
    if (!controller || !jump_start || !falling || !landing) return NULL;
    
    UpeAnimationStateMachine *state_machine = UpeASM_Create("Jump", controller);
    if (!state_machine) return NULL;
    
    uint32_t grounded_state = UpeASM_AddState(state_machine, "Grounded", jump_start, false);
    uint32_t falling_state = UpeASM_AddState(state_machine, "Falling", falling, true);
    uint32_t landing_state = UpeASM_AddState(state_machine, "Landing", landing, false);
    
    uint32_t is_airborne = UpeASM_AddBoolParameter(state_machine, "IsAirborne", false);
    uint32_t is_grounded = UpeASM_AddBoolParameter(state_machine, "IsGrounded", true);
    
    /* Grounded -> Falling */
    uint32_t trans = UpeASM_AddTransition(state_machine, grounded_state, falling_state, 0.1f);
    UpeASM_SetTransitionCondition(state_machine, trans, is_airborne, UPE_TRANS_COND_TRUE, 0.0f);
    
    /* Falling -> Landing */
    trans = UpeASM_AddTransition(state_machine, falling_state, landing_state, 0.0f);
    UpeASM_SetTransitionCondition(state_machine, trans, is_grounded, UPE_TRANS_COND_TRUE, 0.0f);
    
    /* Landing -> Grounded */
    trans = UpeASM_AddTransition(state_machine, landing_state, grounded_state, 0.2f);
    UpeASM_SetTransitionCondition(state_machine, trans, is_grounded, UPE_TRANS_COND_TRUE, 0.0f);
    UpeASM_SetTransitionCanInterrupt(state_machine, trans, true);
    
    return state_machine;
}

UpeAnimationStateMachine* UpeASM_CreateCombatMachine(UpeAnimationController *controller,
                                                      UpeAnimationClip *idle,
                                                      UpeAnimationClip *attack,
                                                      UpeAnimationClip *parry,
                                                      UpeAnimationClip *dodge) {
    if (!controller || !idle || !attack || !parry || !dodge) return NULL;
    
    UpeAnimationStateMachine *state_machine = UpeASM_Create("Combat", controller);
    if (!state_machine) return NULL;
    
    uint32_t idle_state = UpeASM_AddState(state_machine, "Idle", idle, true);
    uint32_t attack_state = UpeASM_AddState(state_machine, "Attack", attack, false);
    uint32_t parry_state = UpeASM_AddState(state_machine, "Parry", parry, false);
    uint32_t dodge_state = UpeASM_AddState(state_machine, "Dodge", dodge, false);
    
    uint32_t do_attack = UpeASM_AddBoolParameter(state_machine, "DoAttack", false);
    uint32_t do_parry = UpeASM_AddBoolParameter(state_machine, "DoParry", false);
    uint32_t do_dodge = UpeASM_AddBoolParameter(state_machine, "DoDodge", false);
    
    /* Idle transitions (can interrupt into any action) */
    uint32_t trans = UpeASM_AddTransition(state_machine, idle_state, attack_state, 0.1f);
    UpeASM_SetTransitionCondition(state_machine, trans, do_attack, UPE_TRANS_COND_TRUE, 0.0f);
    
    trans = UpeASM_AddTransition(state_machine, idle_state, parry_state, 0.05f);
    UpeASM_SetTransitionCondition(state_machine, trans, do_parry, UPE_TRANS_COND_TRUE, 0.0f);
    
    trans = UpeASM_AddTransition(state_machine, idle_state, dodge_state, 0.1f);
    UpeASM_SetTransitionCondition(state_machine, trans, do_dodge, UPE_TRANS_COND_TRUE, 0.0f);
    
    /* Return to idle after action */
    trans = UpeASM_AddTransition(state_machine, attack_state, idle_state, 0.2f);
    UpeASM_SetTransitionCondition(state_machine, trans, do_attack, UPE_TRANS_COND_FALSE, 0.0f);
    UpeASM_SetTransitionCanInterrupt(state_machine, trans, true);
    
    trans = UpeASM_AddTransition(state_machine, parry_state, idle_state, 0.15f);
    UpeASM_SetTransitionCondition(state_machine, trans, do_parry, UPE_TRANS_COND_FALSE, 0.0f);
    UpeASM_SetTransitionCanInterrupt(state_machine, trans, true);
    
    trans = UpeASM_AddTransition(state_machine, dodge_state, idle_state, 0.2f);
    UpeASM_SetTransitionCondition(state_machine, trans, do_dodge, UPE_TRANS_COND_FALSE, 0.0f);
    UpeASM_SetTransitionCanInterrupt(state_machine, trans, true);
    
    return state_machine;
}
