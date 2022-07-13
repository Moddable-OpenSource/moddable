/*---
description: 
flags: [async,onlyStrict]
---*/

const smr = new StaticModuleRecord(`
	export default 0;
	export let v1 = 1;
	export let v2 = 2;
	export const x1 = 1;
	export const x2 = 2;
`);

const vsmr = {
	bindings: [
		{
			export: "default"
		},
		{
			export: "v0"
		},
		{
			export: "v0",
			as: "w0"
		},
		{
			import: "v1",
			as: "w1",
			from: "smr"
		},
		{
			export: "v2",
			as: "w2",
			from: "smr"
		},
		{
			import: "x1",
			from: "smr"
		},
		{
			export: "x2",
			from: "smr"
		},
		{
			importAllFrom: "smr",
			as: "ns1"
		},
		{
			exportAllFrom: "smr",
			as: "ns2"
		}
	],
	initialize($, _import, importMeta) {
		$.default = -1;
		$.v0 = 0;
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
	assert.sameValue(ns.default, -1);
	assert.sameValue(ns.v0, 0);
	assert.sameValue(ns.w0, 0);
	assert.sameValue(ns.w1, undefined);
	assert.sameValue(ns.w2, 2);
	assert.sameValue(ns.x1, undefined);
	assert.sameValue(ns.x2, 2);
	assert.sameValue(ns.ns1, undefined);
	assert.sameValue(ns.ns2.default, 0);
	assert.sameValue(ns.ns2.v1, 1);
	assert.sameValue(ns.ns2.v2, 2);
	assert.sameValue(ns.ns2.x1, 1);
	assert.sameValue(ns.ns2.x2, 2);
})
.then($DONE, $DONE);
