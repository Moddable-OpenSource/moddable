/*---
description: 
flags: [onlyStrict]
---*/

let getterCount = 0;
let setterCount = 0;
const globalLexicals = {
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

const c1 = new Compartment({ assert }, {}, { globalLexicals});
c1.evaluate(`
	foo++;
	bar++;
	shared.foo++;
	shared.bar++;
`);

const c2 = new Compartment({ assert }, {}, { globalLexicals });
c2.evaluate(`
	foo++;
	bar++;
	shared.foo++;
	shared.bar++;
`);

assert.sameValue(getterCount, 2, "getterCount");
assert.sameValue(setterCount, 0, "setterCount");
assert.sameValue(globalLexicals.foo, 0, "globalLexicals.foo");
assert.sameValue(globalLexicals.bar, 0, "globalLexicals.bar");
assert.sameValue(globalLexicals.shared.foo, 4, "globalLexicals.shared.foo");
assert.sameValue(globalLexicals.shared.bar, 4, "globalLexicals.shared.bar");
c1.evaluate(`
	assert.sameValue(foo, 1, "c1.globalLexicals.foo");
	assert.sameValue(bar, 1, "c1.globalLexicals.bar");
`);
assert.sameValue(c1.globalThis.foo, undefined, "c1.globalThis.foo");
assert.sameValue(c1.globalThis.bar, undefined, "c1.globalThis.bar");
c2.evaluate(`
	assert.sameValue(foo, 1, "c2.globalLexicals.foo");
	assert.sameValue(bar, 1, "c2.globalLexicals.bar");
`);
assert.sameValue(c2.globalThis.foo, undefined, "c2.globalThis.foo");
assert.sameValue(c2.globalThis.bar, undefined, "c2.globalThis.bar");

