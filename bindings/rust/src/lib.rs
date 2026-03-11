// Rust FFI bindings for UPE
use std::os::raw::{c_float, c_uint, c_void};

#[repr(C)]
#[derive(Copy, Clone)]
pub struct Vec3 {
    pub x: c_float,
    pub y: c_float,
    pub z: c_float,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct Quat {
    pub x: c_float,
    pub y: c_float,
    pub z: c_float,
    pub w: c_float,
}

#[repr(C)]
pub struct WorldDesc {
    pub fixed_dt: c_float,
}

#[repr(C)]
pub struct BodyDesc {
    pub body_type: c_uint,
    pub position: Vec3,
    pub rotation: Quat,
    pub mass: c_float,
}

#[link(name = "upe")]
extern "C" {
    fn UpeWorld_Create(desc: *const WorldDesc) -> *mut c_void;
    fn UpeWorld_Destroy(world: *mut c_void);
    fn UpeWorld_Step(world: *mut c_void);
    fn UpeWorld_Time(world: *const c_void) -> c_float;
    fn UpeWorld_CreateBody(world: *mut c_void, desc: *const BodyDesc) -> c_uint;
}

pub struct World {
    handle: *mut c_void,
}

impl World {
    pub fn new(fixed_dt: f32) -> Self {
        let desc = WorldDesc { fixed_dt };
        let handle = unsafe { UpeWorld_Create(&desc) };
        World { handle }
    }

    pub fn step(&mut self) {
        unsafe { UpeWorld_Step(self.handle) };
    }

    pub fn time(&self) -> f32 {
        unsafe { UpeWorld_Time(self.handle) }
    }

    pub fn create_body(&mut self, position: Vec3, mass: f32) -> u32 {
        let desc = BodyDesc {
            body_type: 2,
            position,
            rotation: Quat { x: 0.0, y: 0.0, z: 0.0, w: 1.0 },
            mass,
        };
        unsafe { UpeWorld_CreateBody(self.handle, &desc) }
    }
}

impl Drop for World {
    fn drop(&mut self) {
        unsafe { UpeWorld_Destroy(self.handle) };
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_world_creation() {
        let mut world = World::new(1.0 / 60.0);
        for _ in 0..100 {
            world.step();
        }
        let time = world.time();
        assert!(time > 0.0);
    }
}
