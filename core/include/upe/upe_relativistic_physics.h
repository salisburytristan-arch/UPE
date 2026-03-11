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

/* Returns 1 if object at radius r is inside the Schwarzschild radius */
int   UpeAstro_EventHorizonCapture(float r, float mass, float G, float c);

/* Tidal spaghettification: differential acceleration across body_length at radius r */
float UpeAstro_Spaghettification(float mass, float r, float G, float c,
											  float body_length);

/* Einstein ring / gravitational lensing deflection angle [rad]
	b_impact: closest approach distance to mass */
float UpeAstro_GravitationalLensAngle(float mass, float G, float c, float b_impact);

/* Morris-Thorne wormhole stub: approximate traversal time through throat */
float UpeAstro_WormholeTransitTime(float throat_radius, float c);

#ifdef __cplusplus
}
#endif
