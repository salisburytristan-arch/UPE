#include "upe/upe_api.h"
#include "upe/upe_buoyancy.h"
#include "upe/upe_ccd.h"
#include "upe/upe_cloth.h"
#include "upe/upe_replay.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct TimingStats {
    double average_ms = 0.0;
    double peak_ms = 0.0;
};

std::string F(float value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(3) << value;
    return out.str();
}

std::string D(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(3) << value;
    return out.str();
}

std::string Hex(uint64_t value) {
    std::ostringstream out;
    out << std::uppercase << std::hex << value;
    return out.str();
}

void EnsureDir(const fs::path& dir) {
    fs::create_directories(dir);
}

void SaveFile(const fs::path& path, const std::string& text) {
    EnsureDir(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << text;
}

UpeWorldDesc MakeWorldDesc(float gravityY = -9.81f, float dt = 1.0f / 60.0f) {
    UpeWorldDesc desc = {};
    desc.fixed_dt = dt;
    desc.gravity[0] = 0.0f;
    desc.gravity[1] = gravityY;
    desc.gravity[2] = 0.0f;
    desc.solver_algorithm = UPE_SOLVER_PGS;
    desc.solver_iterations = 24;
    desc.solver_friction_iterations = 6;
    desc.slider_bias = 0.08f;
    desc.restitution_threshold = 0.5f;
    desc.broadphase_algorithm = UPE_BROADPHASE_SAP;
    desc.broadphase_grid_cells = 32;
    desc.max_contacts = 8192;
    desc.max_pairs = 16384;
    desc.ccd_threshold = 4.0f;
    desc.ccd_enabled = 1;
    desc.enable_ccd = 1;
    desc.enable_determinism_mode = 1;
    desc.enable_sleeping = 1;
    desc.enable_warm_starting = 1;
    desc.enable_manifold_caching = 1;
    desc.contact_persistence_frames = 4;
    return desc;
}

uint32_t AddBox(UpeWorld* world, UpeBodyType type, UpeVec3 position, UpeVec3 halfExtents, float mass, float friction, const UpeQuat& rotation = UpeQuat_Identity()) {
    UpeBodyDesc body = {};
    body.type = type;
    body.position = position;
    body.rotation = rotation;
    body.mass = mass;
    body.collision_layer = 1u;
    body.collision_mask = 0xFFFFFFFFu;
    const uint32_t bodyId = UpeWorld_CreateBody(world, &body);

    UpeShapeDesc shape = {};
    shape.type = UPE_SHAPE_BOX;
    shape.body_id = bodyId;
    shape.friction = friction;
    shape.restitution = 0.05f;
    shape.collision_layer = 1u;
    shape.collision_mask = 0xFFFFFFFFu;
    shape.box.min = {-halfExtents.x, -halfExtents.y, -halfExtents.z};
    shape.box.max = {halfExtents.x, halfExtents.y, halfExtents.z};
    UpeWorld_CreateShape(world, &shape);
    return bodyId;
}

uint32_t AddSphere(UpeWorld* world, UpeBodyType type, UpeVec3 position, float radius, float mass, float friction) {
    UpeBodyDesc body = {};
    body.type = type;
    body.position = position;
    body.rotation = UpeQuat_Identity();
    body.mass = mass;
    body.collision_layer = 1u;
    body.collision_mask = 0xFFFFFFFFu;
    const uint32_t bodyId = UpeWorld_CreateBody(world, &body);

    UpeShapeDesc shape = {};
    shape.type = UPE_SHAPE_SPHERE;
    shape.body_id = bodyId;
    shape.friction = friction;
    shape.restitution = 0.08f;
    shape.collision_layer = 1u;
    shape.collision_mask = 0xFFFFFFFFu;
    shape.sphere.center = {0.0f, 0.0f, 0.0f};
    shape.sphere.radius = radius;
    UpeWorld_CreateShape(world, &shape);
    return bodyId;
}

uint32_t AddCapsule(UpeWorld* world, UpeBodyType type, UpeVec3 position, float halfHeight, float radius, float mass, float friction) {
    UpeBodyDesc body = {};
    body.type = type;
    body.position = position;
    body.rotation = UpeQuat_Identity();
    body.mass = mass;
    body.collision_layer = 1u;
    body.collision_mask = 0xFFFFFFFFu;
    const uint32_t bodyId = UpeWorld_CreateBody(world, &body);

    UpeShapeDesc shape = {};
    shape.type = UPE_SHAPE_CAPSULE;
    shape.body_id = bodyId;
    shape.friction = friction;
    shape.restitution = 0.04f;
    shape.collision_layer = 1u;
    shape.collision_mask = 0xFFFFFFFFu;
    shape.capsule.a = {0.0f, -halfHeight, 0.0f};
    shape.capsule.b = {0.0f, halfHeight, 0.0f};
    shape.capsule.radius = radius;
    UpeWorld_CreateShape(world, &shape);
    return bodyId;
}

std::string BoxObjectJson(UpeBody* body, UpeVec3 halfExtents, const char* color, const char* label) {
    std::ostringstream out;
    out << "{\"shape\":\"box\",\"label\":\"" << label << "\",\"color\":\"" << color
        << "\",\"pos\":[" << F(body->position.x) << "," << F(body->position.y) << "," << F(body->position.z)
        << "],\"size\":[" << F(halfExtents.x) << "," << F(halfExtents.y) << "," << F(halfExtents.z)
        << "],\"rot\":[" << F(body->rotation.x) << "," << F(body->rotation.y) << "," << F(body->rotation.z) << "," << F(body->rotation.w) << "]}";
    return out.str();
}

std::string SphereObjectJson(UpeBody* body, float radius, const char* color, const char* label) {
    std::ostringstream out;
    out << "{\"shape\":\"sphere\",\"label\":\"" << label << "\",\"color\":\"" << color
        << "\",\"pos\":[" << F(body->position.x) << "," << F(body->position.y) << "," << F(body->position.z)
        << "],\"radius\":" << F(radius)
        << ",\"rot\":[" << F(body->rotation.x) << "," << F(body->rotation.y) << "," << F(body->rotation.z) << "," << F(body->rotation.w) << "]}";
    return out.str();
}

std::string CapsuleObjectJson(UpeBody* body, float halfHeight, float radius, const char* color, const char* label) {
    std::ostringstream out;
    out << "{\"shape\":\"capsule\",\"label\":\"" << label << "\",\"color\":\"" << color
        << "\",\"pos\":[" << F(body->position.x) << "," << F(body->position.y) << "," << F(body->position.z)
        << "],\"halfHeight\":" << F(halfHeight) << ",\"radius\":" << F(radius)
        << ",\"rot\":[" << F(body->rotation.x) << "," << F(body->rotation.y) << "," << F(body->rotation.z) << "," << F(body->rotation.w) << "]}";
    return out.str();
}

TimingStats MeasureWorldStep(UpeWorld* world, int steps) {
    TimingStats stats = {};
    double total = 0.0;
    for (int i = 0; i < steps; ++i) {
        const auto begin = std::chrono::high_resolution_clock::now();
        UpeWorld_Step(world);
        const auto end = std::chrono::high_resolution_clock::now();
        const double ms = std::chrono::duration<double, std::milli>(end - begin).count();
        total += ms;
        if (ms > stats.peak_ms) stats.peak_ms = ms;
    }
    stats.average_ms = steps > 0 ? total / static_cast<double>(steps) : 0.0;
    return stats;
}

void ExportBoxStack(const fs::path& outDir) {
    UpeWorldDesc desc = MakeWorldDesc(-9.81f);
    UpeWorld* world = UpeWorld_Create(&desc);
    AddBox(world, UPE_BODY_STATIC, {0.0f, -0.55f, 0.0f}, {7.5f, 0.55f, 7.5f}, 0.0f, 0.95f);

    std::vector<uint32_t> ids;
    for (int level = 0; level < 4; ++level) {
        for (int col = 0; col < 3 - (level / 2); ++col) {
            const float x = (col - 1.0f) * 1.08f + (level % 2 == 0 ? 0.0f : 0.52f);
            const float y = 0.55f + level * 1.05f;
            ids.push_back(AddBox(world, UPE_BODY_DYNAMIC, {x, y, 0.0f}, {0.5f, 0.5f, 0.5f}, 1.0f, 0.72f));
        }
    }

    std::ostringstream json;
    json << "{\"kind\":\"rigid\",\"title\":\"Box Stack Stability\",\"source\":\"UPE engine export\",\"fps\":60,\"frames\":[";

    for (int frame = 0; frame < 96; ++frame) {
        UpeWorld_Step(world);
        if (frame) json << ",";
        json << "{\"t\":" << F(frame / 60.0f) << ",\"hash\":\"" << Hex(UpeWorld_GetLastStateHash(world)) << "\",\"objects\":[";
        json << "{\"shape\":\"plane\",\"label\":\"ground\",\"color\":\"#0b2d3e\",\"pos\":[0.000,-0.550,0.000],\"size\":[7.500,0.550,7.500]}";
        for (size_t i = 0; i < ids.size(); ++i) {
            if (UpeBody* body = UpeWorld_GetBody(world, ids[i])) {
                json << "," << BoxObjectJson(body, {0.5f, 0.5f, 0.5f}, i % 2 == 0 ? "#18D6C8" : "#7F8CFF", "crate");
            }
        }
        json << "]}";
    }

    json << "]}";
    SaveFile(outDir / "box-stack-stability.json", json.str());
    UpeWorld_Destroy(world);
}

void ExportMixedShapes(const fs::path& outDir) {
    UpeWorldDesc desc = MakeWorldDesc(-9.81f);
    UpeWorld* world = UpeWorld_Create(&desc);
    AddBox(world, UPE_BODY_STATIC, {0.0f, -0.55f, 0.0f}, {8.0f, 0.55f, 8.0f}, 0.0f, 0.8f);

    const uint32_t box = AddBox(world, UPE_BODY_DYNAMIC, {-1.2f, 2.0f, 0.0f}, {0.45f, 0.45f, 0.45f}, 1.0f, 0.7f);
    const uint32_t sphere = AddSphere(world, UPE_BODY_DYNAMIC, {0.3f, 2.8f, 0.0f}, 0.42f, 1.0f, 0.45f);
    const uint32_t capsule = AddCapsule(world, UPE_BODY_DYNAMIC, {1.45f, 3.3f, 0.0f}, 0.48f, 0.22f, 1.0f, 0.6f);

    std::ostringstream json;
    json << "{\"kind\":\"rigid\",\"title\":\"Mixed Shape Resting Contact\",\"source\":\"UPE engine export\",\"fps\":60,\"frames\":[";
    for (int frame = 0; frame < 84; ++frame) {
        UpeWorld_Step(world);
        if (frame) json << ",";
        json << "{\"t\":" << F(frame / 60.0f) << ",\"hash\":\"" << Hex(UpeWorld_GetLastStateHash(world)) << "\",\"objects\":[";
        json << "{\"shape\":\"plane\",\"label\":\"ground\",\"color\":\"#0b2d3e\",\"pos\":[0.000,-0.550,0.000],\"size\":[8.000,0.550,8.000]},";
        json << BoxObjectJson(UpeWorld_GetBody(world, box), {0.45f, 0.45f, 0.45f}, "#18D6C8", "box") << ",";
        json << SphereObjectJson(UpeWorld_GetBody(world, sphere), 0.42f, "#56E39F", "sphere") << ",";
        json << CapsuleObjectJson(UpeWorld_GetBody(world, capsule), 0.48f, 0.22f, "#FFB25B", "capsule");
        json << "]}";
    }
    json << "]}";
    SaveFile(outDir / "mixed-shape-resting-contact.json", json.str());
    UpeWorld_Destroy(world);
}

void ExportFriction(const fs::path& outDir) {
    UpeWorldDesc desc = MakeWorldDesc(-9.81f);
    UpeWorld* world = UpeWorld_Create(&desc);
    AddBox(world, UPE_BODY_STATIC, {0.0f, -0.55f, 0.0f}, {8.0f, 0.55f, 8.0f}, 0.0f, 0.65f);

    const uint32_t lowGrip = AddBox(world, UPE_BODY_DYNAMIC, {-1.4f, 0.6f, -0.4f}, {0.4f, 0.4f, 0.4f}, 1.0f, 0.10f);
    const uint32_t highGrip = AddBox(world, UPE_BODY_DYNAMIC, {-1.4f, 0.6f, 0.9f}, {0.4f, 0.4f, 0.4f}, 1.0f, 0.90f);

    if (UpeBody* body = UpeWorld_GetBody(world, lowGrip)) body->velocity.x = 5.8f;
    if (UpeBody* body = UpeWorld_GetBody(world, highGrip)) body->velocity.x = 3.3f;

    std::ostringstream json;
    json << "{\"kind\":\"rigid\",\"title\":\"Friction Ramp Comparison\",\"source\":\"UPE engine export\",\"fps\":60,\"lanes\":[\"low friction\",\"high friction\"],\"frames\":[";
    for (int frame = 0; frame < 96; ++frame) {
        UpeWorld_Step(world);
        if (frame) json << ",";
        json << "{\"t\":" << F(frame / 60.0f) << ",\"hash\":\"" << Hex(UpeWorld_GetLastStateHash(world)) << "\",\"objects\":[";
        json << "{\"shape\":\"plane\",\"label\":\"track\",\"color\":\"#102938\",\"pos\":[0.000,-0.550,0.000],\"size\":[8.000,0.550,8.000]},";
        json << BoxObjectJson(UpeWorld_GetBody(world, lowGrip), {0.4f, 0.4f, 0.4f}, "#18D6C8", "low-friction") << ",";
        json << BoxObjectJson(UpeWorld_GetBody(world, highGrip), {0.4f, 0.4f, 0.4f}, "#FF8C42", "high-friction");
        json << "]}";
    }
    json << "]}";
    SaveFile(outDir / "friction-ramp-comparison.json", json.str());
    UpeWorld_Destroy(world);
}

void ExportCcd(const fs::path& outDir) {
    std::ostringstream json;
    json << "{\"kind\":\"rigid\",\"title\":\"CCD Tunnel Prevention\",\"source\":\"UPE CCD export\",\"fps\":60,\"frames\":[";

    UpeVec3 startA = {-4.2f, 0.55f, 0.0f};
    UpeVec3 velA = {8.8f, 0.0f, 0.0f};
    UpeVec3 centerB = {1.5f, 0.55f, 0.0f};
    UpeVec3 velB = {0.0f, 0.0f, 0.0f};
    const float dt = 1.0f / 60.0f;
    UpeVec3 current = startA;

    for (int frame = 0; frame < 72; ++frame) {
        UpeSphere projectile = {current, 0.26f};
        UpeSphere blocker = {centerB, 0.5f};
        const UpeCcdResult hit = UpeCcd_SphereSphere(projectile, velA, blocker, velB, dt);
        if (hit.hit && current.x < centerB.x - 0.7f) {
            current = hit.position;
            velA.x = 0.0f;
        } else {
            current.x += velA.x * dt;
        }
        if (frame) json << ",";
        json << "{\"t\":" << F(frame / 60.0f) << ",\"objects\":["
             << "{\"shape\":\"plane\",\"label\":\"ground\",\"color\":\"#0b2d3e\",\"pos\":[0.000,-0.550,0.000],\"size\":[8.000,0.550,4.000]},"
             << "{\"shape\":\"sphere\",\"label\":\"projectile\",\"color\":\"#18D6C8\",\"pos\":[" << F(current.x) << ",0.550,0.000],\"radius\":0.260},"
             << "{\"shape\":\"box\",\"label\":\"barrier\",\"color\":\"#FF6A88\",\"pos\":[1.500,0.550,0.000],\"size\":[0.220,0.720,0.720]}"
             << "]}";
    }

    json << "]}";
    SaveFile(outDir / "ccd-tunnel-prevention.json", json.str());
}

void ExportStress(const fs::path& outDir) {
    UpeWorldDesc desc = MakeWorldDesc(-9.81f);
    UpeWorld* world = UpeWorld_Create(&desc);
    AddBox(world, UPE_BODY_STATIC, {0.0f, -0.55f, 0.0f}, {10.0f, 0.55f, 10.0f}, 0.0f, 0.8f);

    std::vector<uint32_t> ids;
    for (int i = 0; i < 30; ++i) {
        const float x = ((i % 6) - 2.5f) * 1.02f;
        const float y = 0.55f + (i / 6) * 1.01f;
        const float z = ((i % 3) - 1.0f) * 0.65f;
        if (i % 2 == 0) ids.push_back(AddBox(world, UPE_BODY_DYNAMIC, {x, y, z}, {0.45f, 0.45f, 0.45f}, 1.0f, 0.65f));
        else ids.push_back(AddSphere(world, UPE_BODY_DYNAMIC, {x, y, z}, 0.32f, 1.0f, 0.55f));
    }

    std::ostringstream json;
    json << "{\"kind\":\"rigid\",\"title\":\"Dense Contact Stress Scene\",\"source\":\"UPE engine export\",\"fps\":60,\"frames\":[";
    for (int frame = 0; frame < 90; ++frame) {
        UpeWorld_Step(world);
        if (frame) json << ",";
        json << "{\"t\":" << F(frame / 60.0f) << ",\"hash\":\"" << Hex(UpeWorld_GetLastStateHash(world)) << "\",\"objects\":[";
        json << "{\"shape\":\"plane\",\"label\":\"ground\",\"color\":\"#0b2d3e\",\"pos\":[0.000,-0.550,0.000],\"size\":[10.000,0.550,10.000]}";
        for (size_t i = 0; i < ids.size(); ++i) {
            if (UpeBody* body = UpeWorld_GetBody(world, ids[i])) {
                if (i % 2 == 0) json << "," << BoxObjectJson(body, {0.45f, 0.45f, 0.45f}, "#7F8CFF", "stress-box");
                else json << "," << SphereObjectJson(body, 0.32f, "#18D6C8", "stress-sphere");
            }
        }
        json << "]}";
    }
    json << "]}";
    SaveFile(outDir / "dense-contact-stress-scene.json", json.str());
    UpeWorld_Destroy(world);
}

TimingStats ExportVehicle(const fs::path& outDir) {
    UpeWorldDesc worldDesc = MakeWorldDesc(-9.81f);
    UpeWorld* world = UpeWorld_Create(&worldDesc);

    AddBox(world, UPE_BODY_STATIC, {0.0f, -0.6f, 0.0f}, {22.0f, 0.6f, 12.0f}, 0.0f, 0.92f);
    AddBox(world, UPE_BODY_STATIC, {3.0f, -0.1f, 0.0f}, {1.0f, 0.25f, 1.6f}, 0.0f, 0.95f);
    AddBox(world, UPE_BODY_STATIC, {7.4f, 0.15f, 0.0f}, {1.2f, 0.45f, 1.8f}, 0.0f, 0.95f);
    AddBox(world, UPE_BODY_STATIC, {11.2f, -0.05f, 0.0f}, {0.9f, 0.18f, 1.4f}, 0.0f, 0.95f);

    UpeBodyDesc chassisDesc = {};
    chassisDesc.type = UPE_BODY_DYNAMIC;
    chassisDesc.position = {0.0f, 1.4f, 0.0f};
    chassisDesc.rotation = UpeQuat_Identity();
    chassisDesc.mass = 1500.0f;
    chassisDesc.collision_layer = 1u;
    chassisDesc.collision_mask = 0xFFFFFFFFu;
    const uint32_t chassisId = UpeWorld_CreateBody(world, &chassisDesc);

    UpeShapeDesc chassisShape = {};
    chassisShape.type = UPE_SHAPE_BOX;
    chassisShape.body_id = chassisId;
    chassisShape.friction = 0.88f;
    chassisShape.collision_layer = 1u;
    chassisShape.collision_mask = 0xFFFFFFFFu;
    chassisShape.box.min = {-0.9f, -0.35f, -1.65f};
    chassisShape.box.max = {0.9f, 0.45f, 1.65f};
    UpeWorld_CreateShape(world, &chassisShape);

    UpeAdvancedVehicle vehicle = {};
    UpeAdvancedVehicle_Init(&vehicle, chassisId, 4);
    vehicle.transmission.type = UPE_TRANSMISSION_AUTOMATIC;
    vehicle.transmission.engine_power = 180000.0f;
    vehicle.transmission.engine_torque = 380.0f;
    vehicle.traction_control_enabled = 1;
    vehicle.abs_enabled = 1;

    std::ostringstream json;
    json << "{\"kind\":\"vehicle\",\"title\":\"Vehicle Suspension Course\",\"source\":\"UPE advanced vehicle export\",\"fps\":60,\"frames\":[";

    TimingStats stats = {};
    double total = 0.0;

    for (int frame = 0; frame < 140; ++frame) {
        const float t = frame * worldDesc.fixed_dt;
        float throttle = 1.0f;
        float steering = 0.0f;
        float brake = 0.0f;
        float handbrake = 0.0f;
        if (t > 1.8f && t < 3.0f) steering = 0.45f;
        if (t > 3.0f && t < 4.2f) steering = -0.28f;
        if (t > 4.4f) { throttle = 0.55f; steering = 0.12f; }

        UpeAdvancedVehicle_SetInput(&vehicle, throttle, steering, brake, handbrake);
        UpeBody* chassis = UpeWorld_GetBody(world, chassisId);
        const auto begin = std::chrono::high_resolution_clock::now();
        if (chassis) UpeAdvancedVehicle_Update(&vehicle, chassis, worldDesc.fixed_dt);
        UpeWorld_Step(world);
        const auto end = std::chrono::high_resolution_clock::now();
        const double ms = std::chrono::duration<double, std::milli>(end - begin).count();
        total += ms;
        if (ms > stats.peak_ms) stats.peak_ms = ms;

        chassis = UpeWorld_GetBody(world, chassisId);
        if (frame) json << ",";
        json << "{\"t\":" << F(t) << ",\"speed\":" << F(chassis ? UpeVec3_Length(chassis->velocity) * 3.6f : 0.0f)
             << ",\"drift\":" << F(vehicle.drift_angle) << ",\"wheelCompression\":[";
        for (uint32_t wheel = 0; wheel < vehicle.wheel_count; ++wheel) {
            if (wheel) json << ",";
            json << F(vehicle.wheels[wheel].suspension_compression);
        }
        json << "],\"objects\":["
             << "{\"shape\":\"track\",\"label\":\"course\",\"color\":\"#0b2d3e\",\"pos\":[0.000,-0.600,0.000],\"size\":[22.000,0.600,12.000]},"
             << "{\"shape\":\"box\",\"label\":\"bump-a\",\"color\":\"#15384a\",\"pos\":[3.000,-0.100,0.000],\"size\":[1.000,0.250,1.600]},"
             << "{\"shape\":\"box\",\"label\":\"bump-b\",\"color\":\"#15384a\",\"pos\":[7.400,0.150,0.000],\"size\":[1.200,0.450,1.800]},"
             << "{\"shape\":\"box\",\"label\":\"bump-c\",\"color\":\"#15384a\",\"pos\":[11.200,-0.050,0.000],\"size\":[0.900,0.180,1.400]},"
             << BoxObjectJson(chassis, {0.9f, 0.4f, 1.65f}, "#18D6C8", "chassis")
             << "]}";
    }
    json << "]}";
    SaveFile(outDir / "vehicle-suspension-course.json", json.str());
    UpeWorld_Destroy(world);
    stats.average_ms = total / 140.0;
    return stats;
}

TimingStats ExportCloth(const fs::path& outDir) {
    UpeWorldDesc worldDesc = MakeWorldDesc(-2.0f);
    UpeWorld* world = UpeWorld_Create(&worldDesc);

    UpeCloth cloth = {};
    UpeCloth_Init(&cloth, 16, 10, 0.25f);
    for (uint32_t i = 0; i < cloth.vertex_count; ++i) {
        cloth.vertices[i].position.y += 5.0f;
        cloth.vertices[i].position.x -= 2.0f;
    }
    for (uint32_t y = 0; y < 10; ++y) {
        cloth.vertices[y * 16].pinned = 1;
        cloth.vertices[y * 16 + 15].pinned = 1;
    }
    cloth.wind_velocity = {3.0f, 0.0f, 0.0f};
    cloth.drag_coefficient = 0.15f;
    cloth.air_density = 1.2f;

    std::ostringstream json;
    json << "{\"kind\":\"cloth\",\"title\":\"Cloth / Wind Sheet\",\"source\":\"UPE cloth export\",\"fps\":60,\"grid\":[16,10],\"frames\":[";

    TimingStats stats = {};
    double total = 0.0;
    float windPhase = 0.0f;

    for (int frame = 0; frame < 110; ++frame) {
        windPhase += worldDesc.fixed_dt;
        const float gust = std::sinf(windPhase * 0.8f) * 0.5f + 1.0f;
        cloth.wind_velocity.x = 3.0f * gust;
        cloth.wind_velocity.z = 0.55f * std::sinf(windPhase * 0.3f);

        const auto begin = std::chrono::high_resolution_clock::now();
        UpeCloth_Step(&cloth, worldDesc.fixed_dt, {0.0f, -2.0f, 0.0f});
        const auto end = std::chrono::high_resolution_clock::now();
        const double ms = std::chrono::duration<double, std::milli>(end - begin).count();
        total += ms;
        if (ms > stats.peak_ms) stats.peak_ms = ms;

        for (uint32_t y = 0; y < 10; ++y) {
            cloth.vertices[y * 16].position = {-2.0f, 5.0f - y * 0.25f, 0.0f};
            cloth.vertices[y * 16].velocity = {0.0f, 0.0f, 0.0f};
            cloth.vertices[y * 16 + 15].position = {1.75f, 5.0f - y * 0.25f, 0.0f};
            cloth.vertices[y * 16 + 15].velocity = {0.0f, 0.0f, 0.0f};
        }
        UpeWorld_Step(world);

        if (frame) json << ",";
        json << "{\"t\":" << F(frame / 60.0f) << ",\"wind\":[" << F(cloth.wind_velocity.x) << "," << F(cloth.wind_velocity.y) << "," << F(cloth.wind_velocity.z) << "],\"vertices\":[";
        for (uint32_t i = 0; i < cloth.vertex_count; ++i) {
            if (i) json << ",";
            const UpeVec3& p = cloth.vertices[i].position;
            json << "[" << F(p.x) << "," << F(p.y) << "," << F(p.z) << "]";
        }
        json << "]}";
    }

    json << "]}";
    SaveFile(outDir / "cloth-wind-sheet.json", json.str());
    UpeCloth_Destroy(&cloth);
    UpeWorld_Destroy(world);
    stats.average_ms = total / 110.0;
    return stats;
}

void ExportParticles(const fs::path& outDir) {
    UpeParticleSystem particles = {};
    UpeParticleSystem_Init(&particles, 256);

    UpeEmitterPool* pool = UpeEmitterPool_Create(2, 128);
    if (!pool) {
        return;
    }

    UpeEmitterDesc fountain = {};
    fountain.shape = UPE_EMITTER_SPHERE;
    fountain.mode = UPE_EMITTER_CONTINUOUS;
    fountain.position = {0.0f, 0.6f, 0.0f};
    fountain.rotation = UpeQuat_Identity();
    fountain.particles_per_frame = 4;
    fountain.emission_duration = 0.0f;
    fountain.particle_template.velocity = {0.25f, 5.8f, 0.0f};
    fountain.particle_template.lifetime = 2.6f;
    fountain.particle_template.mass = 0.08f;
    fountain.particle_template.size = 0.08f;
    fountain.particle_template.drag = 0.15f;
    fountain.shape_scale = 0.28f;
    fountain.velocity_spread = 1.05f;
    fountain.gravity = {0.0f, -9.81f, 0.0f};
    fountain.wind_velocity = {0.0f, 0.0f, 0.0f};
    fountain.damping = 0.0f;
    const uint32_t fountainId = UpeEmitterPool_AddEmitter(pool, &fountain);

    UpeEmitterDesc scatter = fountain;
    scatter.shape = UPE_EMITTER_CONE;
    scatter.position = {-1.8f, 0.55f, -0.4f};
    scatter.particles_per_frame = 3;
    scatter.particle_template.velocity = {3.4f, 4.1f, 0.8f};
    scatter.particle_template.lifetime = 2.1f;
    scatter.particle_template.size = 0.09f;
    scatter.particle_template.drag = 0.10f;
    scatter.shape_scale = 0.55f;
    scatter.velocity_spread = 0.85f;
    const uint32_t scatterId = UpeEmitterPool_AddEmitter(pool, &scatter);

    UpeParticleCollisionSystem collisions = {};
    UpeParticleCollisionSystem_Init(&collisions, 512);
    UpeParticleCollisionParams collisionParams = {};
    collisionParams.ground_level = 0.08f;
    collisionParams.particle_radius = 0.08f;
    collisionParams.default_restitution = 0.35f;
    collisionParams.default_friction = 0.12f;
    collisionParams.resting_velocity_threshold = 0.05f;
    collisionParams.use_ground_plane = 1;
    collisionParams.use_particle_collision = 1;
    collisionParams.repulsion_strength = 0.18f;
    UpeParticleCollisionSystem_SetParams(&collisions, &collisionParams);

    std::ostringstream json;
    json << "{\"kind\":\"rigid\",\"title\":\"Particle Scatter Fountain\",\"source\":\"UPE particle emitter export\",\"fps\":60,\"frames\":[";

    const UpeVec3 gravity = {0.0f, -9.81f, 0.0f};
    const float dt = 1.0f / 60.0f;
    UpeParticleSpawnRequest requests[128];

    for (int frame = 0; frame < 180; ++frame) {
        const float t = frame * dt;
        if (fountainId != static_cast<uint32_t>(-1)) {
            const UpeVec3 wind = {0.35f * std::sinf(t * 1.6f), 0.0f, 0.22f * std::cosf(t * 1.2f)};
            UpeEmitterPool_SetForces(pool, fountainId, gravity, wind);
        }
        if (scatterId != static_cast<uint32_t>(-1)) {
            const UpeVec3 movingPos = {-1.8f + 0.35f * std::cosf(t * 0.9f), 0.55f, -0.4f + 0.25f * std::sinf(t * 0.7f)};
            UpeEmitterPool_SetTransform(pool, scatterId, movingPos, UpeQuat_Identity());
            const UpeVec3 scatterWind = {0.55f * std::sinf(t * 1.3f), 0.0f, 0.30f * std::cosf(t * 0.8f)};
            UpeEmitterPool_SetForces(pool, scatterId, gravity, scatterWind);
        }

        UpeEmitterPool_Update(pool, dt);
        const uint32_t requestCount = UpeEmitterPool_GetEmissionQueue(pool, requests, 128);
        if (requestCount > 0) {
            UpeParticleSystem_EmitFromRequests(&particles, requests, requestCount);
        }

        UpeParticleSystem_Step(&particles, dt, gravity);
        UpeParticleCollisionSystem_Update(&collisions, &particles, dt);

        if (frame) json << ",";
        json << "{\"t\":" << F(t) << ",\"objects\":["
             << "{\"shape\":\"plane\",\"label\":\"ground\",\"color\":\"#0b2d3e\",\"pos\":[0.000,-0.550,0.000],\"size\":[8.000,0.550,8.000]},"
             << "{\"shape\":\"box\",\"label\":\"emitter-a\",\"color\":\"#103247\",\"pos\":[0.000,0.180,0.000],\"size\":[0.240,0.180,0.240]},"
             << "{\"shape\":\"box\",\"label\":\"emitter-b\",\"color\":\"#15384a\",\"pos\":[-1.800,0.150,-0.400],\"size\":[0.200,0.150,0.200]}";

        const uint32_t visibleCount = std::min<uint32_t>(particles.count, 96u);
        for (uint32_t i = 0; i < visibleCount; ++i) {
            const UpeParticle& particle = particles.particles[i];
            const char* color = particle.velocity.y > 0.0f ? "#7BE7FF" : ((i % 2 == 0) ? "#18D6C8" : "#FFB25B");
            json << ",{\"shape\":\"sphere\",\"label\":\"particle\",\"color\":\"" << color
                 << "\",\"pos\":[" << F(particle.position.x) << "," << F(particle.position.y) << "," << F(particle.position.z)
                 << "],\"radius\":" << F(std::max(0.05f, particle.size * 0.5f)) << "}";
        }
        json << "]}";
    }

    json << "]}";
    SaveFile(outDir / "particle-scatter-fountain.json", json.str());
    UpeParticleCollisionSystem_Destroy(&collisions);
    UpeEmitterPool_Destroy(pool);
    UpeParticleSystem_Destroy(&particles);
}

void ExportFluid(const fs::path& outDir) {
    UpeFluidSph fluid = {};
    UpeFluidSph_Init(&fluid, 256);
    UpeFluidSph_SetBounds(&fluid, {-2.2f, 0.0f, -1.8f}, {2.2f, 3.6f, 1.8f});
    UpeFluidSph_SetGravity(&fluid, {0.0f, -9.81f, 0.0f});

    for (int x = 0; x < 8; ++x) {
        for (int y = 0; y < 6; ++y) {
            for (int z = 0; z < 4; ++z) {
                const UpeVec3 pos = {
                    -1.55f + x * 0.18f,
                    0.45f + y * 0.12f,
                    -0.55f + z * 0.18f
                };
                const UpeVec3 vel = {0.08f * (z % 2 == 0 ? 1.0f : -1.0f), -0.15f, 0.04f * (x - 3.5f)};
                UpeFluidSph_AddParticle(&fluid, pos, vel);
            }
        }
    }

    UpeBodyDesc bodyDesc = {};
    bodyDesc.type = UPE_BODY_DYNAMIC;
    bodyDesc.position = {0.55f, 1.55f, 0.0f};
    bodyDesc.rotation = UpeQuat_Identity();
    bodyDesc.mass = 6.0f;
    UpeBody floater = {};
    UpeBody_Init(&floater, 7, &bodyDesc);

    UpeBuoyancyDesc buoyancyDesc = {};
    buoyancyDesc.fluid_density = 1000.0f;
    buoyancyDesc.fluid_surface_normal = {0.0f, 1.0f, 0.0f};
    buoyancyDesc.fluid_surface_height = 1.15f;
    buoyancyDesc.drag_coefficient = 0.85f;
    buoyancyDesc.angular_drag = 0.45f;
    buoyancyDesc.sample_points = 24;

    std::ostringstream json;
    json << "{\"kind\":\"rigid\",\"title\":\"SPH Water / Buoyancy Tank\",\"source\":\"UPE fluid + buoyancy export\",\"fps\":60,\"frames\":[";

    const float dt = 1.0f / 60.0f;
    for (int frame = 0; frame < 180; ++frame) {
        const float t = frame * dt;
        const auto begin = std::chrono::high_resolution_clock::now();
        UpeFluidSph_Step(&fluid, dt);
        UpeBuoyancyState buoyancyState = {};
        UpeBuoyancy_ApplyUniform(&floater, &buoyancyDesc, &buoyancyState);
        UpeBody_ApplyForce(&floater, {0.0f, floater.mass * -9.81f, 0.0f});
        const auto end = std::chrono::high_resolution_clock::now();
        (void)begin;
        (void)end;

        if (frame == 18) {
            floater.velocity.x = 1.2f;
            floater.velocity.z = 0.35f;
        }

        floater.velocity = UpeVec3_Add(floater.velocity, UpeVec3_Scale(floater.force, dt / floater.mass));
        floater.position = UpeVec3_Add(floater.position, UpeVec3_Scale(floater.velocity, dt));

        if (floater.position.y < 0.42f) {
            floater.position.y = 0.42f;
            if (floater.velocity.y < 0.0f) floater.velocity.y *= -0.18f;
        }
        if (floater.position.x < -1.45f || floater.position.x > 1.45f) {
            floater.position.x = std::max(-1.45f, std::min(1.45f, floater.position.x));
            floater.velocity.x *= -0.45f;
        }
        if (floater.position.z < -1.05f || floater.position.z > 1.05f) {
            floater.position.z = std::max(-1.05f, std::min(1.05f, floater.position.z));
            floater.velocity.z *= -0.35f;
        }
        UpeBody_ClearForces(&floater);

        if (frame) json << ",";
        json << "{\"t\":" << F(t) << ",\"objects\":["
             << "{\"shape\":\"plane\",\"label\":\"tank-floor\",\"color\":\"#0b2d3e\",\"pos\":[0.000,-0.550,0.000],\"size\":[6.000,0.550,4.000]},"
             << "{\"shape\":\"box\",\"label\":\"left-wall\",\"color\":\"#103247\",\"pos\":[-1.800,0.650,0.000],\"size\":[0.120,1.200,1.250]},"
             << "{\"shape\":\"box\",\"label\":\"right-wall\",\"color\":\"#103247\",\"pos\":[1.800,0.650,0.000],\"size\":[0.120,1.200,1.250]},"
             << "{\"shape\":\"box\",\"label\":\"floater\",\"color\":\"#FFC857\",\"pos\":[" << F(floater.position.x) << "," << F(floater.position.y) << "," << F(floater.position.z) << "],\"size\":[0.280,0.180,0.280],\"rot\":[0.000,0.000,0.000,1.000]}";

        const uint32_t visibleCount = std::min<uint32_t>(fluid.count, 192u);
        for (uint32_t i = 0; i < visibleCount; ++i) {
            const UpeSphParticle& particle = fluid.particles[i];
            const char* color = particle.position.y > buoyancyDesc.fluid_surface_height ? "#A7F3FF" : "#59C7FF";
            json << ",{\"shape\":\"sphere\",\"label\":\"water\",\"color\":\"" << color
                 << "\",\"pos\":[" << F(particle.position.x) << "," << F(particle.position.y) << "," << F(particle.position.z)
                 << "],\"radius\":0.070}";
        }
        json << "]}";
    }

    json << "]}";
    SaveFile(outDir / "fluid-buoyancy-tank.json", json.str());
    UpeFluidSph_Destroy(&fluid);
}

void ExportOrbital(const fs::path& outDir) {
    UpeWorldDesc desc = MakeWorldDesc(0.0f, 1.0f / 60.0f);
    desc.enable_sleeping = 0;
    UpeWorld* world = UpeWorld_Create(&desc);

    const uint32_t star = AddSphere(world, UPE_BODY_STATIC, {0.0f, 0.25f, 0.0f}, 0.42f, 0.0f, 0.0f);
    const uint32_t orbiterA = AddSphere(world, UPE_BODY_DYNAMIC, {3.2f, 0.25f, 0.0f}, 0.16f, 1.0f, 0.0f);
    const uint32_t orbiterB = AddSphere(world, UPE_BODY_DYNAMIC, {0.0f, 0.25f, 4.3f}, 0.12f, 0.7f, 0.0f);

    if (UpeBody* body = UpeWorld_GetBody(world, orbiterA)) {
        body->velocity = {0.0f, 0.0f, 1.85f};
    }
    if (UpeBody* body = UpeWorld_GetBody(world, orbiterB)) {
        body->velocity = {-1.42f, 0.0f, 0.0f};
    }

    UpeGravityFieldDesc field = UpeGravityFieldDesc_Default();
    field.position = {0.0f, 0.25f, 0.0f};
    field.strength = 12.0f;
    field.min_distance = 0.65f;
    field.max_distance = 8.0f;
    field.falloff_exponent = 2.0f;
    UpeWorld_AddGravityField(world, &field);

    std::ostringstream json;
    json << "{\"kind\":\"rigid\",\"title\":\"Gravity Field / Orbital Mechanics\",\"source\":\"UPE gravity field export\",\"fps\":60,\"frames\":[";

    for (int frame = 0; frame < 220; ++frame) {
        UpeWorld_Step(world);
        if (frame) json << ",";
        json << "{\"t\":" << F(frame / 60.0f) << ",\"hash\":\"" << Hex(UpeWorld_GetLastStateHash(world)) << "\",\"objects\":["
             << "{\"shape\":\"sphere\",\"label\":\"star\",\"color\":\"#FFB25B\",\"pos\":[0.000,0.250,0.000],\"radius\":0.420}";
        if (UpeBody* body = UpeWorld_GetBody(world, star)) {
            (void)body;
        }
        if (UpeBody* body = UpeWorld_GetBody(world, orbiterA)) {
            json << "," << SphereObjectJson(body, 0.16f, "#18D6C8", "orbiter-a");
        }
        if (UpeBody* body = UpeWorld_GetBody(world, orbiterB)) {
            json << "," << SphereObjectJson(body, 0.12f, "#7F8CFF", "orbiter-b");
        }
        json << "]}";
    }

    json << "]}";
    SaveFile(outDir / "orbital-mechanics-gravity-well.json", json.str());
    UpeWorld_Destroy(world);
}

TimingStats ExportReplay(const fs::path& outDir) {
    auto runTrack = []() {
        UpeWorldDesc desc = MakeWorldDesc(-9.81f);
        UpeWorld* world = UpeWorld_Create(&desc);
        AddBox(world, UPE_BODY_STATIC, {0.0f, -0.55f, 0.0f}, {8.0f, 0.55f, 8.0f}, 0.0f, 0.85f);
        const uint32_t mover = AddSphere(world, UPE_BODY_DYNAMIC, {-2.4f, 2.6f, 0.0f}, 0.32f, 1.0f, 0.45f);
        if (UpeBody* body = UpeWorld_GetBody(world, mover)) {
            body->velocity.x = 3.0f;
        }
        std::vector<std::string> frames;
        for (int frame = 0; frame < 80; ++frame) {
            UpeWorld_Step(world);
            if (UpeBody* body = UpeWorld_GetBody(world, mover)) {
                std::ostringstream out;
                out << "{\"t\":" << F(frame / 60.0f) << ",\"hash\":\"" << Hex(UpeWorld_GetLastStateHash(world))
                    << "\",\"pos\":[" << F(body->position.x) << "," << F(body->position.y) << "," << F(body->position.z) << "]}";
                frames.push_back(out.str());
            }
        }
        UpeWorld_Destroy(world);
        return frames;
    };

    const auto begin = std::chrono::high_resolution_clock::now();
    const auto reference = runTrack();
    const auto resim = runTrack();
    const auto end = std::chrono::high_resolution_clock::now();

    std::ostringstream json;
    json << "{\"kind\":\"replay\",\"title\":\"Rollback / Re-sim Demo\",\"source\":\"UPE determinism export\",\"fps\":60,\"rollbackFrame\":36,\"reference\":[";
    for (size_t i = 0; i < reference.size(); ++i) {
        if (i) json << ",";
        json << reference[i];
    }
    json << "],\"resim\":[";
    for (size_t i = 0; i < resim.size(); ++i) {
        if (i) json << ",";
        json << resim[i];
    }
    json << "]}";
    SaveFile(outDir / "rollback-resim-demo.json", json.str());

    TimingStats stats = {};
    stats.average_ms = std::chrono::duration<double, std::milli>(end - begin).count() / 160.0;
    stats.peak_ms = stats.average_ms;
    return stats;
}

void ExportBenchmark(const fs::path& outDir, const TimingStats& stack, const TimingStats& vehicle, const TimingStats& cloth, const TimingStats& replay) {
    std::vector<double> spark = {
        stack.average_ms, stack.peak_ms, vehicle.average_ms, vehicle.peak_ms,
        cloth.average_ms, cloth.peak_ms, replay.average_ms, replay.peak_ms,
    };

    std::ostringstream json;
    json << "{\"kind\":\"benchmark\",\"title\":\"Canonical Benchmark Sweep\",\"source\":\"UPE timing export\",\"bars\":["
         << "{\"label\":\"Stack\",\"avg\":" << D(stack.average_ms) << ",\"peak\":" << D(stack.peak_ms) << ",\"accent\":\"#18D6C8\"},"
         << "{\"label\":\"Vehicle\",\"avg\":" << D(vehicle.average_ms) << ",\"peak\":" << D(vehicle.peak_ms) << ",\"accent\":\"#59C7FF\"},"
         << "{\"label\":\"Cloth\",\"avg\":" << D(cloth.average_ms) << ",\"peak\":" << D(cloth.peak_ms) << ",\"accent\":\"#7BE7FF\"},"
         << "{\"label\":\"Replay\",\"avg\":" << D(replay.average_ms) << ",\"peak\":" << D(replay.peak_ms) << ",\"accent\":\"#FFB25B\"}],\"spark\":[";
    for (size_t i = 0; i < spark.size(); ++i) {
        if (i) json << ",";
        json << D(spark[i]);
    }
    json << "]}";
    SaveFile(outDir / "canonical-benchmark-sweep.json", json.str());
}

} // namespace

