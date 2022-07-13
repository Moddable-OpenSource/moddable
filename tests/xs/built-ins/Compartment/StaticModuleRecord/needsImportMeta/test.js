/*---
description: 
flags: [onlyStrict]
---*/

const smr1 = new StaticModuleRecord(`
	import * as foo from "foo";
`);
assert.sameValue(smr1.needsImportMeta, false, "needsImportMeta");

const smr2 = new StaticModuleRecord(`
	const foo = import.meta.uri;
`);
assert.sameValue(smr2.needsImportMeta, true, "needsImportMeta");

const smr3 = new StaticModuleRecord(`
	function foo() {
		return import.meta.uri;
	}
`);
assert.sameValue(smr3.needsImportMeta, true, "needsImportMeta");

