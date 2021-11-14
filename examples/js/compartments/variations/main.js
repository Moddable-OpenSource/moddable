const mod = new StaticModuleRecord({ archive:"mod" });
globalThis.name = "oops";
const modules1 = {
	mod,
	vary: new StaticModuleRecord({ archive:"decrement" })
};
let compartment1 = new Compartment({ name:"compartment1" }, {}, {
	resolveHook(specifier, refererSpecifier) {
		return specifier;
	},
	loadNowHook(specifier) {
		return modules1[specifier];
	}
});

const modules2 = {
	mod,
	vary: new StaticModuleRecord({ archive:"increment" })
};
let compartment2 = new Compartment({ name:"compartment2" }, {}, {
	resolveHook(specifier, refererSpecifier) {
		return specifier;
	},
	loadNowHook(specifier) {
		return modules2[specifier];
	}
});

let modNS1 = compartment1.importNow("mod")
let modNS2 = compartment2.importNow("mod")

modNS1.test();
modNS2.test();
modNS1.test();
modNS2.test();
