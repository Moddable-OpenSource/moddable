import * as incrementNS from "increment";
function test() {
    trace("app " + incrementNS.default() + "\n");
}
let compartment = new Compartment({}, { 
	"mod":"mod", 
	"increment":incrementNS 
});
let modNS = compartment.importNow("mod");
test();
modNS.test();
test();
modNS.test();
