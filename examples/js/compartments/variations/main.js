let compartment1 = new Compartment({ name:"compartment1" }, { "mod":"mod", "vary":"decrement" });
let compartment2 = new Compartment({ name:"compartment2" }, { "mod":"mod", "vary":"increment" });

let modNS1 = compartment1.importNow("mod")
let modNS2 = compartment2.importNow("mod")

modNS1.test();
modNS2.test();
modNS1.test();
modNS2.test();
