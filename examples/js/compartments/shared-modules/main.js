import increment from "increment";
function test() {
    trace("app " + increment() + "\n");
}
let compartment = new Compartment({}, {
	increment: "/increment.xsb",
	mod: "/mod.xsb",
}, {
	resolveHook(specifier, refererSpecifier) {
		return specifier;
	},
	loadNowHook(specifier) {
		return modules[specifier];
	}
});
let modNS = compartment.importNow("mod");
test();
modNS.test();
test();
modNS.test();
