#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UpeUnits {
    float length_scale; /**< Meters represented by one engine length unit */
    float mass_scale;   /**< Kilograms represented by one engine mass unit */
    float time_scale;   /**< Seconds represented by one engine time unit */
} UpeUnits;

typedef struct UpePhysicalConstants {
    double standard_gravity;
    double speed_of_light;
    double boltzmann_constant;
    double avogadro_constant;
    double universal_gas_constant;
    double stefan_boltzmann_constant;
    double standard_air_density_kg_per_m3;
    double dynamic_viscosity_air_pascal_seconds;
    double speed_of_sound_air;
} UpePhysicalConstants;

UpeUnits UpeUnits_SI(void);
UpeUnits UpeUnits_FromScales(float length_scale, float mass_scale, float time_scale);
int UpeUnits_IsValid(const UpeUnits *units);
UpePhysicalConstants UpePhysicalConstants_SI(void);

float UpeUnits_LengthToSI(const UpeUnits *units, float value);
float UpeUnits_MassToSI(const UpeUnits *units, float value);
float UpeUnits_TimeToSI(const UpeUnits *units, float value);
float UpeUnits_SpeedToSI(const UpeUnits *units, float value);
float UpeUnits_AccelerationToSI(const UpeUnits *units, float value);
float UpeUnits_ForceToSI(const UpeUnits *units, float value);
float UpeUnits_EnergyToSI(const UpeUnits *units, float value);
float UpeUnits_MomentumToSI(const UpeUnits *units, float value);

#ifdef __cplusplus
}
#endif
