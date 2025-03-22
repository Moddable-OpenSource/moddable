/*---
description: 
flags: [async,onlyStrict]
---*/

function resolveHook(importSpecifier, referrerSpecifier) {
	return importSpecifier;
}
function importHook(specifier) {
	return  { source: new ModuleSource(`
		export default "foo"
	`)};
}
const c = new Compartment({ resolveHook, importHook });

c.import("foo")
.then(foo => {
	assert.sameValue(foo.default, "foo", "foo.default");
})
.then($DONE, $DONE);
