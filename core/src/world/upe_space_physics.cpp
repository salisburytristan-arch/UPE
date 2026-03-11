#include "upe/upe_space_physics.h"
#include <math.h>

float UpeSpace_SolarWindPressure(float luminosity_W, float c, float distance_m) {
    if (luminosity_W <= 0.0f || c <= 0.0f || distance_m <= 0.0f) return 0.0f;
    /* Radiation pressure at distance r: P = L / (4*pi*r^2*c) */
    return luminosity_W / (4.0f * 3.14159265f * distance_m * distance_m * c);
}

float UpeSpace_JeansAccretionRate(float density, float sound_speed, float G, float mass) {
    if (density <= 0.0f || sound_speed <= 0.0f || G <= 0.0f || mass <= 0.0f) return 0.0f;
    /* Bondi-like accretion approximation */
    return 4.0f * 3.14159265f * density * G * G * mass * mass / (sound_speed * sound_speed * sound_speed);
}

float UpeSpace_SedovTaylorBlastRadius(float E_blast, float rho_ambient, float time_s) {
    if (E_blast <= 0.0f || rho_ambient <= 0.0f || time_s <= 0.0f) return 0.0f;
    /* R = xi * (E/rho)^(1/5) * t^(2/5), xi ~ 1.0 for gamma=5/3 */
    return powf(E_blast / rho_ambient, 0.2f) * powf(time_s, 0.4f);
}

float UpeSpace_HubbleRecessionalVelocity(float H0_km_s_Mpc, float distance_Mpc) {
    if (H0_km_s_Mpc <= 0.0f || distance_Mpc <= 0.0f) return 0.0f;
    return H0_km_s_Mpc * distance_Mpc;
}
