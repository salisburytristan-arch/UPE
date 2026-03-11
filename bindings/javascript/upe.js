// JavaScript/WebAssembly bindings for UPE
export class UpeWorld {
    constructor(fixedDt = 1.0 / 60.0) {
        this.world = Module._UpeWorld_Create(fixedDt);
    }

    step() {
        Module._UpeWorld_Step(this.world);
    }

    getTime() {
        return Module._UpeWorld_Time(this.world);
    }

    createBody(x = 0, y = 0, z = 0, mass = 1.0) {
        return Module._UpeWorld_CreateBody(this.world, x, y, z, mass);
    }

    destroy() {
        if (this.world) {
            Module._UpeWorld_Destroy(this.world);
            this.world = null;
        }
    }
}

// Example usage
export async function runSimulation() {
    const world = new UpeWorld();
    
    for (let i = 0; i < 100; i++) {
        world.step();
    }
    
    console.log(`World time: ${world.getTime()}`);
    world.destroy();
}
