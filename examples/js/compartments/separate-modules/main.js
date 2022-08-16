import increment from "increment";
function test() {
    trace("app " + increment() + "\n");
}
let compartment = new Compartment({ 
	modules: {
		increment: { source:"increment" },
		mod: { source:"mod" },
	},
});
let modNS = compartment.importNow("mod");
test();
modNS.test();
test();
modNS.test();
