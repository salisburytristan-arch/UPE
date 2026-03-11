#include "upe/upe_units.h"

UpeUnits UpeUnits_SI() {
    UpeUnits units;
    units.length_scale = 1.0f;
    units.mass_scale = 1.0f;
    units.time_scale = 1.0f;
    return units;
}

UpeUnits UpeUnits_FromScales(float length_scale, float mass_scale, float time_scale) {
    UpeUnits units;
    units.length_scale = length_scale;
    units.mass_scale = mass_scale;
    units.time_scale = time_scale;
    return units;
}
