#include "upe/upe_relativistic_physics.h"

#include <math.h>

static float UpeRelativity_SafeLightSpeed(float light_speed) {
    return (light_speed > 0.0f) ? light_speed : 299792458.0f;
}

float UpeRelativity_LorentzFactor(float velocity, float light_speed) {
    float c = UpeRelativity_SafeLightSpeed(light_speed);
    float beta = velocity / c;
    float beta2 = beta * beta;
    if (beta2 >= 1.0f) {
        return 0.0f;
    }
    return 1.0f / sqrtf(1.0f - beta2);
}

float UpeRelativity_TimeDilation(float proper_time, float velocity, float light_speed) {
    if (proper_time < 0.0f) {
        return 0.0f;
    }
    float gamma = UpeRelativity_LorentzFactor(velocity, light_speed);
    if (gamma <= 0.0f) {
        return 0.0f;
    }
    return proper_time * gamma;
}

float UpeRelativity_LengthContraction(float proper_length, float velocity, float light_speed) {
    if (proper_length < 0.0f) {
        return 0.0f;
    }
    float gamma = UpeRelativity_LorentzFactor(velocity, light_speed);
    if (gamma <= 0.0f) {
        return 0.0f;
    }
    return proper_length / gamma;
}

float UpeRelativity_RelativisticKineticEnergy(float mass, float velocity, float light_speed) {
    if (mass <= 0.0f) {
        return 0.0f;
    }
    float c = UpeRelativity_SafeLightSpeed(light_speed);
    float gamma = UpeRelativity_LorentzFactor(velocity, c);
    if (gamma <= 0.0f) {
        return 0.0f;
    }
    return (gamma - 1.0f) * mass * c * c;
}

float UpeRelativity_RelativisticMomentum(float mass, float velocity, float light_speed) {
    if (mass <= 0.0f) {
        return 0.0f;
    }
    float gamma = UpeRelativity_LorentzFactor(velocity, light_speed);
    if (gamma <= 0.0f) {
        return 0.0f;
    }
    return gamma * mass * velocity;
}

float UpeRelativity_VelocityAddition(float v1, float v2, float light_speed) {
    float c = UpeRelativity_SafeLightSpeed(light_speed);
    float denom = 1.0f + ((v1 * v2) / (c * c));
    if (fabsf(denom) < 1e-12f) {
        return 0.0f;
    }
    return (v1 + v2) / denom;
}

float UpeAstro_SchwarzschildRadius(float mass, float gravitational_constant, float light_speed) {
    if (mass <= 0.0f || gravitational_constant <= 0.0f) {
        return 0.0f;
    }
    float c = UpeRelativity_SafeLightSpeed(light_speed);
    return (2.0f * gravitational_constant * mass) / (c * c);
}

float UpeAstro_GravitationalTimeDilationFactor(float radius, float mass, float gravitational_constant, float light_speed) {
    if (radius <= 0.0f || mass <= 0.0f || gravitational_constant <= 0.0f) {
        return 0.0f;
    }
    float rs = UpeAstro_SchwarzschildRadius(mass, gravitational_constant, light_speed);
    if (radius <= rs) {
        return 0.0f;
    }
    return sqrtf(1.0f - (rs / radius));
}

float UpeAstro_GravitationalRedshift(float emitted_wavelength, float radius, float mass, float gravitational_constant, float light_speed) {
    if (emitted_wavelength <= 0.0f) {
        return 0.0f;
    }
    float dilation = UpeAstro_GravitationalTimeDilationFactor(radius, mass, gravitational_constant, light_speed);
    if (dilation <= 0.0f) {
        return 0.0f;
    }
    return emitted_wavelength / dilation;
}
