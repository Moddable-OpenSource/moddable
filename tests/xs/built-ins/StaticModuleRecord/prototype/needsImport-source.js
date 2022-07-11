/*---
description: 
flags: [onlyStrict]
---*/

const smr1 = new StaticModuleRecord({ source:`
	import * as foo from "foo";
`});
assert.sameValue(smr1.needsImport, false, "needsImport");

const smr2 = new StaticModuleRecord({ source:`
	const foo = await import("foo");
`});
assert.sameValue(smr2.needsImport, true, "needsImport");

const smr3 = new StaticModuleRecord({ source:`
	async function foo() {
		return await import("foo");
	}
`});
assert.sameValue(smr3.needsImport, true, "needsImport");

