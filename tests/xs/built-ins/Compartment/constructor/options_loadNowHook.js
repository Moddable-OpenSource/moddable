/*---
description: 
flags: [module]
---*/

const c = new Compartment({}, {}, {
	loadNowHook(specifier) {
		return { source:`export default import.meta.uri`, meta: { uri: specifier } };
	},
})
const nsa = await c.importNow("a");
assert.sameValue(nsa.default, "a", "a");
const nsb = await c.importNow("b");
assert.sameValue(nsb.default, "b", "b");
