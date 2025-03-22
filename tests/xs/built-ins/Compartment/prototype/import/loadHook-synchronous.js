/*---
description: 
flags: [async,onlyStrict]
---*/

function resolveHook(importSpecifier, referrerSpecifier) {
	return importSpecifier;
}
function loadHook(specifier) {
	return  { source: new ModuleSource(`
		export default "foo"
	`)};
}
const c = new Compartment({ resolveHook, loadHook });

c.import("foo")
.then(foo => {
	assert.sameValue(foo.default, "foo", "foo.default");
})
.then($DONE, $DONE);
