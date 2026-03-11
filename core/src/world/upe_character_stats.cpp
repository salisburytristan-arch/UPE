#include "upe/upe_character_stats.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

/* Stat derivation formulas */
#define HP_PER_CONSTITUTION 8.0f
#define STAMINA_PER_WISDOM 5.0f
#define MANA_PER_INTELLIGENCE 6.0f
#define ARMOR_PER_CONSTITUTION 0.5f

/* Regen rates (per second) */
#define BASE_HEALTH_REGEN 0.5f
#define BASE_STAMINA_REGEN 2.0f
#define BASE_MANA_REGEN 1.5f

/* Combat formulas */
#define BASE_CRITICAL_CHANCE 5.0f
#define DODGE_BASE 5.0f
#define BLOCK_BASE 5.0f

static char stat_debug_buffer[2048];

/* Helper: clamp value */
static inline float upe_stat_clamp(float val, float min_val, float max_val) {
    return val < min_val ? min_val : (val > max_val ? max_val : val);
}

/* Create default stats */
UpeCharacterStats* UpeCharacterStats_Create(void) {
    UpeCharacterStats* stats = (UpeCharacterStats*)calloc(1, sizeof(UpeCharacterStats));
    if (!stats) return NULL;
    
    /* Default attributes (all 10 = average) */
    for (int i = 0; i < UPE_STAT_COUNT; i++) {
        stats->attributes[i] = 10.0f;
    }
    
    /* Default skills (0 = untrained) */
    for (int i = 0; i < UPE_SKILL_COUNT; i++) {
        stats->skills[i] = 0.0f;
        stats->skill_xp[i] = 0.0f;
        stats->skill_level[i] = 0;
    }
    
    /* Initialize resources */
    stats->level = 1;
    stats->character_class = UPE_CLASS_CUSTOM;
    stats->is_alive = 1;
    stats->is_conscious = 1;
    stats->alignment = 0.0f;
    stats->reputation = 0;
    stats->movement_speed_mult = 1.0f;
    stats->encumbrance = 0.0f;
    stats->condition_count = 0;
    
    /* Calculate derived stats */
    UpeCharacterStats_RecalculateDerived(stats);
    
    return stats;
}

void UpeCharacterStats_Destroy(UpeCharacterStats* stats) {
    if (stats) {
        free(stats);
    }
}

/* Class presets */
UpeCharacterStats* UpeCharacterStats_CreateWarrior(void) {
    UpeCharacterStats* stats = UpeCharacterStats_Create();
    if (!stats) return NULL;
    
    stats->character_class = UPE_CLASS_WARRIOR;
    stats->attributes[UPE_STAT_STRENGTH] = 15.0f;
    stats->attributes[UPE_STAT_CONSTITUTION] = 14.0f;
    stats->attributes[UPE_STAT_DEXTERITY] = 10.0f;
    stats->attributes[UPE_STAT_INTELLIGENCE] = 8.0f;
    stats->attributes[UPE_STAT_WISDOM] = 10.0f;
    stats->attributes[UPE_STAT_CHARISMA] = 11.0f;
    
    stats->skills[UPE_SKILL_MELEE_COMBAT] = 30.0f;
    stats->skills[UPE_SKILL_BLOCKING] = 25.0f;
    
    UpeCharacterStats_RecalculateDerived(stats);
    return stats;
}

UpeCharacterStats* UpeCharacterStats_CreateRogue(void) {
    UpeCharacterStats* stats = UpeCharacterStats_Create();
    if (!stats) return NULL;
    
    stats->character_class = UPE_CLASS_ROGUE;
    stats->attributes[UPE_STAT_DEXTERITY] = 16.0f;
    stats->attributes[UPE_STAT_INTELLIGENCE] = 12.0f;
    stats->attributes[UPE_STAT_CONSTITUTION] = 10.0f;
    stats->attributes[UPE_STAT_STRENGTH] = 11.0f;
    stats->attributes[UPE_STAT_WISDOM] = 11.0f;
    stats->attributes[UPE_STAT_CHARISMA] = 12.0f;
    
    stats->skills[UPE_SKILL_THIEVERY] = 40.0f;
    stats->skills[UPE_SKILL_DODGE] = 30.0f;
    stats->skills[UPE_SKILL_RANGED_COMBAT] = 25.0f;
    
    UpeCharacterStats_RecalculateDerived(stats);
    return stats;
}

