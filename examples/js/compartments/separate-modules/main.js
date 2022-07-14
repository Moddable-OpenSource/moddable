import increment from "increment";
function test() {
    trace("app " + increment() + "\n");
}
let compartment = new Compartment({ 
	modules: {
		increment: { record:"increment" },
		mod: { record:"mod" },
	},
});
debugger
let modNS = compartment.importNow("mod");
test();
modNS.test();
test();
modNS.test();
