/*---
description: 
flags: [module]
---*/

const c = new Compartment({}, {
	a: { source:`
		import b from "b";
		export default "a" + b;
	`},
	b_a: { source:`
		import c from "c";
		export default "b" + c;
	`},
	c_b_a: { source:`
		export default "c";
	`},
}, {
	resolveHook(specifier, referrerSpecifier) {
		return specifier + "_" + referrerSpecifier;
	}
})
const nsa = await c.import("a");
assert.sameValue(nsa.default, "abc", "resolveHook");
