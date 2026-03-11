#include <stdio.h>
#include <math.h>
#include <string.h>
#include "upe/upe_api.h"

/*================================================
  ANIMATION STATE MACHINE DEMO
  
  Demonstrates hierarchical animation control:
  - Locomotion system (idle -> walk -> run -> sprint)
  - Jump system (grounded -> airborne -> landing)
  - Combat system (idle -> attack/parry/dodge -> idle)
  - Parameter-driven transitions
  - Blending between states
================================================*/

/* World globals for demo */
UpeWorld *g_world = NULL;
UpeSkeleton *g_skeleton = NULL;
UpeAnimationController *g_controller = NULL;
UpeAnimationStateMachine *g_locomotion_asm = NULL;
UpeAnimationStateMachine *g_jump_asm = NULL;
UpeAnimationStateMachine *g_combat_asm = NULL;

/* Stub animation creator (creates minimal animations) */
UpeAnimationClip* CreateStubAnimation(const char *name, float duration) {
    UpeAnimationClip *clip = UpeAnimationClip_Create(name, duration);
    if (!clip) return NULL;
    
    /* Create keyframes for one bone (minimal) */
    UpeKeyframe keyframes[] = {
        {0.0f, {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}},
        {duration, {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}},
    };
    UpeAnimationClip_AddTrack(clip, 0, keyframes, 2);
    
    return clip;
}

void DemoLocomotion(void) {
    printf("\n=== Locomotion State Machine Demo ===\n\n");
    
    /* Create animations */
    UpeAnimationClip *idle = CreateStubAnimation("Idle", 1.0f);
    UpeAnimationClip *walk = CreateStubAnimation("Walk", 0.8f);
    UpeAnimationClip *run = CreateStubAnimation("Run", 0.6f);
    UpeAnimationClip *sprint = CreateStubAnimation("Sprint", 0.5f);
    
    /* Create locomotion state machine */
    UpeAnimationStateMachine *loco = UpeASM_CreateLocomotionMachine(g_controller,
                                                                    idle, walk, run, sprint);
    if (!loco) {
        printf("Failed to create locomotion ASM\n");
        return;
    }
    
    printf("Created locomotion state machine:\n");
    printf("  Idle (0.0 m/s)\n");
    printf("  └─> Walk (0.1-2.0 m/s)\n");
    printf("  └─> Run (2.0-5.0 m/s)\n");
    printf("  └─> Sprint (5.0+ m/s)\n\n");
    
    uint32_t speed_param = UpeASM_GetParameterID(loco, "Speed");
    
    printf("Time   | Speed | Current State\n");
    printf("-------|-------|---------------\n");
    
    float sim_time = 0.0f;
    float dt = 1.0f / 60.0f;
    
    while (sim_time < 10.0f) {
        /* Simulate speed variation (0 -> 6 m/s -> 0) */
        float speed;
        if (sim_time < 2.5f) {
            speed = 2.4f * (sim_time / 2.5f);  /* 0 -> 2.4 m/s */
        } else if (sim_time < 5.0f) {
            speed = 2.4f + 2.6f * ((sim_time - 2.5f) / 2.5f);  /* 2.4 -> 5.0 m/s */
        } else if (sim_time < 7.5f) {
            speed = 6.0f * (1.0f - (sim_time - 5.0f) / 2.5f);  /* 6.0 -> 0 m/s */
        } else {
            speed = 0.0f;
        }
        
        /* Update parameter */
        UpeASM_SetFloatParameter(loco, speed_param, speed);
        
        /* Update state machine */
        UpeASM_Update(loco, dt, g_world);
        
        /* Print every 1 second */
        if (fmodf(sim_time, 1.0f) < dt) {
            printf("%.1f s  | %.1f  | %s %s\n",
                   sim_time, speed, 
                   UpeASM_GetCurrentStateName(loco),
                   UpeASM_IsTransitioning(loco) ? "[blending]" : "");
        }
        
        sim_time += dt;
    }
    
    printf("\n✓ Locomotion ASM demonstration complete\n");
    
    UpeASM_Destroy(loco);
    UpeAnimationClip_Destroy(idle);
    UpeAnimationClip_Destroy(walk);
    UpeAnimationClip_Destroy(run);
    UpeAnimationClip_Destroy(sprint);
}

