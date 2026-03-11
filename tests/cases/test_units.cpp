#include "test_framework.h"
#include "upe/upe_units.h"

TEST(TestUnitsScales) {
    UpeUnits si = UpeUnits_SI();
    REQUIRE(si.length_scale == 1.0f);
    REQUIRE(si.mass_scale == 1.0f);
    REQUIRE(si.time_scale == 1.0f);

    UpeUnits custom = UpeUnits_FromScales(0.01f, 2.0f, 0.5f);
    REQUIRE(custom.length_scale == 0.01f);
    REQUIRE(custom.mass_scale == 2.0f);
    REQUIRE(custom.time_scale == 0.5f);
}
