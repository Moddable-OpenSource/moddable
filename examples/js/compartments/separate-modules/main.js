import increment from "increment";
function test() {
    trace("app " + increment() + "\n");
}
let compartment = new Compartment({}, { 
	"*": { 
		"mod":"mod", 
		"increment":"increment" } 
});
let modNS = await compartment.import("mod");
test();
modNS.test();
test();
modNS.test();
