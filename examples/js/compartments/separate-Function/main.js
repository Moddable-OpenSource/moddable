globalThis.x = 0;
globalThis.increment = new Function("return x++");
globalThis.test = function() {
    trace("app " + increment() + "\n");
}
let compartment = new Compartment();
await compartment.import("mod");
test();
compartment.globalThis.test();
test();
compartment.globalThis.test();
