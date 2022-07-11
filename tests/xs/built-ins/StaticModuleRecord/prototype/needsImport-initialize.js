/*---
description: 
flags: [module]
---*/

const smr1 = new StaticModuleRecord({ 
	initialize($, _import, importMeta) {
		assert.sameValue(typeof _import, "undefined");
	}
});
assert.sameValue(smr1.needsImport, false, "needsImport");

const smr2 = new StaticModuleRecord({ 
	initialize($, _import, importMeta) {
		assert.sameValue(typeof _import, "function");
	},
	needsImport: true,
});
assert.sameValue(smr2.needsImport, true, "needsImport");

const c1 = new Compartment({ 
	modules: {
		smr1: { record:smr1 },
		smr2: { record:smr2 },
	}
});
// const ns1 = await c1.import("smr1");
const ns2 = await c1.import("smr2");
