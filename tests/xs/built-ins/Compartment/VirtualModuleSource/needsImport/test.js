/*---
description: 
flags: [async,onlyStrict]
---*/

const foo = { 
	execute($, Import, ImportMeta) {
		assert.sameValue(typeof Import, "undefined");
	}
};

const bar = { 
	execute($, Import, ImportMeta) {
		assert.sameValue(typeof Import, "function");
	},
	needsImport: true,
};

const c1 = new Compartment({ 
	modules: {
		foo: { source:foo },
		bar: { source:bar },
	}
});

Promise.all([
	c1.import("foo"),
	c1.import("bar"),
])
.then(([ns1, ns2]) => {
})
.then($DONE, $DONE);
