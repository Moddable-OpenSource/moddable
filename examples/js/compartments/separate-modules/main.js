import increment from "increment";
function test() {
    trace("app " + increment() + "\n");
}
const modules = {
	increment: new StaticModuleRecord({ archive:"increment" }),
	mod: new StaticModuleRecord({ archive:"mod" }),
};
let compartment = new Compartment({}, {}, {
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
