#include "upe/upe_orbital_mechanics.h"

#include <math.h>

static float UpeOrbital_ClampPositive(float value) {
    return (value > 0.0f) ? value : 0.0f;
}

float UpeOrbital_GravitationalParameter(float gravitational_constant, float central_mass) {
    if (gravitational_constant <= 0.0f || central_mass <= 0.0f) {
        return 0.0f;
    }
    return gravitational_constant * central_mass;
}

float UpeOrbital_CircularOrbitSpeed(float gravitational_parameter, float radius) {
    if (gravitational_parameter <= 0.0f || radius <= 0.0f) {
        return 0.0f;
    }
    return sqrtf(gravitational_parameter / radius);
}

float UpeOrbital_EscapeSpeed(float gravitational_parameter, float radius) {
    if (gravitational_parameter <= 0.0f || radius <= 0.0f) {
        return 0.0f;
    }
    return sqrtf((2.0f * gravitational_parameter) / radius);
}

float UpeOrbital_VisVivaSpeed(float gravitational_parameter, float radius, float semi_major_axis) {
    if (gravitational_parameter <= 0.0f || radius <= 0.0f || semi_major_axis <= 0.0f) {
        return 0.0f;
    }
    float term = (2.0f / radius) - (1.0f / semi_major_axis);
    if (term <= 0.0f) {
        return 0.0f;
    }
    return sqrtf(gravitational_parameter * term);
}

float UpeOrbital_OrbitalPeriod(float gravitational_parameter, float semi_major_axis) {
    if (gravitational_parameter <= 0.0f || semi_major_axis <= 0.0f) {
        return 0.0f;
    }
    float a3 = semi_major_axis * semi_major_axis * semi_major_axis;
    float n = sqrtf(gravitational_parameter / a3);
    if (n <= 0.0f) {
        return 0.0f;
    }
    return (2.0f * 3.14159265358979323846f) / n;
}

float UpeOrbital_HohmannDeltaV1(float gravitational_parameter, float radius_from, float radius_to) {
    if (gravitational_parameter <= 0.0f || radius_from <= 0.0f || radius_to <= 0.0f) {
        return 0.0f;
    }
    float a_transfer = 0.5f * (radius_from + radius_to);
    float v_circular_from = UpeOrbital_CircularOrbitSpeed(gravitational_parameter, radius_from);
    float v_transfer_periapsis = UpeOrbital_VisVivaSpeed(gravitational_parameter, radius_from, a_transfer);
    return UpeOrbital_ClampPositive(fabsf(v_transfer_periapsis - v_circular_from));
}

float UpeOrbital_HohmannDeltaV2(float gravitational_parameter, float radius_from, float radius_to) {
    if (gravitational_parameter <= 0.0f || radius_from <= 0.0f || radius_to <= 0.0f) {
        return 0.0f;
    }
    float a_transfer = 0.5f * (radius_from + radius_to);
    float v_circular_to = UpeOrbital_CircularOrbitSpeed(gravitational_parameter, radius_to);
    float v_transfer_apoapsis = UpeOrbital_VisVivaSpeed(gravitational_parameter, radius_to, a_transfer);
    return UpeOrbital_ClampPositive(fabsf(v_circular_to - v_transfer_apoapsis));
}

float UpeOrbital_MeanMotion(float gravitational_parameter, float semi_major_axis) {
    if (gravitational_parameter <= 0.0f || semi_major_axis <= 0.0f) {
        return 0.0f;
    }
    float a3 = semi_major_axis * semi_major_axis * semi_major_axis;
    if (a3 <= 0.0f) {
        return 0.0f;
    }
    return sqrtf(gravitational_parameter / a3);
}

float UpeOrbital_SynodicPeriod(float period_a, float period_b) {
    if (period_a <= 0.0f || period_b <= 0.0f) {
        return 0.0f;
    }
    float rate_delta = fabsf((1.0f / period_a) - (1.0f / period_b));
    if (rate_delta <= 1e-12f) {
        return 0.0f;
    }
    return 1.0f / rate_delta;
}

float UpeOrbital_SphereOfInfluenceRadius(float primary_mass, float secondary_mass, float semi_major_axis) {
    if (primary_mass <= 0.0f || secondary_mass <= 0.0f || semi_major_axis <= 0.0f) {
        return 0.0f;
    }
    float mass_ratio = secondary_mass / primary_mass;
    if (mass_ratio <= 0.0f) {
        return 0.0f;
    }
    return semi_major_axis * powf(mass_ratio, 0.4f);
}

float UpeOrbital_HillSphereRadius(float primary_mass, float secondary_mass, float semi_major_axis) {
    if (primary_mass <= 0.0f || secondary_mass <= 0.0f || semi_major_axis <= 0.0f) {
        return 0.0f;
    }
    float mass_ratio = secondary_mass / (3.0f * primary_mass);
    if (mass_ratio <= 0.0f) {
        return 0.0f;
    }
    return semi_major_axis * cbrtf(mass_ratio);
}

UpeVec3 UpeOrbital_CircularVelocityVector(UpeVec3 central_position,
                                          UpeVec3 body_position,
                                          UpeVec3 orbit_normal,
                                          float gravitational_parameter,
                                          int prograde) {
    UpeVec3 zero = {0.0f, 0.0f, 0.0f};
    UpeVec3 radius_vec = UpeVec3_Sub(body_position, central_position);
    float radius = UpeVec3_Length(radius_vec);
    if (radius <= 0.0f || gravitational_parameter <= 0.0f) {
        return zero;
    }

    UpeVec3 radial_dir = UpeVec3_Scale(radius_vec, 1.0f / radius);
    UpeVec3 normal_dir = UpeVec3_Normalize(orbit_normal);
    if (UpeVec3_Length(normal_dir) <= 0.0f) {
        return zero;
    }

    UpeVec3 tangent_dir = UpeVec3_Cross(normal_dir, radial_dir);
    tangent_dir = UpeVec3_Normalize(tangent_dir);
    if (UpeVec3_Length(tangent_dir) <= 0.0f) {
        return zero;
    }

    if (!prograde) {
        tangent_dir = UpeVec3_Scale(tangent_dir, -1.0f);
    }

    float speed = UpeOrbital_CircularOrbitSpeed(gravitational_parameter, radius);
    return UpeVec3_Scale(tangent_dir, speed);
}

int UpeOrbital_ComputeCircularState(UpeVec3 central_position,
                                    UpeVec3 body_position,
                                    UpeVec3 orbit_normal,
                                    float gravitational_parameter,
                                    int prograde,
                                    UpeVec3 *out_velocity) {
    if (!out_velocity) {
        return 0;
    }

    UpeVec3 velocity = UpeOrbital_CircularVelocityVector(
        central_position,
        body_position,
        orbit_normal,
        gravitational_parameter,
        prograde);

    if (UpeVec3_Length(velocity) <= 0.0f) {
        *out_velocity = {0.0f, 0.0f, 0.0f};
        return 0;
    }

    *out_velocity = velocity;
    return 1;
}
