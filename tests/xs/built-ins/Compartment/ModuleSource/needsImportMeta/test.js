/*---
description: 
flags: [onlyStrict]
---*/

const smr1 = new ModuleSource(`
	import * as foo from "foo";
`);
assert.sameValue(smr1.needsImportMeta, false, "needsImportMeta");

const smr2 = new ModuleSource(`
	const foo = import.meta.uri;
`);
assert.sameValue(smr2.needsImportMeta, true, "needsImportMeta");

const smr3 = new ModuleSource(`
	function foo() {
		return import.meta.uri;
	}
`);
assert.sameValue(smr3.needsImportMeta, true, "needsImportMeta");