UpeCharacterStats* UpeCharacterStats_CreateMage(void) {
    UpeCharacterStats* stats = UpeCharacterStats_Create();
    if (!stats) return NULL;
    
    stats->character_class = UPE_CLASS_MAGE;
    stats->attributes[UPE_STAT_INTELLIGENCE] = 16.0f;
    stats->attributes[UPE_STAT_WISDOM] = 13.0f;
    stats->attributes[UPE_STAT_DEXTERITY] = 11.0f;
    stats->attributes[UPE_STAT_CONSTITUTION] = 9.0f;
    stats->attributes[UPE_STAT_STRENGTH] = 8.0f;
    stats->attributes[UPE_STAT_CHARISMA] = 12.0f;
    
    stats->skills[UPE_SKILL_MAGIC] = 35.0f;
    stats->skills[UPE_SKILL_SURVIVAL] = 15.0f;
    
    UpeCharacterStats_RecalculateDerived(stats);
    return stats;
}

UpeCharacterStats* UpeCharacterStats_CreateCleric(void) {
    UpeCharacterStats* stats = UpeCharacterStats_Create();
    if (!stats) return NULL;
    
    stats->character_class = UPE_CLASS_CLERIC;
    stats->attributes[UPE_STAT_WISDOM] = 15.0f;
    stats->attributes[UPE_STAT_STRENGTH] = 12.0f;
    stats->attributes[UPE_STAT_CONSTITUTION] = 12.0f;
    stats->attributes[UPE_STAT_INTELLIGENCE] = 11.0f;
    stats->attributes[UPE_STAT_DEXTERITY] = 10.0f;
    stats->attributes[UPE_STAT_CHARISMA] = 14.0f;
    
    stats->skills[UPE_SKILL_MAGIC] = 25.0f;
    stats->skills[UPE_SKILL_SURVIVAL] = 20.0f;
    stats->skills[UPE_SKILL_MELEE_COMBAT] = 20.0f;
    
    UpeCharacterStats_RecalculateDerived(stats);
    return stats;
}

UpeCharacterStats* UpeCharacterStats_CreatePaladin(void) {
    UpeCharacterStats* stats = UpeCharacterStats_Create();
    if (!stats) return NULL;
    
    stats->character_class = UPE_CLASS_PALADIN;
    stats->attributes[UPE_STAT_STRENGTH] = 14.0f;
    stats->attributes[UPE_STAT_WISDOM] = 14.0f;
    stats->attributes[UPE_STAT_CONSTITUTION] = 13.0f;
    stats->attributes[UPE_STAT_CHARISMA] = 13.0f;
    stats->attributes[UPE_STAT_DEXTERITY] = 10.0f;
    stats->attributes[UPE_STAT_INTELLIGENCE] = 10.0f;
    
    stats->skills[UPE_SKILL_MELEE_COMBAT] = 25.0f;
    stats->skills[UPE_SKILL_BLOCKING] = 25.0f;
    stats->skills[UPE_SKILL_MAGIC] = 15.0f;
    
    UpeCharacterStats_RecalculateDerived(stats);
    return stats;
}

UpeCharacterStats* UpeCharacterStats_CreateRanger(void) {
    UpeCharacterStats* stats = UpeCharacterStats_Create();
    if (!stats) return NULL;
    
    stats->character_class = UPE_CLASS_RANGER;
    stats->attributes[UPE_STAT_DEXTERITY] = 14.0f;
    stats->attributes[UPE_STAT_WISDOM] = 14.0f;
    stats->attributes[UPE_STAT_CONSTITUTION] = 12.0f;
    stats->attributes[UPE_STAT_STRENGTH] = 12.0f;
    stats->attributes[UPE_STAT_INTELLIGENCE] = 10.0f;
    stats->attributes[UPE_STAT_CHARISMA] = 11.0f;
    
    stats->skills[UPE_SKILL_RANGED_COMBAT] = 30.0f;
    stats->skills[UPE_SKILL_SURVIVAL] = 25.0f;
    stats->skills[UPE_SKILL_DODGE] = 20.0f;
    
    UpeCharacterStats_RecalculateDerived(stats);
    return stats;
}

