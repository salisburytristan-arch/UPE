#include "test_framework.h"
#include "upe/upe_orbital_mechanics.h"
#include "upe/upe_string_theory.h"

#include <math.h>

TEST(TestOrbitalMeanMotionPeriodConsistency) {
    const float mu_earth = 3.9860044e14f;
    const float r_geo = 4.2164e7f;

    float mean_motion = UpeOrbital_MeanMotion(mu_earth, r_geo);
    float period = UpeOrbital_OrbitalPeriod(mu_earth, r_geo);

    REQUIRE(mean_motion > 0.0f);
    REQUIRE(period > 0.0f);

    float two_pi = 6.28318530718f;
    float reconstructed = mean_motion * period;
    REQUIRE(fabsf(reconstructed - two_pi) < 0.02f);
}

TEST(TestOrbitalSynodicAndInfluenceRadii) {
    const float earth_period_s = 365.25f * 86400.0f;
    const float mars_period_s = 686.98f * 86400.0f;

    float synodic = UpeOrbital_SynodicPeriod(earth_period_s, mars_period_s);
    REQUIRE(synodic > 6.0e7f);
    REQUIRE(synodic < 7.5e7f);

    const float m_sun = 1.9885e30f;
    const float m_earth = 5.9722e24f;
    const float one_au = 1.4959787e11f;

    float soi = UpeOrbital_SphereOfInfluenceRadius(m_sun, m_earth, one_au);
    float hill = UpeOrbital_HillSphereRadius(m_sun, m_earth, one_au);

    REQUIRE(soi > 8.0e8f);
    REQUIRE(soi < 1.1e9f);
    REQUIRE(hill > 1.2e9f);
    REQUIRE(hill < 1.7e9f);
}

TEST(TestOrbitalHelpersRejectInvalidInputs) {
    REQUIRE(UpeOrbital_MeanMotion(0.0f, 1.0f) == 0.0f);
    REQUIRE(UpeOrbital_SynodicPeriod(0.0f, 1.0f) == 0.0f);
    REQUIRE(UpeOrbital_SphereOfInfluenceRadius(1.0f, 0.0f, 1.0f) == 0.0f);
    REQUIRE(UpeOrbital_HillSphereRadius(1.0f, 0.0f, 1.0f) == 0.0f);
}

TEST(TestStringReggeRelations) {
    float alpha_prime = 0.5f;
    float m2 = UpeString_ReggeMassSquared(4u, alpha_prime);
    float m = UpeString_ReggeMass(4u, alpha_prime);

    REQUIRE(fabsf(m2 - 8.0f) < 1e-5f);
    REQUIRE(fabsf(m - sqrtf(8.0f)) < 1e-5f);
}

TEST(TestStringCompactificationAndBraneTension) {
    float mkk = UpeString_CompactificationKKModeMass(2u, 1.0545718e-34f, 1.0e-18f, 3.0e8f);
    REQUIRE(mkk > 6.0e-25f);
    REQUIRE(mkk < 8.0e-25f);

    float t1 = UpeString_DBraneTension(1u, 0.1f, 1.0e-3f);
    REQUIRE(t1 > 1.0e6f);
    REQUIRE(t1 < 2.0e6f);
}

TEST(TestStringHelpersRejectInvalidInputs) {
    REQUIRE(UpeString_ReggeMassSquared(1u, 0.0f) == 0.0f);
    REQUIRE(UpeString_ReggeMass(1u, 0.0f) == 0.0f);
    REQUIRE(UpeString_CompactificationKKModeMass(1u, 1.0f, 0.0f, 1.0f) == 0.0f);
    REQUIRE(UpeString_DBraneTension(2u, 0.0f, 1.0f) == 0.0f);
}
