/*---
description: 
flags: [module]
---*/

const c = new Compartment({}, {}, {
	moduleMapHook(specifier) {
		throw new Error(specifier);
	}
});

try {
	const fooNS = await c.import("foo");
	throw new Test262Error("no error");
}
catch(e) {
	assert.sameValue(e.toString(), "Error: foo");
}

