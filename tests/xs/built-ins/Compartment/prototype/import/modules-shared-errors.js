/*---
description: 
flags: [async,onlyStrict]
---*/

function resolveHook(importSpecifier, referrerSpecifier) {
	return importSpecifier;
}

const importMeta = { count: { foo:0, bar:0 } };

const foo = { source: new ModuleSource(`
	import.meta.count.foo++;
	export default "foo";
	throw new Error("foo");
`), importMeta};
const bar = { source: new ModuleSource(`
	import foo from "foo";
	export default foo + "bar";
	import.meta.count.bar++;
`), importMeta};
const modules = { foo, bar };

const c1 = new Compartment({ resolveHook, modules });
const c2 = new Compartment({ resolveHook, modules: {
	foo: { namespace:"foo", compartment:c1 },
	bar: { namespace:"bar", compartment:c1 },
}});

Promise.allSettled([
	c1.import("bar"),
	c2.import("bar"),
	c1.import("foo"),
	c2.import("foo"),
])
.then(([ bar1, bar2, foo1, foo2 ]) => {
	assert.sameValue(foo1.status, "rejected", "foo1 rejected");
	assert.sameValue(foo2.status, "rejected", "foo2 rejected");
	assert.sameValue(bar1.status, "rejected", "bar1 rejected");
	assert.sameValue(bar2.status, "rejected", "bar2 rejected");
	assert(foo1.reason == foo2.reason, "shared errors");
	assert(bar1.reason == bar2.reason, "shared errors");
	assert.sameValue(importMeta.count.foo, 1, "foo initialized once");
	assert.sameValue(importMeta.count.bar, 0, "bar uninitialized");
})
.then($DONE, $DONE);
