#pragma once

#include <stdint.h>
#include "upe/upe_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
  Unified Material System
  
  Every domain "feels real" through comprehensive material properties.
  100+ material presets covering woods, metals, polymers, ceramics, soils, liquids, gases.
  
  Properties organized by physics domain:
  - Bulk: density, elasticity, plasticity, fracture
  - Surface: friction, restitution, adhesion, wear
  - Thermal: conductivity, capacity, expansion
  - Electrical/Magnetic: conductivity, permittivity, permeability
  - Chemical: corrosion, flammability, toxicity
  - Extra: anisotropy, porosity, moisture, creep, fatigue
*/

// Material ID (for 100+ presets)
typedef uint32_t UpeMaterialID;

// Combine modes (for contact between different materials)
typedef enum UpeMaterialCombine {
    UPE_COMBINE_AVERAGE = 0,
    UPE_COMBINE_MIN = 1,
    UPE_COMBINE_MAX = 2,
    UPE_COMBINE_MULTIPLY = 3,
    UPE_COMBINE_CUSTOM = 4
} UpeMaterialCombine;

// Bulk (volume) properties
typedef struct UpeMaterialBulk {
    float density;               // kg/m³
    float youngs_modulus;        // Pa (stiffness)
    float poissons_ratio;        // 0..0.5
    float shear_modulus;         // Pa (derived or explicit)
    float bulk_modulus;          // Pa (compression resistance)
    
    // Damping
    float viscous_damping;       // Linear damping coefficient
    float angular_damping;       // Angular damping coefficient
    
    // Plasticity
    float yield_stress;          // Pa (onset of plastic deformation)
    float hardening_coefficient; // For work hardening
    float plastic_flow_rate;     // How fast it deforms after yield
    
    // Fracture
    float fracture_toughness;    // J/m² (energy to propagate crack)
    float tensile_strength;      // Pa (ultimate strength)
    float compressive_strength;  // Pa
    
    // Anisotropy (grain direction)
    UpeVec3 grain_direction;     // For wood, composites
    float anisotropy_ratio;      // 1.0 = isotropic, >1 = stronger along grain
    
    // Porosity & moisture
    float porosity;              // 0..1 (void fraction)
    float moisture_content;      // 0..1 (current wetness)
    float moisture_absorption;   // Rate of water uptake
    
    // Time-dependent
    float creep_rate;            // 1/s (long-term deformation under load)
    float fatigue_exponent;      // For S-N curve
    
} UpeMaterialBulk;

// Surface properties
typedef struct UpeMaterialSurface {
    float static_friction;       // Stick coefficient
    float kinetic_friction;      // Slide coefficient
    float rolling_friction;      // For wheels, barrels
    float torsional_friction;    // For spinning contact
    
    float restitution;           // Bounciness (0..1)
    float adhesion;              // Stickiness (N/m² or similar)
    
    float roughness;             // 0..1 (affects friction + contact area)
    float hardness;              // Shore/Rockwell-like (0..1 normalized)
    
    float wear_rate;             // Material loss per sliding distance
    float abrasiveness;          // How much it wears other materials
    
    // Friction anisotropy
    UpeVec3 friction_direction;  // Grain direction for friction
    float friction_anisotropy;   // 1.0 = isotropic
    
} UpeMaterialSurface;

// Thermal properties
typedef struct UpeMaterialThermal {
    float conductivity;          // W/(m·K)
    float heat_capacity;         // J/(kg·K)
    float thermal_expansion;     // 1/K (linear expansion coefficient)
    float emissivity;            // 0..1 (for radiation)
    
    // Phase transitions
    float melting_point;         // K (0 if not relevant)
    float boiling_point;         // K
    float flash_point;           // K (ignition temperature)
    float sublimation_point;     // K (solid→gas)
    
    float latent_heat_fusion;    // J/kg (melting)
    float latent_heat_vaporization; // J/kg (boiling)
    
} UpeMaterialThermal;

// Electrical/Magnetic properties
typedef struct UpeMaterialElectric {
    float electrical_conductivity; // S/m (Siemens per meter)
    float permittivity;            // Relative (ε_r)
    float permeability;            // Relative (μ_r)
    float dielectric_strength;     // V/m (breakdown voltage)
    
} UpeMaterialElectric;

// Chemical properties
typedef struct UpeMaterialChemical {
    float corrosion_rate;        // Oxidation rate in air/water
    float acid_resistance;       // 0..1 (0 = dissolves, 1 = immune)
    float base_resistance;
    float solvent_resistance;
    
    float flammability;          // 0..1 (0 = non-flammable)
    float combustion_heat;       // J/kg (energy release)
    float auto_ignition_temp;    // K
    
    float toxicity;              // 0..1 (for gameplay hazard)
    float reactivity;            // 0..1 (tendency to react)
    
} UpeMaterialChemical;

