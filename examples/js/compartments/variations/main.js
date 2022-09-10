function resolveHook(specifier) { return specifier }

let compartment1 = new Compartment({
	globals: { 
		name:"compartment1"
	}, 
	modules: {
		mod:{ source:"mod" },
		vary:{ source:"decrement" },
	},
	resolveHook
});
let compartment2 = new Compartment({
	globals: {
		name:"compartment2"
	}, 
	modules: {
		mod:{ source:"mod" },
		vary:{ source:"increment" },
	},
	resolveHook
});
let modNS1 = compartment1.importNow("mod")
let modNS2 = compartment2.importNow("mod")

modNS1.test();
modNS2.test();
modNS1.test();
modNS2.test();
