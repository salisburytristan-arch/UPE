#include "upe/upe_contact_scalar.h"
#include <string.h>

void UpeScalarContactManifold_Clear(UpeScalarContactManifold *manifold) {
    if (!manifold) {
        return;
    }
    memset(manifold, 0, sizeof(*manifold));
}
