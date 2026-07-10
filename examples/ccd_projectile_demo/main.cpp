#include "upe/upe_api.h"
#include "upe/upe_ccd.h"
#include <cstdio>

int main() {
    std::printf("=== UPE CCD Projectile Demo ===\n\n");

    UpeAabb3 wall = {};
    wall.min = {-0.5f, -1.0f, -1.0f};
    wall.max = {0.5f, 1.0f, 1.0f};

    UpeSphere projectile = {};
    projectile.center = {-10.0f, 0.0f, 0.0f};
    projectile.radius = 0.25f;

    UpeCcdResult result = UpeCcd_SphereBox(projectile, {30.0f, 0.0f, 0.0f}, wall, {0.0f, 0.0f, 0.0f}, 1.0f);
    std::printf("Projectile start: (%.2f, %.2f, %.2f)\n", projectile.center.x, projectile.center.y, projectile.center.z);
    std::printf("Wall bounds: min=(%.2f, %.2f, %.2f) max=(%.2f, %.2f, %.2f)\n",
                wall.min.x, wall.min.y, wall.min.z, wall.max.x, wall.max.y, wall.max.z);
    std::printf("CCD hit: %s\n", result.hit ? "yes" : "no");
    std::printf("TOI: %.4f\n", result.toi);
    std::printf("Contact position: (%.3f, %.3f, %.3f)\n", result.position.x, result.position.y, result.position.z);
    std::printf("Contact normal: (%.3f, %.3f, %.3f)\n", result.normal.x, result.normal.y, result.normal.z);

    if (result.hit) {
        std::printf("ASCII sweep: [----------|X|----wall----]\n");
        return 0;
    }

    std::printf("ASCII sweep: [-----------> no hit ]\n");
    return 1;
}