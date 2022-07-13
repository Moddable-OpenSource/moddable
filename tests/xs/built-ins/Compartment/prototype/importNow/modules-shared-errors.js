/*---
description: 
flags: [onlyStrict]
---*/

const importMeta = { count: { foo:0, bar:0 } };

const foo = { record: new StaticModuleRecord(`
	import.meta.count.foo++;
	export default "foo";
	throw new Error("foo");
`), importMeta};
const bar = { record: new StaticModuleRecord(`
	import foo from "foo";
	export default foo + "bar";
	import.meta.count.bar++;
`), importMeta};
const modules = { foo, bar };

const c1 = new Compartment({ modules });
const c2 = new Compartment({ modules: {
	foo: { specifier:"foo", compartment:c1 },
	bar: { specifier:"bar", compartment:c1 },
}});

function test(compartment, specifier) {
	let result;
	try {
		result = { status:"fulfilled", value:compartment.importNow(specifier) };
	}
	catch(reason) {
		result = { status:"rejected", reason };
	}
	return result;
}
const [ bar1, bar2, foo1, foo2 ] = [
	test(c1, "bar"),
	test(c2, "bar"),
	test(c1, "foo"),
	test(c2, "foo"),
];
assert.sameValue(foo1.status, "rejected", "foo1 rejected");
assert.sameValue(foo2.status, "rejected", "foo2 rejected");
assert.sameValue(bar1.status, "rejected", "bar1 rejected");
assert.sameValue(bar2.status, "rejected", "bar2 rejected");
assert(foo1.reason == foo2.reason, "shared errors");
assert(bar1.reason == bar2.reason, "shared errors");
assert.sameValue(importMeta.count.foo, 1, "foo initialized once");
assert.sameValue(importMeta.count.bar, 0, "bar uninitialized");
