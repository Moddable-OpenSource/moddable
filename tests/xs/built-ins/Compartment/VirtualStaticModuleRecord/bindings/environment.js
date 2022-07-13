/*---
description: 
flags: [async,onlyStrict]
---*/

const smr = new StaticModuleRecord(`
	export const x = 1;
	export const z = 3;
`);

const vsmr = {
	bindings:[
		{ import: "x", from: "smr" },
		{ export: "y" },
		{ export: "z", from: "smr" },
	], 
	async initialize($, Import, ImportMeta) {
		assert.throws(TypeError, () => {
			$.z = 0;
		});
		assert.throws(TypeError, () => {
			delete $.y;
		});
		assert.throws(TypeError, () => {
			$.x = 0;
		});
		$.y = 2;
	} 
};

const c1 = new Compartment({ 
	modules: {
		smr: { record:smr },
		vsmr: { record:vsmr },
	}
});

c1.import("vsmr")
.then((ns) => {
	assert.sameValue(ns.x, undefined);
	assert.sameValue(ns.y, 2);
	assert.sameValue(ns.z, 3);
})
.then($DONE, $DONE);
