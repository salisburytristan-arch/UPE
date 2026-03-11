"""
UPE QA Scene: Stack Test
Tests rigid body stacking stability with various shapes
"""

from upe_bindings import UpeBindings, Vec3, Quat

def create_stack_scene():
    upe = UpeBindings()
    world = upe.create_world(fixed_dt=1.0/60.0)
    
    # Create ground plane
    ground = upe.create_body(world, position=(0, -1, 0), mass=0.0)  # Static
    
    # Create box stack
    box_height = 1.0
    num_boxes = 10
    
    for i in range(num_boxes):
        y = box_height * i + 0.5
        body_id = upe.create_body(world, position=(0, y, 0), mass=1.0)
        print(f"Created box {i} at height {y}")
    
    # Simulate
    print("Running stack stability test...")
    for step in range(600):  # 10 seconds
        upe.step_world(world)
        if step % 60 == 0:
            print(f"Time: {upe.get_world_time(world):.2f}s")
    
    print("Stack test complete")
    upe.destroy_world(world)

if __name__ == "__main__":
    create_stack_scene()
