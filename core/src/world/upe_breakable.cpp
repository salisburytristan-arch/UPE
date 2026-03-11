#include "upe/upe_breakable.h"

void UpeBreakable_Init(UpeBreakable *breakable, uint32_t body_id, float threshold) {
    if (!breakable) {
        return;
    }
    breakable->body_id = body_id;
    breakable->fracture_threshold = threshold;
    breakable->stress = 0.0f;
    breakable->fractured = 0;
}

void UpeBreakable_ApplyStress(UpeBreakable *breakable, float stress) {
    if (!breakable) {
        return;
    }
    breakable->stress += stress;
    if (breakable->stress >= breakable->fracture_threshold) {
        breakable->fractured = 1;
    }
}

int UpeBreakable_CheckFracture(UpeBreakable *breakable) {
    return breakable ? breakable->fractured : 0;
}
