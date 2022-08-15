/*---
description: 
flags: [onlyStrict]
---*/

function resolveHook(importSpecifier, referrerSpecifier) {
	return importSpecifier;
}

const importMeta = { count: { foo:0, bar:0 } };

const foo = { source: new ModuleSource(`
	export default "foo";
	import.meta.count.foo++;
`), importMeta };
const bar = { source: new ModuleSource(`
	import foo from "foo";
	export default foo + "bar";
	import.meta.count.bar++;
`), importMeta };
const modules = { foo, bar };

const c1 = new Compartment({ resolveHook, modules });
const c2 = new Compartment({ resolveHook, modules });

const [ bar1, bar2, foo1, foo2 ] = [
	c1.importNow("bar"),
	c2.importNow("bar"),
	c1.importNow("foo"),
	c2.importNow("foo"),
];
assert(foo1 != foo2, "separate namepaces");
assert(bar1 != bar2, "separate namepaces");
assert.sameValue(importMeta.count.foo, 2, "foo initialized twice");
assert.sameValue(importMeta.count.bar, 2, "bar initialized twice");
assert.sameValue(bar1.default, "foobar", "bar1.default");
assert.sameValue(bar2.default, "foobar", "bar2.default");
