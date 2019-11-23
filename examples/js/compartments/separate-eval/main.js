globalThis.x = 0;
globalThis.increment = function() {
	return (1,eval)("x++");
}
globalThis.test = function() {
    trace("app " + increment() + "\n");
}
let mod = new Compartment("mod");
test();
mod.global.test();
test();
mod.global.test();
