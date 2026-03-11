#pragma once

#ifdef __cplusplus
extern "C" {
#endif

float UpeRelativity_LorentzFactor(float velocity, float light_speed);
float UpeRelativity_TimeDilation(float proper_time, float velocity, float light_speed);
float UpeRelativity_LengthContraction(float proper_length, float velocity, float light_speed);
float UpeRelativity_RelativisticKineticEnergy(float mass, float velocity, float light_speed);
float UpeRelativity_RelativisticMomentum(float mass, float velocity, float light_speed);
float UpeRelativity_VelocityAddition(float v1, float v2, float light_speed);

float UpeAstro_SchwarzschildRadius(float mass, float gravitational_constant, float light_speed);
float UpeAstro_GravitationalTimeDilationFactor(float radius, float mass, float gravitational_constant, float light_speed);
float UpeAstro_GravitationalRedshift(float emitted_wavelength, float radius, float mass, float gravitational_constant, float light_speed);

#ifdef __cplusplus
}
#endif