/* Attribute access */
float UpeCharacterStats_GetStat(const UpeCharacterStats* stats, UpeStatType stat) {
    if (!stats || stat >= UPE_STAT_COUNT) return 10.0f;
    return stats->attributes[stat];
}

void UpeCharacterStats_SetStat(UpeCharacterStats* stats, UpeStatType stat, float value) {
    if (!stats || stat >= UPE_STAT_COUNT) return;
    stats->attributes[stat] = upe_stat_clamp(value, 1.0f, 20.0f);
    UpeCharacterStats_RecalculateDerived(stats);
}

void UpeCharacterStats_ModifyStat(UpeCharacterStats* stats, UpeStatType stat, float delta) {
    if (!stats || stat >= UPE_STAT_COUNT) return;
    stats->attributes[stat] = upe_stat_clamp(stats->attributes[stat] + delta, 1.0f, 20.0f);
    UpeCharacterStats_RecalculateDerived(stats);
}

/* Skill system */
float UpeCharacterStats_GetSkill(const UpeCharacterStats* stats, UpeSkillType skill) {
    if (!stats || skill >= UPE_SKILL_COUNT) return 0.0f;
    return stats->skills[skill];
}

void UpeCharacterStats_SetSkill(UpeCharacterStats* stats, UpeSkillType skill, float value) {
    if (!stats || skill >= UPE_SKILL_COUNT) return;
    stats->skills[skill] = upe_stat_clamp(value, 0.0f, 100.0f);
}

void UpeCharacterStats_AddSkillXP(UpeCharacterStats* stats, UpeSkillType skill, float amount) {
    if (!stats || skill >= UPE_SKILL_COUNT) return;
    
    stats->skill_xp[skill] += amount;
    float xp_per_level = 100.0f;
    
    while (stats->skill_xp[skill] >= xp_per_level && stats->skills[skill] < 100.0f) {
        stats->skill_level[skill]++;
        stats->skills[skill] += 10.0f;
        if (stats->skills[skill] > 100.0f) stats->skills[skill] = 100.0f;
        stats->skill_xp[skill] -= xp_per_level;
    }
}

/* Resource management */
void UpeCharacterStats_SetResource(UpeCharacterStats* stats, UpeResourceType resource, float value) {
    if (!stats) return;
    
    switch (resource) {
        case UPE_RESOURCE_HEALTH:
            stats->health = upe_stat_clamp(value, 0.0f, stats->max_health);
            break;
        case UPE_RESOURCE_STAMINA:
            stats->stamina = upe_stat_clamp(value, 0.0f, stats->max_stamina);
            break;
        case UPE_RESOURCE_MANA:
            stats->mana = upe_stat_clamp(value, 0.0f, stats->max_mana);
            break;
        default:
            break;
    }
}

float UpeCharacterStats_GetResource(const UpeCharacterStats* stats, UpeResourceType resource) {
    if (!stats) return 0.0f;
    
    switch (resource) {
        case UPE_RESOURCE_HEALTH: return stats->health;
        case UPE_RESOURCE_STAMINA: return stats->stamina;
        case UPE_RESOURCE_MANA: return stats->mana;
        default: return 0.0f;
    }
}

float UpeCharacterStats_GetResourceMax(const UpeCharacterStats* stats, UpeResourceType resource) {
    if (!stats) return 0.0f;
    
    switch (resource) {
        case UPE_RESOURCE_HEALTH: return stats->max_health;
        case UPE_RESOURCE_STAMINA: return stats->max_stamina;
        case UPE_RESOURCE_MANA: return stats->max_mana;
        default: return 0.0f;
    }
}

int UpeCharacterStats_HasResource(const UpeCharacterStats* stats, UpeResourceType resource, float amount) {
    return UpeCharacterStats_GetResource(stats, resource) >= amount;
}

