/**
 * UPE Character Stats System
 * 
 * Manages player/NPC statistics and attributes:
 * - Core attributes (strength, dexterity, constitution, etc.)
 * - Health, stamina, and resource management
 * - Skill tracking and progression
 * - Stat-based game mechanics (damage calculation, defense, etc.)
 * 
 * Designed to work with character controller, advanced movement, and animation systems.
 * No direct physics coupling - purely gameplay mechanics.
 * 
 * Usage:
 *   UpeCharacterStats* stats = UpeCharacterStats_Create();
 *   UpeCharacterStats_SetStat(stats, UPE_STAT_STRENGTH, 15);
 *   float dmg_multiplier = UpeCharacterStats_GetDamageMultiplier(stats);
 *   UpeCharacterStats_TakeDamage(stats, 25.0f);
 *   bool alive = UpeCharacterStats_IsAlive(stats);
 */

#ifndef UPE_CHARACTER_STATS_H
#define UPE_CHARACTER_STATS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Core attributes (1-20 scale, 10 = average) */
typedef enum {
    UPE_STAT_STRENGTH = 0,      /* Physical damage, carry capacity */
    UPE_STAT_DEXTERITY = 1,     /* Attack speed, dodge chance */
    UPE_STAT_CONSTITUTION = 2,  /* Max health, stamina recovery */
    UPE_STAT_INTELLIGENCE = 3,  /* Mana, spell effectiveness */
    UPE_STAT_WISDOM = 4,        /* Max stamina, perception */
    UPE_STAT_CHARISMA = 5,      /* NPC interaction, prices */
    UPE_STAT_COUNT = 6
} UpeStatType;

/* Resource types */
typedef enum {
    UPE_RESOURCE_HEALTH = 0,
    UPE_RESOURCE_STAMINA = 1,
    UPE_RESOURCE_MANA = 2,
    UPE_RESOURCE_COUNT = 3
} UpeResourceType;

/* Skill levels (0-100 scale) */
typedef enum {
    UPE_SKILL_MELEE_COMBAT = 0,
    UPE_SKILL_RANGED_COMBAT = 1,
    UPE_SKILL_DODGE = 2,
    UPE_SKILL_BLOCKING = 3,
    UPE_SKILL_MAGIC = 4,
    UPE_SKILL_THIEVERY = 5,
    UPE_SKILL_SURVIVAL = 6,
    UPE_SKILL_COUNT = 7
} UpeSkillType;

/* Character class/archetype */
typedef enum {
    UPE_CLASS_WARRIOR = 0,      /* High STR, tank */
    UPE_CLASS_ROGUE = 1,        /* High DEX, damage */
    UPE_CLASS_MAGE = 2,         /* High INT, spells */
    UPE_CLASS_CLERIC = 3,       /* High WIS, healing */
    UPE_CLASS_PALADIN = 4,      /* Balanced, hybrid */
    UPE_CLASS_RANGER = 5,       /* DEX/WIS, ranged */
    UPE_CLASS_CUSTOM = 6        /* User-defined */
} UpeCharacterClass;

/* Condition/status effect */
typedef struct {
    uint32_t id;                /* Unique condition ID */
    const char* name;           /* Effect name */
    float remaining_time;       /* Duration left (seconds, -1 = permanent) */
    float stat_modifiers[UPE_STAT_COUNT];  /* Stat adjustment while active */
    int is_active;              /* Currently affecting character */
} UpeCharacterCondition;

/* Core character stats structure */
typedef struct {
    /* Attributes (1-20 scale) */
    float attributes[UPE_STAT_COUNT];
    
    /* Resources (current / max) */
    float health;               /* Current HP */
    float max_health;           /* Max HP (calculated from CON) */
    float stamina;              /* Current stamina */
    float max_stamina;          /* Max stamina (calculated from WIS) */
    float mana;                 /* Current mana */
    float max_mana;             /* Max mana (calculated from INT) */
    
    /* Regeneration (per second) */
    float health_regen;         /* HP/s (from CON) */
    float stamina_regen;        /* Stamina/s (from WIS) */
    float mana_regen;           /* Mana/s (from INT) */
    
    /* Skills (0-100 scale) */
    float skills[UPE_SKILL_COUNT];
    float skill_xp[UPE_SKILL_COUNT];      /* XP toward next level */
    uint32_t skill_level[UPE_SKILL_COUNT];/* Current level per skill */
    
    /* Character info */
    UpeCharacterClass character_class;
    uint32_t level;             /* Character level (1+) */
    uint32_t experience;        /* Total XP earned */
    uint32_t experience_next_level;  /* XP needed for next level */
    
    /* Combat stats (derived from attributes) */
    float armor;                /* Damage reduction (flat) */
    float resistances[3];       /* Physical, Magic, Fire */
    float critical_chance;      /* Percent (0-100) */
    float critical_damage;      /* Multiplier (1.5 = 150% damage) */
    float dodge_chance;         /* Percent (0-100) */
    float block_chance;         /* Percent (0-100) */
    
    /* Movement (affected by encumbrance) */
    float movement_speed_mult;  /* Multiplier (1.0 = normal) */
    float encumbrance;          /* 0-100%, affects speed/stamina */
    
    /* Conditions/status effects */
    UpeCharacterCondition conditions[16];
    uint32_t condition_count;
    
    /* Reputation/alignment */
    int reputation;             /* -1000 to +1000 scale */
    float alignment;            /* -1.0 (evil) to +1.0 (good) */
    
    /* State flags */
    int is_alive;               /* Still living */
    int is_conscious;           /* Awake and acting */
    int is_poisoned;            /* Taking damage over time */
    int is_burning;             /* Fire damage over time */
    int is_frozen;              /* Movement/action slowed */
} UpeCharacterStats;

