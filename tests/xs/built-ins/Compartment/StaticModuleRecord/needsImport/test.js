/*---
description: 
flags: [onlyStrict]
---*/

const smr1 = new StaticModuleRecord(`
	import * as foo from "foo";
`);
assert.sameValue(smr1.needsImport, false, "needsImport");

const smr2 = new StaticModuleRecord(`
	const foo = await import("foo");
`);
assert.sameValue(smr2.needsImport, true, "needsImport");

const smr3 = new StaticModuleRecord(`
	async function foo() {
		return await import("foo");
	}
`);
assert.sameValue(smr3.needsImport, true, "needsImport");

