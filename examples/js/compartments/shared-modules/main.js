import * as increment from "increment";
function test() {
    trace("app " + increment.default() + "\n");
}
const modules = {
	mod: new StaticModuleRecord({ archive:"mod" }),
};
let compartment = new Compartment({}, { increment }, {
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
