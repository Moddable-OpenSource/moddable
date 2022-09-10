const c = 0;
let l = 0;
var v = 0;
globalThis.g = 0;

const compartment = new Compartment({
	globals: { e:0 }
});
const globals = compartment.globalThis;

compartment.evaluate(`
	const c = 1;
	let l = 1;
	var v = 1;
	globalThis.g = 1;
`);

trace(c + " " + l + " " + v + " " + g + "\n");
// 0 0 0 0
trace(globals.c + " " + globals.l + " " + globals.v + " " + globals.g + "\n");
// undefined undefined undefined 1

globals.e++;
let result = compartment.evaluate(`
	try {
		x = 0;
	}
	catch {
		trace("strict ");
	}
	++e;
`);
trace(result + "\n");
// strict 2