/* Health and damage */
void UpeCharacterStats_TakeDamage(UpeCharacterStats* stats, float damage) {
    if (!stats || !stats->is_alive) return;
    
    /* Apply armor reduction */
    float effective_damage = damage - stats->armor;
    if (effective_damage < 0.1f) effective_damage = 0.1f;  /* Minimum 0.1 damage gets through */
    
    stats->health -= effective_damage;
    if (stats->health < 0.0f) {
        stats->health = 0.0f;
        stats->is_alive = 0;
        stats->is_conscious = 0;
    }
}

void UpeCharacterStats_TakeHealing(UpeCharacterStats* stats, float healing) {
    if (!stats) return;
    stats->health += healing;
    if (stats->health > stats->max_health) {
        stats->health = stats->max_health;
    }
    if (stats->health > 0.0f) {
        stats->is_alive = 1;
    }
}

int UpeCharacterStats_IsAlive(const UpeCharacterStats* stats) {
    return stats ? stats->is_alive : 0;
}

int UpeCharacterStats_IsDefeated(const UpeCharacterStats* stats) {
    return stats ? (!stats->is_alive || !stats->is_conscious) : 1;
}

/* Stamina */
void UpeCharacterStats_ConsumStamina(UpeCharacterStats* stats, float amount) {
    if (!stats) return;
    stats->stamina -= amount;
    if (stats->stamina < 0.0f) stats->stamina = 0.0f;
}

void UpeCharacterStats_RecoverStamina(UpeCharacterStats* stats, float amount) {
    if (!stats) return;
    stats->stamina += amount;
    if (stats->stamina > stats->max_stamina) {
        stats->stamina = stats->max_stamina;
    }
}

int UpeCharacterStats_CanAction(const UpeCharacterStats* stats, float cost) {
    return stats && stats->stamina >= cost;
}

/* Combat mechanics */
float UpeCharacterStats_GetDamageMultiplier(const UpeCharacterStats* stats) {
    if (!stats) return 1.0f;
    
    /* Every 3 STR = +10% damage */
    float str_bonus = (stats->attributes[UPE_STAT_STRENGTH] - 10.0f) * 0.033f;
    float skill_bonus = stats->skills[UPE_SKILL_MELEE_COMBAT] * 0.005f;
    
    return 1.0f + str_bonus + skill_bonus;
}

float UpeCharacterStats_GetDefenseMultiplier(const UpeCharacterStats* stats) {
    if (!stats) return 1.0f;
    
    /* Every 4 CON = +10% defense */
    float con_bonus = (stats->attributes[UPE_STAT_CONSTITUTION] - 10.0f) * 0.025f;
    float skill_bonus = stats->skills[UPE_SKILL_BLOCKING] * 0.003f;
    
    return 1.0f + con_bonus + skill_bonus;
}

float UpeCharacterStats_RollAttackHit(const UpeCharacterStats* attacker, 
                                      const UpeCharacterStats* defender) {
    if (!attacker || !defender) return 0.5f;
    
    /* Base hit chance 50% */
    float hit_chance = 50.0f;
    
    /* Attacker DEX/skill improves accuracy */
    hit_chance += (attacker->attributes[UPE_STAT_DEXTERITY] - 10.0f) * 2.0f;
    hit_chance += attacker->skills[UPE_SKILL_MELEE_COMBAT] * 0.3f;
    
    /* Defender DEX/dodge reduces hit chance */
    hit_chance -= (defender->attributes[UPE_STAT_DEXTERITY] - 10.0f) * 2.0f;
    hit_chance -= defender->skills[UPE_SKILL_DODGE] * 0.3f;
    
    /* Clamp to 10-90% */
    hit_chance = upe_stat_clamp(hit_chance, 10.0f, 90.0f);
    
    return hit_chance / 100.0f;
}

/* Conditions */
void UpeCharacterStats_ApplyCondition(UpeCharacterStats* stats, 
                                      UpeCharacterCondition* condition) {
    if (!stats || !condition || stats->condition_count >= 16) return;
    
    stats->conditions[stats->condition_count] = *condition;
    stats->conditions[stats->condition_count].is_active = 1;
    stats->condition_count++;
}

