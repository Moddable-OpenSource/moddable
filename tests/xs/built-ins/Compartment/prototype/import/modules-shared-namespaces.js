/*---
description: 
flags: [async,onlyStrict]
---*/

function resolveHook(importSpecifier, referrerSpecifier) {
	return importSpecifier;
}

const importMeta = { count: { foo:0, bar:0 } };

const foo = { source: new ModuleSource(`
	export default "foo";
	import.meta.count.foo++;
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

Promise.all([
	c1.import("bar"),
	c2.import("bar"),
	c1.import("foo"),
	c2.import("foo"),
])
.then(([ bar1, bar2, foo1, foo2 ]) => {
	assert(foo1 == foo2, "shared namepaces");
	assert(bar1 == bar2, "shared namepaces");
	assert.sameValue(importMeta.count.foo, 1, "foo initialized once");
	assert.sameValue(importMeta.count.bar, 1, "bar initialized once");
	assert.sameValue(bar1.default, "foobar", "bar1.default");
	assert.sameValue(bar2.default, "foobar", "bar2.default");
})
.then($DONE, $DONE);
