globalThis.x = 0;
globalThis.increment = function() {
    return x++;
}
globalThis.test = function() {
    trace("app " + increment() + "\n");
}
let compartment = new Compartment({ increment });
await compartment.import("mod");
test();
compartment.global.test();
test();
compartment.global.test();
