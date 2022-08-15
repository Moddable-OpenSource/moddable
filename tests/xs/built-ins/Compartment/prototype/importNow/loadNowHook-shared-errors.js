/*---
description: 
flags: [onlyStrict]
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

const c1 = new Compartment({
	resolveHook, 
	loadNowHook(specifier) {
		return modules[specifier];
	}
});
const c2 = new Compartment({
	resolveHook, 
	loadNowHook(specifier) {
		return { namespace:specifier, compartment:c1 };
	}
});

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
