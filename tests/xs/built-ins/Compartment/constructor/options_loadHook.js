/*---
description: 
flags: [module]
---*/

const c = new Compartment({}, {}, {
	async loadHook(specifier) {
		return { source:`export default import.meta.uri`, meta: { uri: specifier } };
	},
})
const nsa = await c.import("a");
assert.sameValue(nsa.default, "a", "a");
const nsb = await c.import("b");
assert.sameValue(nsb.default, "b", "b");
