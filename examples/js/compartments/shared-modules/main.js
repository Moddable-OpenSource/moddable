import * as increment from "increment";
let compartment1 = new Compartment({ 
	modules: {
		increment: { namespace: increment },
		mod: { source: "mod" },
	}
});
let compartment2 = new Compartment({ 
	modules: {
		increment: { namespace: increment },
		mod: { source: "mod" },
	}
});

let modNS1 = compartment1.importNow("mod")
let modNS2 = compartment2.importNow("mod")

modNS1.test();
modNS2.test();
modNS1.test();
modNS2.test();