int main(int argc, char** argv) {
    fs::path outDir = argc > 1 ? fs::path(argv[1]) : fs::current_path() / "site_visual_export";
    EnsureDir(outDir);

    std::cout << "[UPE] Exporting site visuals to: " << outDir.string() << "\n";

    ExportBoxStack(outDir);
    ExportMixedShapes(outDir);
    ExportFriction(outDir);
    ExportCcd(outDir);
    ExportStress(outDir);
    ExportParticles(outDir);
    const TimingStats vehicle = ExportVehicle(outDir);
    const TimingStats cloth = ExportCloth(outDir);
    ExportFluid(outDir);
    ExportOrbital(outDir);
    const TimingStats replay = ExportReplay(outDir);

    UpeWorldDesc stackDesc = MakeWorldDesc(-9.81f);
    UpeWorld* stackWorld = UpeWorld_Create(&stackDesc);
    AddBox(stackWorld, UPE_BODY_STATIC, {0.0f, -0.55f, 0.0f}, {7.5f, 0.55f, 7.5f}, 0.0f, 0.95f);
    for (int i = 0; i < 12; ++i) {
        AddBox(stackWorld, UPE_BODY_DYNAMIC, {(i % 3 - 1) * 1.05f, 0.55f + (i / 3) * 1.0f, 0.0f}, {0.5f, 0.5f, 0.5f}, 1.0f, 0.72f);
    }
    const TimingStats stack = MeasureWorldStep(stackWorld, 90);
    UpeWorld_Destroy(stackWorld);

    ExportBenchmark(outDir, stack, vehicle, cloth, replay);

    std::cout << "[UPE] Visual scene export complete.\n";
    return 0;
}
