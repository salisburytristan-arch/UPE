#include "upe/upe_api.h"
#include "upe/upe_ccd.h"
#include "upe/upe_world_body_api.h"
#include <cstdio>

static void PrintAabb(const char *label, const UpeAabb3 &box) {
    std::printf("%s min=(%.2f, %.2f, %.2f) max=(%.2f, %.2f, %.2f)\n",
                label,
                box.min.x, box.min.y, box.min.z,
                box.max.x, box.max.y, box.max.z);
}

int main() {
    std::printf("=== UPE Collision Debug Visualization ===\n\n");

    UpeWorldDesc desc = UpeWorldDesc_Default();
    desc.fixed_dt = 1.0f / 60.0f;
    desc.enable_determinism_mode = 1;
    desc.enable_ccd = 1;

    UpeWorld *world = UpeWorld_Create(&desc);
    if (!world) {
        std::printf("Failed to create world\n");
        return 1;
    }

    UpeBodyDesc ground_desc = {};
    ground_desc.type = UPE_BODY_STATIC;
    ground_desc.position = {0.0f, -1.0f, 0.0f};
    ground_desc.rotation = UpeQuat_Identity();
    ground_desc.mass = 0.0f;
    uint32_t ground_id = UpeWorld_CreateBody(world, &ground_desc);

    UpeShapeDesc ground_shape = {};
    ground_shape.type = UPE_SHAPE_BOX;
    ground_shape.body_id = ground_id;
    ground_shape.box.min = {-3.0f, -0.5f, -3.0f};
    ground_shape.box.max = {3.0f, 0.0f, 3.0f};
    UpeWorld_CreateShape(world, &ground_shape);

    UpeBodyDesc body_desc = {};
    body_desc.type = UPE_BODY_DYNAMIC;
    body_desc.position = {0.0f, 6.0f, 0.0f};
    body_desc.rotation = UpeQuat_Identity();
    body_desc.mass = 1.0f;
    uint32_t body_id = UpeWorld_CreateBody(world, &body_desc);

    UpeShapeDesc body_shape = {};
    body_shape.type = UPE_SHAPE_SPHERE;
    body_shape.body_id = body_id;
    body_shape.sphere.center = {0.0f, 0.0f, 0.0f};
    body_shape.sphere.radius = 0.35f;
    UpeWorld_CreateShape(world, &body_shape);

    std::printf("Scene layout:\n");
    PrintAabb("  Ground", ground_shape.box);
    std::printf("  Projectile sphere radius=%.2f at y=%.2f\n\n", body_shape.sphere.radius, body_desc.position.y);

    UpeSphere probe = {};
    probe.center = body_desc.position;
    probe.radius = body_shape.sphere.radius;
    UpeCcdResult sweep = UpeCcd_SphereBox(probe, {0.0f, -18.0f, 0.0f}, ground_shape.box, {0.0f, 0.0f, 0.0f}, 1.0f);
    int hit = sweep.hit;

    std::printf("Sweep result: %s\n", hit ? "hit" : "miss");
    std::printf("TOI: %.4f\n", sweep.toi);
    std::printf("Hit position: (%.3f, %.3f, %.3f)\n", sweep.position.x, sweep.position.y, sweep.position.z);
    std::printf("Hit normal: (%.3f, %.3f, %.3f)\n", sweep.normal.x, sweep.normal.y, sweep.normal.z);
    std::printf("\nASCII visualization:\n");
    std::printf("  y=6.0   o   projectile\n");
    std::printf("  y=0.0  ===== ground =====\n");
    std::printf("  contact at toi=%.3f\n", sweep.toi);

    UpeWorld_Destroy(world);
    return hit ? 0 : 1;
}