void UpeCharacterStats_RemoveCondition(UpeCharacterStats* stats, uint32_t condition_id) {
    if (!stats) return;
    
    for (uint32_t i = 0; i < stats->condition_count; i++) {
        if (stats->conditions[i].id == condition_id) {
            /* Shift remaining conditions */
            for (uint32_t j = i; j < stats->condition_count - 1; j++) {
                stats->conditions[j] = stats->conditions[j + 1];
            }
            stats->condition_count--;
            break;
        }
    }
}

int UpeCharacterStats_HasCondition(const UpeCharacterStats* stats, const char* name) {
    if (!stats || !name) return 0;
    
    for (uint32_t i = 0; i < stats->condition_count; i++) {
        if (stats->conditions[i].name && strcmp(stats->conditions[i].name, name) == 0) {
            return stats->conditions[i].is_active;
        }
    }
    return 0;
}

/* Level and experience */
void UpeCharacterStats_GainExperience(UpeCharacterStats* stats, uint32_t amount) {
    if (!stats) return;
    
    stats->experience += amount;
    while (stats->experience >= stats->experience_next_level) {
        UpeCharacterStats_LevelUp(stats);
    }
}

void UpeCharacterStats_LevelUp(UpeCharacterStats* stats) {
    if (!stats) return;
    
    stats->level++;
    stats->experience_next_level = (uint32_t)(1000 * powf(1.05f, (float)stats->level - 1));
    
    /* Increase max resources */
    stats->max_health += 10.0f;
    stats->max_stamina += 5.0f;
    stats->max_mana += 8.0f;
    
    UpeCharacterStats_RecalculateDerived(stats);
}

float UpeCharacterStats_GetExpProgress(const UpeCharacterStats* stats) {
    if (!stats || stats->experience_next_level == 0) return 0.0f;
    return (float)stats->experience / (float)stats->experience_next_level;
}

/* Encumbrance */
void UpeCharacterStats_SetEncumbrance(UpeCharacterStats* stats, float percent) {
    if (!stats) return;
    stats->encumbrance = upe_stat_clamp(percent, 0.0f, 100.0f);
}

float UpeCharacterStats_GetMovementSpeedMultiplier(const UpeCharacterStats* stats) {
    if (!stats) return 1.0f;
    
    /* 25% encumbrance = 10% speed reduction, etc. */
    float encumb_penalty = (stats->encumbrance / 100.0f) * 0.4f;
    return upe_stat_clamp(1.0f - encumb_penalty, 0.3f, 1.2f) * stats->movement_speed_mult;
}

/* Composite calculations */
float UpeCharacterStats_GetEffectiveStat(const UpeCharacterStats* stats, UpeStatType stat) {
    if (!stats || stat >= UPE_STAT_COUNT) return 10.0f;
    
    float effective = stats->attributes[stat];
    
    /* Apply condition modifiers */
    for (uint32_t i = 0; i < stats->condition_count; i++) {
        if (stats->conditions[i].is_active) {
            effective += stats->conditions[i].stat_modifiers[stat];
        }
    }
    
    return effective;
}

