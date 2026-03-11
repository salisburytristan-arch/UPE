#include "upe/upe_shape.h"

void UpeShape_Init(UpeShape *shape, uint32_t id, const UpeShapeDesc *desc) {
    if (!shape || !desc) {
        return;
    }
    shape->id = id;
    shape->type = desc->type;
    shape->body_id = desc->body_id;
    shape->collision_layer = desc->collision_layer;
    shape->collision_mask = desc->collision_mask;
    shape->friction = desc->friction > 0.0f ? desc->friction : 0.5f;
    shape->restitution = desc->restitution > 0.0f ? desc->restitution : 0.3f;
    shape->next_shape_index = 0xFFFFFFFF;

    switch (desc->type) {
        case UPE_SHAPE_SPHERE:
            shape->sphere = desc->sphere;
            break;
        case UPE_SHAPE_CAPSULE:
            shape->capsule = desc->capsule;
            break;
        case UPE_SHAPE_BOX:
            shape->box = desc->box;
            break;
        case UPE_SHAPE_OBB:
            shape->obb = desc->obb;
            UpeOBB_UpdateAxes(&shape->obb);
            break;
        default:
            break;
    }
}