// Complete material definition
typedef struct UpeMaterial {
    char name[64];
    UpeMaterialID id;
    
    UpeMaterialBulk bulk;
    UpeMaterialSurface surface;
    UpeMaterialThermal thermal;
    UpeMaterialElectric electric;
    UpeMaterialChemical chemical;
    
    // Combine rules (how properties mix at contacts)
    UpeMaterialCombine friction_combine;
    UpeMaterialCombine restitution_combine;
    UpeMaterialCombine adhesion_combine;
    
    // Visual/audio hooks
    float color[3];              // RGB for debug visualization
    float sound_pitch_mult;      // Impact sound pitch multiplier
    
} UpeMaterial;

// Material system (database)
typedef struct UpeMaterialSystem {
    UpeMaterial *materials;
    uint32_t material_count;
    uint32_t material_capacity;
    
    // Fast lookup
    UpeMaterialID *id_to_index;  // Hash map
    
} UpeMaterialSystem;

// Initialization
void UpeMaterial_InitSystem(UpeMaterialSystem *sys);
void UpeMaterial_DestroySystem(UpeMaterialSystem *sys);

// Material registration
UpeMaterialID UpeMaterial_Register(UpeMaterialSystem *sys, const UpeMaterial *material);
const UpeMaterial* UpeMaterial_Get(const UpeMaterialSystem *sys, UpeMaterialID id);

// Property combining (for contacts between different materials)
float UpeMaterial_CombineFriction(const UpeMaterial *a, const UpeMaterial *b);
float UpeMaterial_CombineRestitution(const UpeMaterial *a, const UpeMaterial *b);
float UpeMaterial_CombineAdhesion(const UpeMaterial *a, const UpeMaterial *b);

// Query
UpeMaterialID UpeMaterial_FindByName(const UpeMaterialSystem *sys, const char *name);
void UpeMaterial_PrintAll(const UpeMaterialSystem *sys);

// === 100 MATERIAL PRESETS ===

// Woods (12)
UpeMaterialID UpeMaterial_Oak(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Maple(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Pine(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Cedar(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Birch(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Walnut(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Mahogany(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Teak(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Bamboo(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Balsa(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Plywood(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_MDF(UpeMaterialSystem *sys);

// Metals & alloys (25)
UpeMaterialID UpeMaterial_MildSteel(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_HighCarbonSteel(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_ToolSteel(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_CastIron(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Stainless304(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Stainless316(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Aluminum6061(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Aluminum7075(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_TitaniumTi6Al4V(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Copper(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Brass(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Bronze(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Nickel(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Chromium(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Zinc(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Magnesium(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Lead(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Tin(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Tungsten(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_CobaltAlloy(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Silver(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Gold(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Platinum(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Inconel718(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Nitinol(UpeMaterialSystem *sys);

// Polymers / elastomers / foams (15)
UpeMaterialID UpeMaterial_ABS(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_PLA(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_PETG(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Nylon6(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Nylon12(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Polycarbonate(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_HDPE(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Polypropylene(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_PVC(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_PTFE(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_POM(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_PolyurethFoam(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_NaturalRubber(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_SiliconeRubber(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_EVAFoam(UpeMaterialSystem *sys);

// Ceramics / glass / cementitious (10)
UpeMaterialID UpeMaterial_SodaLimeGlass(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_TemperedGlass(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_BorosilicateGlass(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_AluminaCeramic(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_SiliconCarbideCeramic(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Porcelain(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Concrete(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_ReinforcedConcrete(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Brick(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Gypsum(UpeMaterialSystem *sys);

// Composites (8)
UpeMaterialID UpeMaterial_CarbonFiberEpoxy(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_FiberglassPolyester(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_KevlarEpoxy(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_CarbonAramidHybrid(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_CFRPHoneycomb(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_LaminatedSafetyGlass(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_ReinforcedRubberTire(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_BasaltFiberComposite(UpeMaterialSystem *sys);

// Soils / granular / cryo / ash (10)
UpeMaterialID UpeMaterial_DrySand(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_WetSand(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Clay(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Loam(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Gravel(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Mud(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Permafrost(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Snowpack(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Ice(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_VolcanicAsh(UpeMaterialSystem *sys);

// Liquids (12)
UpeMaterialID UpeMaterial_FreshWater(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Seawater(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_EngineOil(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_HydraulicOil(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Gasoline(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Diesel(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Ethanol(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Glycerin(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Mercury(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_LiquidNitrogen(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_MoltenLava(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_MoltenMetal(UpeMaterialSystem *sys);

// Gases (8)
UpeMaterialID UpeMaterial_Air(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Oxygen(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Nitrogen(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_CarbonDioxide(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Methane(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Hydrogen(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Helium(UpeMaterialSystem *sys);
UpeMaterialID UpeMaterial_Steam(UpeMaterialSystem *sys);

#ifdef __cplusplus
}
#endif
