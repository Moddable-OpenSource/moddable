import increment from "increment";
function test() {
    trace("app " + increment() + "\n");
}
let mod = new Compartment("mod");
test();
mod.export.test();
test();
mod.export.test();
