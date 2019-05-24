globalThis.x = 0;
globalThis.increment = function() {
    return x++;
}
globalThis.test = function() {
    trace("app " + increment() + "\n");
}
let mod = new Compartment("mod", { increment });
test();
mod.global.test();
test();
mod.global.test();