void UpeCharacterStats_RecalculateDerived(UpeCharacterStats* stats) {
    if (!stats) return;
    
    float con = UpeCharacterStats_GetEffectiveStat(stats, UPE_STAT_CONSTITUTION);
    float wis = UpeCharacterStats_GetEffectiveStat(stats, UPE_STAT_WISDOM);
    float intel = UpeCharacterStats_GetEffectiveStat(stats, UPE_STAT_INTELLIGENCE);
    float str = UpeCharacterStats_GetEffectiveStat(stats, UPE_STAT_STRENGTH);
    float dex = UpeCharacterStats_GetEffectiveStat(stats, UPE_STAT_DEXTERITY);
    
    /* Calculate max resources */
    stats->max_health = 50.0f + (con * HP_PER_CONSTITUTION) + (stats->level * 8.0f);
    stats->max_stamina = 40.0f + (wis * STAMINA_PER_WISDOM) + (stats->level * 5.0f);
    stats->max_mana = 30.0f + (intel * MANA_PER_INTELLIGENCE) + (stats->level * 6.0f);
    
    /* Clamp current values to max */
    if (stats->health > stats->max_health) stats->health = stats->max_health;
    if (stats->stamina > stats->max_stamina) stats->stamina = stats->max_stamina;
    if (stats->mana > stats->max_mana) stats->mana = stats->max_mana;
    
    /* Regen rates */
    stats->health_regen = BASE_HEALTH_REGEN + (con - 10.0f) * 0.1f;
    stats->stamina_regen = BASE_STAMINA_REGEN + (wis - 10.0f) * 0.15f;
    stats->mana_regen = BASE_MANA_REGEN + (intel - 10.0f) * 0.15f;
    
    /* Combat stats */
    stats->armor = (con * ARMOR_PER_CONSTITUTION);
    stats->critical_chance = BASE_CRITICAL_CHANCE + (dex - 10.0f) * 1.5f;
    stats->critical_damage = 1.5f + (str - 10.0f) * 0.05f;
    stats->dodge_chance = DODGE_BASE + (dex - 10.0f) * 2.0f;
    stats->block_chance = BLOCK_BASE + (str - 10.0f) * 1.0f;
}

/* Update (handle conditions, regen) */
void UpeCharacterStats_Update(UpeCharacterStats* stats, float dt) {
    if (!stats || !stats->is_alive) return;
    
    /* Regenerate resources */
    stats->health += stats->health_regen * dt;
    if (stats->health > stats->max_health) stats->health = stats->max_health;
    
    if (stats->is_conscious) {
        stats->stamina += stats->stamina_regen * dt;
        if (stats->stamina > stats->max_stamina) stats->stamina = stats->max_stamina;
        
        stats->mana += stats->mana_regen * dt;
        if (stats->mana > stats->max_mana) stats->mana = stats->max_mana;
    } else {
        /* Faster stamina/mana regen while unconscious */
        stats->stamina += stats->stamina_regen * dt * 1.5f;
        stats->mana += stats->mana_regen * dt * 1.5f;
    }
    
    /* Update conditions */
    for (uint32_t i = 0; i < stats->condition_count; i++) {
        if (stats->conditions[i].remaining_time > 0.0f) {
            stats->conditions[i].remaining_time -= dt;
            if (stats->conditions[i].remaining_time <= 0.0f) {
                stats->conditions[i].is_active = 0;
            }
        }
    }
    
    /* Remove inactive conditions */
    for (uint32_t i = 0; i < stats->condition_count; ) {
        if (!stats->conditions[i].is_active && stats->conditions[i].remaining_time != -1.0f) {
            /* Shift remaining conditions */
            for (uint32_t j = i; j < stats->condition_count - 1; j++) {
                stats->conditions[j] = stats->conditions[j + 1];
            }
            stats->condition_count--;
        } else {
            i++;
        }
    }
}

/* Debug */
const char* UpeCharacterStats_GetDebugString(const UpeCharacterStats* stats) {
    if (!stats) {
        sprintf(stat_debug_buffer, "Stats: NULL");
        return stat_debug_buffer;
    }
    
    sprintf(stat_debug_buffer,
        "Lvl:%d HP:%.0f/%.0f Sta:%.0f/%.0f Mana:%.0f/%.0f | "
        "STR:%.0f DEX:%.0f CON:%.0f INT:%.0f WIS:%.0f CHA:%.0f | "
        "Armor:%.0f Dodge:%.0f%% Crit:%.0f%%",
        stats->level,
        stats->health, stats->max_health,
        stats->stamina, stats->max_stamina,
        stats->mana, stats->max_mana,
        stats->attributes[UPE_STAT_STRENGTH],
        stats->attributes[UPE_STAT_DEXTERITY],
        stats->attributes[UPE_STAT_CONSTITUTION],
        stats->attributes[UPE_STAT_INTELLIGENCE],
        stats->attributes[UPE_STAT_WISDOM],
        stats->attributes[UPE_STAT_CHARISMA],
        stats->armor,
        stats->dodge_chance,
        stats->critical_chance
    );
    
    return stat_debug_buffer;
}
