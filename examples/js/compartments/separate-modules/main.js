import increment from "increment";
function test() {
    trace("app " + increment() + "\n");
}
let compartment = new Compartment({}, {
	increment: "increment",
	mod: "mod",
});
let modNS = compartment.importNow("mod");
test();
modNS.test();
test();
modNS.test();
