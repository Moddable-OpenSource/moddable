/*---
description: 
flags: [module]
---*/

const smr1 = new StaticModuleRecord({ 
	initialize($, _import, importMeta) {
		assert.sameValue(typeof importMeta, "undefined");
	}
});
assert.sameValue(smr1.needsImportMeta, false);

const smr2 = new StaticModuleRecord({ 
	initialize($, _import, importMeta) {
		assert.sameValue(typeof importMeta, "object");
	},
	needsImportMeta: true,
});
assert.sameValue(smr2.needsImportMeta, true);

const c1 = new Compartment({ 
	modules: {
		smr1: { record:smr1 },
		smr2: { record:smr2 },
	}
});
const ns1 = await c1.import("smr1");
const ns2 = await c1.import("smr2");
