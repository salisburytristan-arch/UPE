#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Chemistry & Reaction-Diffusion Systems
  
  - Reaction-diffusion equations (pattern formation)
  - Chemical reactions and concentration fields
  - Corrosion, combustion, polymerization
  - pH and dissolution
*/

#define UPE_CHEM_MAX_SPECIES 32
#define UPE_CHEM_MAX_REACTIONS 128

typedef struct UpeChemSpecies {
    char name[32];
    float diffusion_rate;        // m²/s
    float *concentration;        // Concentration grid (mol/L)
    float color[3];              // For visualization (RGB)
    uint32_t grid_width;
    uint32_t grid_height;
    float cell_size;
} UpeChemSpecies;

typedef struct UpeChemReaction {
    uint32_t reactant_indices[4];    // Up to 4 reactants
    float reactant_coefficients[4];  // Stoichiometric coefficients
    uint32_t product_indices[4];     // Up to 4 products
    float product_coefficients[4];
    float rate_constant;             // Reaction rate (depends on order)
    float activation_energy;         // For temperature dependence
} UpeChemReaction;

typedef struct UpeChemSystem {
    UpeChemSpecies *species;
    uint32_t species_count;
    uint32_t species_capacity;
    
    UpeChemReaction *reactions;
    uint32_t reaction_count;
    uint32_t reaction_capacity;
    
    // Environment
    float temperature;           // K (for rate constants)
    float *ph_field;             // pH grid (optional)
    
    // Grid dimensions (shared by all species)
    uint32_t grid_width;
    uint32_t grid_height;
    float cell_size;
    
} UpeChemSystem;

void UpeChem_Init(UpeChemSystem *chem, uint32_t grid_width, uint32_t grid_height, float cell_size);
void UpeChem_Destroy(UpeChemSystem *chem);

// Species management
uint32_t UpeChem_AddSpecies(UpeChemSystem *chem, const char *name, float diffusion_rate);
void UpeChem_SetConcentration(UpeChemSystem *chem, uint32_t species_id, uint32_t x, uint32_t y, float concentration);
float UpeChem_GetConcentration(const UpeChemSystem *chem, uint32_t species_id, uint32_t x, uint32_t y);

// Reaction management
uint32_t UpeChem_AddReaction(UpeChemSystem *chem, const UpeChemReaction *reaction);

// Simulation
void UpeChem_Step(UpeChemSystem *chem, float dt);
void UpeChem_DiffuseSpecies(UpeChemSystem *chem, uint32_t species_id, float dt);
void UpeChem_ApplyReactions(UpeChemSystem *chem, float dt);

// Predefined reactions
void UpeChem_ReactionCorrosion(UpeChemSystem *chem); // Metal + O2 → Oxide
void UpeChem_ReactionCombustion(UpeChemSystem *chem); // Fuel + O2 → CO2 + H2O
void UpeChem_ReactionAcidBase(UpeChemSystem *chem);  // H+ + OH- → H2O

// Classic reaction-diffusion patterns
void UpeChem_GrayScott(UpeChemSystem *chem, float feed_rate, float kill_rate);
void UpeChem_TuringPattern(UpeChemSystem *chem);

#ifdef __cplusplus
}
#endif
