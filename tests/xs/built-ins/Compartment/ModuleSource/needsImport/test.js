/*---
description: 
flags: [onlyStrict]
---*/

const smr1 = new ModuleSource(`
	import * as foo from "foo";
`);
assert.sameValue(smr1.needsImport, false, "needsImport");

const smr2 = new ModuleSource(`
	const foo = await import("foo");
`);
assert.sameValue(smr2.needsImport, true, "needsImport");

const smr3 = new ModuleSource(`
	async function foo() {
		return await import("foo");
	}
`);
assert.sameValue(smr3.needsImport, true, "needsImport");

