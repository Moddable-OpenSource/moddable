/*---
description: 
flags: [onlyStrict]
includes: [deepEqual.js]
---*/

const bindings = [
	{
		export: "v0",
		as: "w0"
	},
	{
		export: "v0"
	},
	{
		export: "default"
	},
	{
		import: "default",
		as: "v1",
		from: "mod"
	},
	{
		importAllFrom: "mod",
		as: "ns1"
	},
	{
		import: "x1",
		from: "mod"
	},
	{
		import: "v1",
		as: "w1",
		from: "mod"
	},
	{
		export: "x2",
		from: "mod"
	},
	{
		export: "v2",
		as: "w2",
		from: "mod"
	},
	{
		exportAllFrom: "mod"
	},
	{
		exportAllFrom: "mod",
		as: "ns2"
	}
];

const smr = new StaticModuleRecord({
	bindings,
	initialize($, _import, importMeta) {
	}
});

assert.deepEqual(smr.bindings, bindings);
