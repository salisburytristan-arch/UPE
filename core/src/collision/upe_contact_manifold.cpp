#include "upe/upe_contact.h"

void UpeContactManifold_Clear(UpeContactManifold *manifold) {
    if (!manifold) {
        return;
    }
    manifold->count = 0;
}
