let { decrement, increment, mod } = Compartment.map;
let mod1 = new Compartment("mod", { name:"mod1" }, { mod, vary:decrement });
let mod2 = new Compartment("mod", { name:"mod2" }, { mod, vary:increment });
mod1.export.test();
mod2.export.test();
mod1.export.test();
mod2.export.test();
