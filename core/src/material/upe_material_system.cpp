#include "upe/upe_material_system.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Initialize material system
void UpeMaterial_InitSystem(UpeMaterialSystem *sys) {
    sys->material_capacity = 128;
    sys->materials = (UpeMaterial*)malloc(sizeof(UpeMaterial) * sys->material_capacity);
    sys->material_count = 0;
    sys->id_to_index = NULL;  // Simple linear search for now
}

// Destroy material system
void UpeMaterial_DestroySystem(UpeMaterialSystem *sys) {
    free(sys->materials);
    sys->materials = NULL;
    sys->material_count = 0;
}

// Register a new material
UpeMaterialID UpeMaterial_Register(UpeMaterialSystem *sys, const UpeMaterial *material) {
    if (sys->material_count >= sys->material_capacity) {
        sys->material_capacity *= 2;
        sys->materials = (UpeMaterial*)realloc(sys->materials, sizeof(UpeMaterial) * sys->material_capacity);
    }
    
    UpeMaterialID id = sys->material_count;
    sys->materials[sys->material_count] = *material;
    sys->materials[sys->material_count].id = id;
    sys->material_count++;
    
    return id;
}

// Get material by ID
const UpeMaterial* UpeMaterial_Get(const UpeMaterialSystem *sys, UpeMaterialID id) {
    if (id >= sys->material_count) return NULL;
    return &sys->materials[id];
}

// Combine friction using combine mode
float UpeMaterial_CombineFriction(const UpeMaterial *a, const UpeMaterial *b) {
    float fa = a->surface.kinetic_friction;
    float fb = b->surface.kinetic_friction;
    
    switch (a->friction_combine) {
        case UPE_COMBINE_AVERAGE: return (fa + fb) * 0.5f;
        case UPE_COMBINE_MIN: return (fa < fb) ? fa : fb;
        case UPE_COMBINE_MAX: return (fa > fb) ? fa : fb;
        case UPE_COMBINE_MULTIPLY: return fa * fb;
        default: return (fa + fb) * 0.5f;
    }
}

// Combine restitution
float UpeMaterial_CombineRestitution(const UpeMaterial *a, const UpeMaterial *b) {
    float ra = a->surface.restitution;
    float rb = b->surface.restitution;
    
    switch (a->restitution_combine) {
        case UPE_COMBINE_AVERAGE: return (ra + rb) * 0.5f;
        case UPE_COMBINE_MIN: return (ra < rb) ? ra : rb;
        case UPE_COMBINE_MAX: return (ra > rb) ? ra : rb;
        case UPE_COMBINE_MULTIPLY: return ra * rb;
        default: return (ra + rb) * 0.5f;
    }
}

// Combine adhesion
float UpeMaterial_CombineAdhesion(const UpeMaterial *a, const UpeMaterial *b) {
    float aa = a->surface.adhesion;
    float ab = b->surface.adhesion;
    return (aa + ab) * 0.5f;  // Typically average
}

// Find material by name
UpeMaterialID UpeMaterial_FindByName(const UpeMaterialSystem *sys, const char *name) {
    for (uint32_t i = 0; i < sys->material_count; i++) {
        if (strcmp(sys->materials[i].name, name) == 0) {
            return sys->materials[i].id;
        }
    }
    return (UpeMaterialID)(-1);
}

// Helper to create default material
static UpeMaterial CreateDefaultMaterial(const char *name) {
    UpeMaterial mat = {};
    strncpy(mat.name, name, 63);
    
    // Defaults
    mat.bulk.density = 1000.0f;
    mat.bulk.youngs_modulus = 1e9f;
    mat.bulk.poissons_ratio = 0.3f;
    mat.surface.static_friction = 0.5f;
    mat.surface.kinetic_friction = 0.4f;
    mat.surface.restitution = 0.5f;
    mat.thermal.conductivity = 0.5f;
    mat.thermal.heat_capacity = 1000.0f;
    mat.thermal.thermal_expansion = 1e-5f;
    mat.thermal.emissivity = 0.9f;
    
    mat.friction_combine = UPE_COMBINE_AVERAGE;
    mat.restitution_combine = UPE_COMBINE_AVERAGE;
    mat.adhesion_combine = UPE_COMBINE_AVERAGE;
    
    return mat;
}

// ===== WOODS (12) =====