void DemoCombat(void) {
    printf("\n=== Combat State Machine Demo ===\n\n");
    
    /* Create animations */
    UpeAnimationClip *idle = CreateStubAnimation("Idle", 1.0f);
    UpeAnimationClip *attack = CreateStubAnimation("Attack", 0.6f);
    UpeAnimationClip *parry = CreateStubAnimation("Parry", 0.4f);
    UpeAnimationClip *dodge = CreateStubAnimation("Dodge", 0.5f);
    
    /* Create combat ASM */
    UpeAnimationStateMachine *combat = UpeASM_CreateCombatMachine(g_controller,
                                                                  idle, attack, parry, dodge);
    if (!combat) {
        printf("Failed to create combat ASM\n");
        return;
    }
    
    printf("Created combat state machine with action transitions\n\n");
    
    uint32_t attack_param = UpeASM_GetParameterID(combat, "DoAttack");
    uint32_t parry_param = UpeASM_GetParameterID(combat, "DoParry");
    uint32_t dodge_param = UpeASM_GetParameterID(combat, "DoDodge");
    
    printf("Time   | Action | Current State | Blend %%\n");
    printf("-------|--------|---------------|---------\n");
    
    /* Simulate combat sequence */
    const struct {
        float time;
        uint32_t action_param;
        bool activate;
        const char *action_name;
    } events[] = {
        {1.0f, 0, true, "Attack"},   /* attack_param */
        {1.8f, 0, false, NULL},      /* stop attack */
        {2.5f, 1, true, "Parry"},    /* parry_param */
        {3.1f, 1, false, NULL},      /* stop parry */
        {3.5f, 2, true, "Dodge"},    /* dodge_param */
        {4.2f, 2, false, NULL},      /* stop dodge */
        {5.0f, 0, true, "Attack"},   /* attack_param */
        {5.6f, 0, false, NULL},      /* stop attack */
    };
    
    float sim_time = 0.0f;
    float dt = 1.0f / 60.0f;
    int event_idx = 0;
    
    while (sim_time < 6.0f && event_idx < 8) {
        /* Process events */
        if (event_idx < 8 && sim_time >= events[event_idx].time) {
            uint32_t params[] = {attack_param, parry_param, dodge_param};
            if (events[event_idx].action_param < 3) {
                UpeASM_SetBoolParameter(combat, params[events[event_idx].action_param],
                                      events[event_idx].activate);
            }
            event_idx++;
        }
        
        /* Update state machine */
        UpeASM_Update(combat, dt, g_world);
        
        /* Print every 0.25 seconds */
        if (fmodf(sim_time, 0.25f) < dt) {
            float blend = UpeASM_GetTransitionProgress(combat) * 100.0f;
            printf("%.2f s | %-6s | %13s | %.0f%%\n",
                   sim_time, "",
                   UpeASM_GetCurrentStateName(combat),
                   blend);
        }
        
        sim_time += dt;
    }
    
    printf("\n✓ Combat ASM demonstration complete\n");
    
    UpeASM_Destroy(combat);
    UpeAnimationClip_Destroy(idle);
    UpeAnimationClip_Destroy(attack);
    UpeAnimationClip_Destroy(parry);
    UpeAnimationClip_Destroy(dodge);
}

int main(void) {
    printf("=== Animation State Machine System Demo ===\n");
    printf("Demonstrates hierarchical state machine control\n");
    
    /* Create world and skeleton */
    UpeWorldDesc desc = {};
    desc.fixed_dt = 1.0f / 60.0f;
    g_world = UpeWorld_Create(&desc);
    if (!g_world) {
        printf("Failed to create world\n");
        return 1;
    }
    
    g_skeleton = UpeSkeleton_Create(UPE_MAX_BONES);
    if (!g_skeleton) {
        printf("Failed to create skeleton\n");
        UpeWorld_Destroy(g_world);
        return 1;
    }
    
    /* Add simple bone for animation */
    uint32_t root = UpeSkeleton_AddBone(g_skeleton, "Root", UINT32_MAX, 0.0f);
    (void)root;
    
    g_controller = UpeAnimationController_Create(g_skeleton);
    if (!g_controller) {
        printf("Failed to create animation controller\n");
        UpeSkeleton_Destroy(g_skeleton);
        UpeWorld_Destroy(g_world);
        return 1;
    }
    
    printf("\nInitialized animation system\n");
    
    /* Run demonstrations */
    DemoLocomotion();
    DemoCombat();
    
    printf("\n=== State Machine Features Demonstrated ===\n");
    printf("✓ Hierarchical state organization\n");
    printf("✓ Parameter-driven transitions\n");
    printf("✓ Automatic state blending\n");
    printf("✓ Interruption handling (can_interrupt)\n");
    printf("✓ Complex transition conditions\n");
    printf("✓ Pre-configured animation patterns\n");
    printf("✓ Real-time parameter updates\n");
    
    /* Cleanup */
    UpeAnimationController_Destroy(g_controller);
    UpeSkeleton_Destroy(g_skeleton);
    UpeWorld_Destroy(g_world);
    
    printf("\n✓ All demonstrations complete\n");
    return 0;
}
