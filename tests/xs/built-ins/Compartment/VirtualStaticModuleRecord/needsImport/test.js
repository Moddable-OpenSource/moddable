/*---
description: 
flags: [async,onlyStrict]
---*/

const vsmr1 = { 
	initialize($, _import, importMeta) {
		assert.sameValue(typeof _import, "undefined");
	}
};

const vsmr2 = { 
	initialize($, _import, importMeta) {
		assert.sameValue(typeof _import, "function");
	},
	needsImport: true,
};

const c1 = new Compartment({ 
	modules: {
		vsmr1: { record:vsmr1 },
		vsmr2: { record:vsmr2 },
	}
});

Promise.all([
	c1.import("vsmr1"),
	c1.import("vsmr2"),
])
.then(([ns1, ns2]) => {
})
.then($DONE, $DONE);
