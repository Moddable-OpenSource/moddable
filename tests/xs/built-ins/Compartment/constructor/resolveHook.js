/*---
description: 
flags: [module]
---*/

const c = new Compartment({
	modules: {
		a: { record: new StaticModuleRecord(`
			import b from "b";
			export default "a" + b;
		`)},
		b_a: { record: new StaticModuleRecord(`
			import c from "c";
			export default "b" + c;
		`)},
		c_b_a: { record: new StaticModuleRecord(`
			export default "c";
		`)},
	},
	resolveHook(specifier, referrerSpecifier) {
		return specifier + "_" + referrerSpecifier;
	}
})
const nsa = await c.import("a");
assert.sameValue(nsa.default, "abc", "resolveHook");
