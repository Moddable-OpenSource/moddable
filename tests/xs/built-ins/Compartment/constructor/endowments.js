/*---
description: 
flags: [onlyStrict]
---*/

let getterCount = 0;
let setterCount = 0;
const endowments = {
	foo: 0,
	get bar() {
		getterCount++;
		return this.foo;
	},
	set bar(it) {
		setterCount++;
		this.foo = it;
	},
	shared: {
		foo: 0,
		get bar() {
			return this.foo;
		},
		set bar(it) {
			this.foo = it;
		},
	}
};

const c1 = new Compartment(endowments, {}, {});
c1.evaluate(`
	foo++;
	bar++;
	shared.foo++;
	shared.bar++;
	globalThis.which = 1;
`);

const c2 = new Compartment(endowments, {}, {});
c2.evaluate(`
	foo++;
	bar++;
	shared.foo++;
	shared.bar++;
	globalThis.which = 2;
`);

assert.sameValue(getterCount, 2, "getterCount");
assert.sameValue(setterCount, 0, "setterCount");
assert.sameValue(endowments.foo, 0, "endowments.foo");
assert.sameValue(endowments.bar, 0, "endowments.bar");
assert.sameValue(endowments.shared.foo, 4, "endowments.shared.foo");
assert.sameValue(endowments.shared.bar, 4, "endowments.shared.bar");
assert.sameValue(endowments.which, undefined, "endowments.which");
assert.sameValue(c1.globalThis.foo, 1, "c1.globalThis.foo");
assert.sameValue(c1.globalThis.bar, 1, "c1.globalThis.bar");
assert.sameValue(c1.globalThis.which, 1, "c1.globalThis.which");
assert.sameValue(c2.globalThis.foo, 1, "c2.globalThis.foo");
assert.sameValue(c2.globalThis.bar, 1, "c2.globalThis.bar");
assert.sameValue(c2.globalThis.which, 2, "c2.globalThis.which");

