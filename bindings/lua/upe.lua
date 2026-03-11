-- Lua FFI bindings for UPE
local ffi = require("ffi")

ffi.cdef[[
    typedef struct { float x, y, z; } Vec3;
    typedef struct { float x, y, z, w; } Quat;
    typedef struct { float fixed_dt; } WorldDesc;
    typedef struct {
        uint32_t type;
        Vec3 position;
        Quat rotation;
        float mass;
    } BodyDesc;

    void* UpeWorld_Create(const WorldDesc* desc);
    void UpeWorld_Destroy(void* world);
    void UpeWorld_Step(void* world);
    float UpeWorld_Time(const void* world);
    uint32_t UpeWorld_CreateBody(void* world, const BodyDesc* desc);
]]

local upe = ffi.load("upe")

local World = {}
World.__index = World

function World.new(fixed_dt)
    fixed_dt = fixed_dt or 1.0 / 60.0
    local desc = ffi.new("WorldDesc", {fixed_dt = fixed_dt})
    local handle = upe.UpeWorld_Create(desc)
    return setmetatable({handle = handle}, World)
end

function World:step()
    upe.UpeWorld_Step(self.handle)
end

function World:get_time()
    return upe.UpeWorld_Time(self.handle)
end

function World:create_body(x, y, z, mass)
    x, y, z, mass = x or 0, y or 0, z or 0, mass or 1.0
    local desc = ffi.new("BodyDesc", {
        type = 2,
        position = {x = x, y = y, z = z},
        rotation = {x = 0, y = 0, z = 0, w = 1},
        mass = mass
    })
    return upe.UpeWorld_CreateBody(self.handle, desc)
end

function World:destroy()
    if self.handle ~= nil then
        upe.UpeWorld_Destroy(self.handle)
        self.handle = nil
    end
end

-- Example usage
local world = World.new()
for i = 1, 100 do
    world:step()
end
print("World time: " .. world:get_time())
world:destroy()

return World
