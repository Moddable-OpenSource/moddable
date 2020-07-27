let x = 0;
const increment = {
	default() {
		return x++;
	}
};
function test() {
    trace("app " + increment.default() + "\n");
}
let compartment = new Compartment({}, { 
	"mod":"mod", 
	increment
});
let modNS = compartment.importNow("mod");
test();
modNS.test();
test();
modNS.test();
