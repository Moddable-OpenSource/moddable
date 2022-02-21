import * as increment from "increment";
const mod = "mod"
let compartment1 = new Compartment({}, { 
	increment,
	mod
});
let compartment2 = new Compartment({}, { 
	increment,
	mod
});

let modNS1 = compartment1.importNow("mod")
let modNS2 = compartment2.importNow("mod")

modNS1.test();
modNS2.test();
modNS1.test();
modNS2.test();
