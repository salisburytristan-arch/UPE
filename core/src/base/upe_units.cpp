#include "upe/upe_units.h"

#include <math.h>

static UpeUnits UpeUnits_SafeDefaults(void) {
    UpeUnits units;
    units.length_scale = 1.0f;
    units.mass_scale = 1.0f;
    units.time_scale = 1.0f;
    return units;
}

UpeUnits UpeUnits_SI(void) {
    return UpeUnits_SafeDefaults();
}

UpeUnits UpeUnits_FromScales(float length_scale, float mass_scale, float time_scale) {
    UpeUnits units = UpeUnits_SafeDefaults();
    if (isfinite(length_scale) && length_scale > 0.0f) {
        units.length_scale = length_scale;
    }
    if (isfinite(mass_scale) && mass_scale > 0.0f) {
        units.mass_scale = mass_scale;
    }
    if (isfinite(time_scale) && time_scale > 0.0f) {
        units.time_scale = time_scale;
    }
    return units;
}

int UpeUnits_IsValid(const UpeUnits *units) {
    return units &&
           isfinite(units->length_scale) && units->length_scale > 0.0f &&
           isfinite(units->mass_scale) && units->mass_scale > 0.0f &&
           isfinite(units->time_scale) && units->time_scale > 0.0f;
}

UpePhysicalConstants UpePhysicalConstants_SI(void) {
    UpePhysicalConstants constants = {};
    constants.standard_gravity = 9.80665;
    constants.speed_of_light = 299792458.0;
    constants.boltzmann_constant = 1.380649e-23;
    constants.avogadro_constant = 6.02214076e23;
    constants.universal_gas_constant = 8.314462618;
    constants.stefan_boltzmann_constant = 5.670374419e-8;
    constants.standard_air_density_kg_per_m3 = 1.225;
    constants.dynamic_viscosity_air_pascal_seconds = 1.81e-5;
    constants.speed_of_sound_air = 343.0;
    return constants;
}

float UpeUnits_LengthToSI(const UpeUnits *units, float value) {
    UpeUnits safe = UpeUnits_IsValid(units) ? *units : UpeUnits_SafeDefaults();
    return value * safe.length_scale;
}

float UpeUnits_MassToSI(const UpeUnits *units, float value) {
    UpeUnits safe = UpeUnits_IsValid(units) ? *units : UpeUnits_SafeDefaults();
    return value * safe.mass_scale;
}

float UpeUnits_TimeToSI(const UpeUnits *units, float value) {
    UpeUnits safe = UpeUnits_IsValid(units) ? *units : UpeUnits_SafeDefaults();
    return value * safe.time_scale;
}

float UpeUnits_SpeedToSI(const UpeUnits *units, float value) {
    UpeUnits safe = UpeUnits_IsValid(units) ? *units : UpeUnits_SafeDefaults();
    return value * safe.length_scale / safe.time_scale;
}

float UpeUnits_AccelerationToSI(const UpeUnits *units, float value) {
    UpeUnits safe = UpeUnits_IsValid(units) ? *units : UpeUnits_SafeDefaults();
    return value * safe.length_scale / (safe.time_scale * safe.time_scale);
}

float UpeUnits_ForceToSI(const UpeUnits *units, float value) {
    UpeUnits safe = UpeUnits_IsValid(units) ? *units : UpeUnits_SafeDefaults();
    return value * safe.mass_scale * safe.length_scale / (safe.time_scale * safe.time_scale);
}

float UpeUnits_EnergyToSI(const UpeUnits *units, float value) {
    UpeUnits safe = UpeUnits_IsValid(units) ? *units : UpeUnits_SafeDefaults();
    return value * safe.mass_scale * safe.length_scale * safe.length_scale /
           (safe.time_scale * safe.time_scale);
}

float UpeUnits_MomentumToSI(const UpeUnits *units, float value) {
    UpeUnits safe = UpeUnits_IsValid(units) ? *units : UpeUnits_SafeDefaults();
    return value * safe.mass_scale * safe.length_scale / safe.time_scale;
}