/* Creation and lifecycle */
UpeCharacterStats* UpeCharacterStats_Create(void);
void UpeCharacterStats_Destroy(UpeCharacterStats* stats);

/* Class presets */
UpeCharacterStats* UpeCharacterStats_CreateWarrior(void);
UpeCharacterStats* UpeCharacterStats_CreateRogue(void);
UpeCharacterStats* UpeCharacterStats_CreateMage(void);
UpeCharacterStats* UpeCharacterStats_CreateCleric(void);
UpeCharacterStats* UpeCharacterStats_CreatePaladin(void);
UpeCharacterStats* UpeCharacterStats_CreateRanger(void);

/* Attribute access */
float UpeCharacterStats_GetStat(const UpeCharacterStats* stats, UpeStatType stat);
void UpeCharacterStats_SetStat(UpeCharacterStats* stats, UpeStatType stat, float value);
void UpeCharacterStats_ModifyStat(UpeCharacterStats* stats, UpeStatType stat, float delta);

/* Skill system */
float UpeCharacterStats_GetSkill(const UpeCharacterStats* stats, UpeSkillType skill);
void UpeCharacterStats_SetSkill(UpeCharacterStats* stats, UpeSkillType skill, float value);
void UpeCharacterStats_AddSkillXP(UpeCharacterStats* stats, UpeSkillType skill, float amount);

/* Resource management */
void UpeCharacterStats_SetResource(UpeCharacterStats* stats, UpeResourceType resource, float value);
float UpeCharacterStats_GetResource(const UpeCharacterStats* stats, UpeResourceType resource);
float UpeCharacterStats_GetResourceMax(const UpeCharacterStats* stats, UpeResourceType resource);
int UpeCharacterStats_HasResource(const UpeCharacterStats* stats, UpeResourceType resource, float amount);

/* Health and damage */
void UpeCharacterStats_TakeDamage(UpeCharacterStats* stats, float damage);
void UpeCharacterStats_TakeHealing(UpeCharacterStats* stats, float healing);
int UpeCharacterStats_IsAlive(const UpeCharacterStats* stats);
int UpeCharacterStats_IsDefeated(const UpeCharacterStats* stats);  /* Dead or unconscious */

/* Stamina (for movement/actions) */
void UpeCharacterStats_ConsumStamina(UpeCharacterStats* stats, float amount);
void UpeCharacterStats_RecoverStamina(UpeCharacterStats* stats, float amount);
int UpeCharacterStats_CanAction(const UpeCharacterStats* stats, float cost);

/* Combat mechanics */
float UpeCharacterStats_GetDamageMultiplier(const UpeCharacterStats* stats);
float UpeCharacterStats_GetDefenseMultiplier(const UpeCharacterStats* stats);
float UpeCharacterStats_RollAttackHit(const UpeCharacterStats* attacker, 
                                      const UpeCharacterStats* defender);

/* Conditions/status */
void UpeCharacterStats_ApplyCondition(UpeCharacterStats* stats, 
                                      UpeCharacterCondition* condition);
void UpeCharacterStats_RemoveCondition(UpeCharacterStats* stats, uint32_t condition_id);
int UpeCharacterStats_HasCondition(const UpeCharacterStats* stats, const char* name);

/* Level and experience */
void UpeCharacterStats_GainExperience(UpeCharacterStats* stats, uint32_t amount);
void UpeCharacterStats_LevelUp(UpeCharacterStats* stats);
float UpeCharacterStats_GetExpProgress(const UpeCharacterStats* stats);

/* Encumbrance/inventory weight */
void UpeCharacterStats_SetEncumbrance(UpeCharacterStats* stats, float percent);
float UpeCharacterStats_GetMovementSpeedMultiplier(const UpeCharacterStats* stats);

/* Composite calculations */
float UpeCharacterStats_GetEffectiveStat(const UpeCharacterStats* stats, UpeStatType stat);
void UpeCharacterStats_RecalculateDerived(UpeCharacterStats* stats);

/* Update */
void UpeCharacterStats_Update(UpeCharacterStats* stats, float dt);

/* Debug */
const char* UpeCharacterStats_GetDebugString(const UpeCharacterStats* stats);

#ifdef __cplusplus
}
#endif

#endif