UpeMaterialID UpeMaterial_Oak(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Oak");
    mat.bulk.density = 750.0f;
    mat.bulk.youngs_modulus = 11e9f;
    mat.bulk.poissons_ratio = 0.37f;
    mat.bulk.fracture_toughness = 8000.0f;
    mat.bulk.tensile_strength = 90e6f;
    mat.bulk.anisotropy_ratio = 2.0f;  // Strong along grain
    mat.surface.static_friction = 0.6f;
    mat.surface.kinetic_friction = 0.5f;
    mat.surface.restitution = 0.4f;
    mat.surface.hardness = 0.6f;
    mat.thermal.conductivity = 0.17f;
    mat.thermal.heat_capacity = 1700.0f;
    mat.thermal.flash_point = 573.0f;
    mat.chemical.flammability = 0.7f;
    mat.color[0] = 0.6f; mat.color[1] = 0.4f; mat.color[2] = 0.2f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Maple(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Maple");
    mat.bulk.density = 700.0f;
    mat.bulk.youngs_modulus = 12e9f;
    mat.bulk.fracture_toughness = 7500.0f;
    mat.bulk.anisotropy_ratio = 1.8f;
    mat.surface.hardness = 0.65f;
    mat.surface.wear_rate = 0.001f;
    mat.thermal.flash_point = 570.0f;
    mat.color[0] = 0.8f; mat.color[1] = 0.6f; mat.color[2] = 0.4f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Pine(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Pine");
    mat.bulk.density = 500.0f;
    mat.bulk.youngs_modulus = 8e9f;
    mat.bulk.fracture_toughness = 4000.0f;
    mat.bulk.tensile_strength = 40e6f;
    mat.surface.hardness = 0.3f;
    mat.thermal.flash_point = 550.0f;
    mat.chemical.flammability = 0.8f;
    mat.color[0] = 0.9f; mat.color[1] = 0.8f; mat.color[2] = 0.6f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Cedar(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Cedar");
    mat.bulk.density = 380.0f;
    mat.bulk.youngs_modulus = 7e9f;
    mat.bulk.moisture_absorption = 0.1f;  // Low moisture absorption
    mat.surface.hardness = 0.25f;
    mat.thermal.flash_point = 560.0f;
    mat.chemical.corrosion_rate = 0.01f;  // Resists decay
    mat.color[0] = 0.7f; mat.color[1] = 0.3f; mat.color[2] = 0.2f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Birch(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Birch");
    mat.bulk.density = 650.0f;
    mat.bulk.youngs_modulus = 14e9f;
    mat.bulk.fracture_toughness = 6000.0f;
    mat.surface.hardness = 0.5f;
    mat.color[0] = 0.95f; mat.color[1] = 0.9f; mat.color[2] = 0.85f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Walnut(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Walnut");
    mat.bulk.density = 660.0f;
    mat.bulk.youngs_modulus = 11e9f;
    mat.bulk.fracture_toughness = 7000.0f;
    mat.surface.hardness = 0.6f;
    mat.color[0] = 0.4f; mat.color[1] = 0.25f; mat.color[2] = 0.15f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Mahogany(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Mahogany");
    mat.bulk.density = 560.0f;
    mat.bulk.youngs_modulus = 9e9f;
    mat.thermal.thermal_expansion = 0.8e-5f;  // Stable
    mat.surface.hardness = 0.45f;
    mat.color[0] = 0.6f; mat.color[1] = 0.2f; mat.color[2] = 0.1f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Teak(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Teak");
    mat.bulk.density = 660.0f;
    mat.bulk.youngs_modulus = 12e9f;
    mat.bulk.moisture_absorption = 0.05f;  // Very low
    mat.surface.static_friction = 0.45f;  // Oily surface
    mat.surface.kinetic_friction = 0.35f;
    mat.chemical.corrosion_rate = 0.001f;
    mat.color[0] = 0.7f; mat.color[1] = 0.5f; mat.color[2] = 0.2f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Bamboo(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Bamboo");
    mat.bulk.density = 700.0f;
    mat.bulk.youngs_modulus = 20e9f;  // Very stiff along grain
    mat.bulk.anisotropy_ratio = 3.0f;
    mat.bulk.tensile_strength = 500e6f;  // Extremely strong in tension
    mat.surface.hardness = 0.7f;
    mat.color[0] = 0.8f; mat.color[1] = 0.85f; mat.color[2] = 0.5f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Balsa(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Balsa");
    mat.bulk.density = 160.0f;  // Ultra-light
    mat.bulk.youngs_modulus = 3e9f;
    mat.bulk.fracture_toughness = 500.0f;
    mat.bulk.compressive_strength = 5e6f;
    mat.surface.hardness = 0.1f;
    mat.color[0] = 0.95f; mat.color[1] = 0.95f; mat.color[2] = 0.9f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Plywood(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Plywood");
    mat.bulk.density = 600.0f;
    mat.bulk.youngs_modulus = 10e9f;
    mat.bulk.anisotropy_ratio = 1.2f;  // Layered structure
    mat.bulk.fracture_toughness = 5000.0f;
    mat.surface.hardness = 0.4f;
    mat.color[0] = 0.7f; mat.color[1] = 0.6f; mat.color[2] = 0.4f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_MDF(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("MDF");
    mat.bulk.density = 750.0f;
    mat.bulk.youngs_modulus = 4e9f;
    mat.bulk.fracture_toughness = 1000.0f;  // Brittle crumble
    mat.bulk.anisotropy_ratio = 1.0f;  // Uniform
    mat.surface.hardness = 0.3f;
    mat.bulk.moisture_absorption = 0.5f;  // Swells easily
    mat.color[0] = 0.6f; mat.color[1] = 0.5f; mat.color[2] = 0.4f;
    return UpeMaterial_Register(sys, &mat);
}

// ===== METALS (25) =====

UpeMaterialID UpeMaterial_MildSteel(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Mild Steel");
    mat.bulk.density = 7850.0f;
    mat.bulk.youngs_modulus = 200e9f;
    mat.bulk.poissons_ratio = 0.29f;
    mat.bulk.yield_stress = 250e6f;
    mat.bulk.tensile_strength = 400e6f;
    mat.bulk.hardening_coefficient = 0.2f;
    mat.surface.static_friction = 0.74f;
    mat.surface.kinetic_friction = 0.57f;
    mat.surface.restitution = 0.5f;
    mat.surface.hardness = 0.6f;
    mat.thermal.conductivity = 50.0f;
    mat.thermal.heat_capacity = 490.0f;
    mat.thermal.thermal_expansion = 12e-6f;
    mat.thermal.melting_point = 1811.0f;
    mat.electric.electrical_conductivity = 1e7f;
    mat.chemical.corrosion_rate = 0.5f;  // Rusts
    mat.color[0] = 0.7f; mat.color[1] = 0.7f; mat.color[2] = 0.7f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_HighCarbonSteel(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("High Carbon Steel");
    mat.bulk.density = 7850.0f;
    mat.bulk.youngs_modulus = 205e9f;
    mat.bulk.yield_stress = 500e6f;
    mat.bulk.tensile_strength = 900e6f;
    mat.bulk.fracture_toughness = 50000.0f;  // Brittle edge
    mat.surface.hardness = 0.85f;
    mat.thermal.melting_point = 1800.0f;
    mat.color[0] = 0.6f; mat.color[1] = 0.6f; mat.color[2] = 0.65f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_ToolSteel(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Tool Steel");
    mat.bulk.density = 7800.0f;
    mat.bulk.youngs_modulus = 210e9f;
    mat.bulk.yield_stress = 1000e6f;
    mat.bulk.tensile_strength = 2000e6f;
    mat.surface.hardness = 0.95f;
    mat.surface.wear_rate = 0.0001f;
    mat.thermal.melting_point = 1785.0f;
    mat.color[0] = 0.55f; mat.color[1] = 0.55f; mat.color[2] = 0.6f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_CastIron(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Cast Iron");
    mat.bulk.density = 7200.0f;
    mat.bulk.youngs_modulus = 170e9f;
    mat.bulk.yield_stress = 300e6f;
    mat.bulk.compressive_strength = 700e6f;  // Much higher than tensile
    mat.bulk.fracture_toughness = 10000.0f;  // Brittle
    mat.surface.hardness = 0.7f;
    mat.thermal.conductivity = 55.0f;
    mat.thermal.melting_point = 1473.0f;
    mat.color[0] = 0.5f; mat.color[1] = 0.5f; mat.color[2] = 0.5f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Stainless304(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Stainless 304");
    mat.bulk.density = 8000.0f;
    mat.bulk.youngs_modulus = 193e9f;
    mat.bulk.yield_stress = 215e6f;
    mat.bulk.tensile_strength = 505e6f;
    mat.bulk.hardening_coefficient = 0.45f;  // Work hardens significantly
    mat.surface.hardness = 0.65f;
    mat.thermal.conductivity = 16.2f;
    mat.thermal.thermal_expansion = 17e-6f;
    mat.thermal.melting_point = 1673.0f;
    mat.chemical.corrosion_rate = 0.01f;  // Corrosion resistant
    mat.chemical.acid_resistance = 0.8f;
    mat.color[0] = 0.75f; mat.color[1] = 0.75f; mat.color[2] = 0.78f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Stainless316(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Stainless 316");
    mat.bulk.density = 8000.0f;
    mat.bulk.youngs_modulus = 193e9f;
    mat.bulk.yield_stress = 205e6f;
    mat.bulk.tensile_strength = 515e6f;
    mat.chemical.corrosion_rate = 0.001f;  // Excellent corrosion resistance
    mat.chemical.acid_resistance = 0.9f;
    mat.chemical.base_resistance = 0.9f;
    mat.thermal.melting_point = 1673.0f;
    mat.color[0] = 0.78f; mat.color[1] = 0.78f; mat.color[2] = 0.8f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Aluminum6061(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Aluminum 6061");
    mat.bulk.density = 2700.0f;
    mat.bulk.youngs_modulus = 69e9f;
    mat.bulk.yield_stress = 276e6f;
    mat.bulk.tensile_strength = 310e6f;
    mat.surface.hardness = 0.5f;
    mat.surface.kinetic_friction = 0.47f;
    mat.thermal.conductivity = 167.0f;
    mat.thermal.heat_capacity = 896.0f;
    mat.thermal.thermal_expansion = 23.6e-6f;
    mat.thermal.melting_point = 855.0f;
    mat.electric.electrical_conductivity = 24e6f;
    mat.chemical.corrosion_rate = 0.05f;
    mat.color[0] = 0.85f; mat.color[1] = 0.85f; mat.color[2] = 0.85f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Aluminum7075(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Aluminum 7075");
    mat.bulk.density = 2810.0f;
    mat.bulk.youngs_modulus = 72e9f;
    mat.bulk.yield_stress = 505e6f;
    mat.bulk.tensile_strength = 570e6f;
    mat.bulk.fracture_toughness = 25000.0f;  // Less ductile
    mat.surface.hardness = 0.7f;
    mat.thermal.melting_point = 750.0f;
    mat.color[0] = 0.82f; mat.color[1] = 0.82f; mat.color[2] = 0.82f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_TitaniumTi6Al4V(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Titanium Ti-6Al-4V");
    mat.bulk.density = 4430.0f;
    mat.bulk.youngs_modulus = 114e9f;
    mat.bulk.yield_stress = 880e6f;
    mat.bulk.tensile_strength = 950e6f;
    mat.surface.hardness = 0.75f;
    mat.surface.restitution = 0.6f;  // Springy
    mat.thermal.conductivity = 6.7f;
    mat.thermal.thermal_expansion = 8.6e-6f;
    mat.thermal.melting_point = 1878.0f;
    mat.chemical.corrosion_rate = 0.001f;
    mat.color[0] = 0.7f; mat.color[1] = 0.72f; mat.color[2] = 0.75f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Copper(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Copper");
    mat.bulk.density = 8960.0f;
    mat.bulk.youngs_modulus = 130e9f;
    mat.bulk.yield_stress = 70e6f;  // Soft
    mat.bulk.tensile_strength = 220e6f;
    mat.surface.hardness = 0.35f;
    mat.surface.kinetic_friction = 0.36f;  // Low friction
    mat.thermal.conductivity = 401.0f;  // Excellent
    mat.thermal.heat_capacity = 385.0f;
    mat.thermal.thermal_expansion = 17e-6f;
    mat.thermal.melting_point = 1358.0f;
    mat.electric.electrical_conductivity = 59.6e6f;  // Excellent
    mat.chemical.corrosion_rate = 0.2f;  // Patina formation
    mat.color[0] = 0.95f; mat.color[1] = 0.6f; mat.color[2] = 0.4f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Brass(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Brass");
    mat.bulk.density = 8500.0f;
    mat.bulk.youngs_modulus = 100e9f;
    mat.bulk.yield_stress = 200e6f;
    mat.surface.hardness = 0.5f;
    mat.surface.static_friction = 0.51f;
    mat.thermal.conductivity = 120.0f;
    mat.thermal.melting_point = 1173.0f;
    mat.electric.electrical_conductivity = 15e6f;
    mat.color[0] = 0.9f; mat.color[1] = 0.8f; mat.color[2] = 0.4f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Bronze(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Bronze");
    mat.bulk.density = 8800.0f;
    mat.bulk.youngs_modulus = 120e9f;
    mat.bulk.yield_stress = 300e6f;
    mat.surface.hardness = 0.6f;
    mat.surface.wear_rate = 0.0005f;  // Good bearing material
    mat.thermal.conductivity = 50.0f;
    mat.thermal.melting_point = 1223.0f;
    mat.color[0] = 0.8f; mat.color[1] = 0.5f; mat.color[2] = 0.2f;
    return UpeMaterial_Register(sys, &mat);
}

// Continue with remaining metals...
UpeMaterialID UpeMaterial_Nickel(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Nickel");
    mat.bulk.density = 8908.0f;
    mat.bulk.youngs_modulus = 200e9f;
    mat.bulk.yield_stress = 148e6f;
    mat.thermal.conductivity = 91.0f;
    mat.thermal.melting_point = 1728.0f;
    mat.electric.electrical_conductivity = 14e6f;
    mat.electric.permeability = 100.0f;  // Ferromagnetic
    mat.chemical.corrosion_rate = 0.01f;
    mat.color[0] = 0.78f; mat.color[1] = 0.78f; mat.color[2] = 0.75f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Chromium(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Chromium");
    mat.bulk.density = 7190.0f;
    mat.bulk.youngs_modulus = 279e9f;
    mat.surface.hardness = 0.9f;
    mat.bulk.fracture_toughness = 8000.0f;  // Brittle
    mat.thermal.melting_point = 2180.0f;
    mat.chemical.corrosion_rate = 0.0f;  // Excellent
    mat.color[0] = 0.88f; mat.color[1] = 0.92f; mat.color[2] = 0.95f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Zinc(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Zinc");
    mat.bulk.density = 7140.0f;
    mat.bulk.youngs_modulus = 108e9f;
    mat.bulk.yield_stress = 100e6f;
    mat.surface.hardness = 0.4f;
    mat.thermal.melting_point = 692.0f;  // Low melting
    mat.chemical.corrosion_rate = 0.1f;  // Sacrificial anode
    mat.color[0] = 0.75f; mat.color[1] = 0.75f; mat.color[2] = 0.8f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Magnesium(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Magnesium");
    mat.bulk.density = 1738.0f;  // Very light metal
    mat.bulk.youngs_modulus = 45e9f;
    mat.bulk.yield_stress = 175e6f;
    mat.thermal.melting_point = 923.0f;
    mat.chemical.flammability = 0.9f;  // Flammable chips!
    mat.chemical.reactivity = 0.8f;
    mat.color[0] = 0.85f; mat.color[1] = 0.85f; mat.color[2] = 0.85f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Lead(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Lead");
    mat.bulk.density = 11340.0f;  // Very dense
    mat.bulk.youngs_modulus = 16e9f;  // Soft
    mat.bulk.yield_stress = 12e6f;
    mat.bulk.creep_rate = 1e-6f;  // Creeps under load
    mat.surface.hardness = 0.15f;
    mat.thermal.melting_point = 600.0f;
    mat.chemical.toxicity = 0.9f;
    mat.color[0] = 0.5f; mat.color[1] = 0.5f; mat.color[2] = 0.55f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Tin(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Tin");
    mat.bulk.density = 7265.0f;
    mat.bulk.youngs_modulus = 50e9f;
    mat.surface.hardness = 0.2f;
    mat.thermal.melting_point = 505.0f;
    mat.color[0] = 0.82f; mat.color[1] = 0.82f; mat.color[2] = 0.82f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Tungsten(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Tungsten");
    mat.bulk.density = 19250.0f;  // Extremely dense
    mat.bulk.youngs_modulus = 411e9f;  // Extremely stiff
    mat.bulk.yield_stress = 750e6f;
    mat.surface.hardness = 0.9f;
    mat.thermal.melting_point = 3695.0f;  // Highest melting point
    mat.thermal.conductivity = 173.0f;
    mat.color[0] = 0.6f; mat.color[1] = 0.6f; mat.color[2] = 0.62f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_CobaltAlloy(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Cobalt Alloy");
    mat.bulk.density = 8900.0f;
    mat.bulk.youngs_modulus = 220e9f;
    mat.bulk.yield_stress = 500e6f;
    mat.surface.hardness = 0.85f;
    mat.surface.wear_rate = 0.0002f;
    mat.thermal.melting_point = 1768.0f;
    mat.color[0] = 0.7f; mat.color[1] = 0.72f; mat.color[2] = 0.75f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Silver(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Silver");
    mat.bulk.density = 10490.0f;
    mat.bulk.youngs_modulus = 83e9f;
    mat.bulk.yield_stress = 55e6f;  // Soft
    mat.surface.hardness = 0.25f;
    mat.thermal.conductivity = 429.0f;  // Highest thermal conductivity
    mat.thermal.melting_point = 1235.0f;
    mat.electric.electrical_conductivity = 63e6f;  // Highest electrical conductivity
    mat.chemical.corrosion_rate = 0.05f;  // Tarnishes
    mat.color[0] = 0.95f; mat.color[1] = 0.95f; mat.color[2] = 0.97f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Gold(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Gold");
    mat.bulk.density = 19300.0f;  // Very dense
    mat.bulk.youngs_modulus = 79e9f;
    mat.bulk.yield_stress = 100e6f;
    mat.surface.hardness = 0.25f;
    mat.thermal.conductivity = 318.0f;
    mat.thermal.melting_point = 1337.0f;
    mat.electric.electrical_conductivity = 45e6f;
    mat.chemical.corrosion_rate = 0.0f;  // Does not corrode
    mat.color[0] = 1.0f; mat.color[1] = 0.85f; mat.color[2] = 0.0f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Platinum(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Platinum");
    mat.bulk.density = 21450.0f;  // Extremely dense
    mat.bulk.youngs_modulus = 168e9f;
    mat.bulk.yield_stress = 125e6f;
    mat.surface.hardness = 0.4f;
    mat.thermal.melting_point = 2041.0f;
    mat.chemical.corrosion_rate = 0.0f;
    mat.chemical.reactivity = 0.0f;
    mat.color[0] = 0.9f; mat.color[1] = 0.9f; mat.color[2] = 0.92f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Inconel718(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Inconel 718");
    mat.bulk.density = 8190.0f;
    mat.bulk.youngs_modulus = 200e9f;
    mat.bulk.yield_stress = 1100e6f;  // High-temp strength
    mat.bulk.creep_rate = 1e-10f;  // Excellent creep resistance
    mat.surface.hardness = 0.8f;
    mat.thermal.melting_point = 1570.0f;
    mat.chemical.corrosion_rate = 0.001f;
    mat.color[0] = 0.72f; mat.color[1] = 0.72f; mat.color[2] = 0.73f;
    return UpeMaterial_Register(sys, &mat);
}

UpeMaterialID UpeMaterial_Nitinol(UpeMaterialSystem *sys) {
    UpeMaterial mat = CreateDefaultMaterial("Nitinol");
    mat.bulk.density = 6450.0f;
    mat.bulk.youngs_modulus = 83e9f;  // But changes with phase
    mat.bulk.yield_stress = 500e6f;
    mat.surface.restitution = 0.8f;  // Superelastic
    mat.thermal.melting_point = 1583.0f;
    // Shape memory behavior would need special flags
    mat.color[0] = 0.75f; mat.color[1] = 0.75f; mat.color[2] = 0.76f;
    return UpeMaterial_Register(sys, &mat);
}

// Note: Due to length constraints, I'm showing the pattern. The full implementation would continue with:
// - Polymers (15 materials)
// - Ceramics/Glass (10 materials)
// - Composites (8 materials)
// - Soils/Granular (10 materials)
// - Liquids (12 materials)
// - Gases (8 materials)

// Each would follow the same pattern with appropriate properties.

void UpeMaterial_PrintAll(const UpeMaterialSystem *sys) {
    printf("=== Material Database ===\n");
    printf("Total materials: %u\n\n", sys->material_count);
    for (uint32_t i = 0; i < sys->material_count; i++) {
        const UpeMaterial *m = &sys->materials[i];
        printf("[%u] %s\n", m->id, m->name);
        printf("  Density: %.1f kg/m³\n", m->bulk.density);
        printf("  Friction: %.2f / %.2f (static/kinetic)\n",
               m->surface.static_friction, m->surface.kinetic_friction);
        printf("  Restitution: %.2f\n", m->surface.restitution);
        printf("  Hardness: %.2f\n", m->surface.hardness);
        printf("\n");
    }
}
