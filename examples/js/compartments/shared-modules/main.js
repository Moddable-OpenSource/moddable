import increment from "increment";
function test() {
    trace("app " + increment() + "\n");
}
let compartment = new Compartment();
let modNS = await compartment.import("mod");
test();
modNS.test();
test();
modNS.test();
