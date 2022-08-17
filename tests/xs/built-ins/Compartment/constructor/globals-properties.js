/*---
description: 
flags: [onlyStrict]
---*/

let getterCount = 0;
let setterCount = 0;
let neverCount = 0;
const globals = Object.create(
	{ 
		get x() {
			neverCount++;
		}
	},
	{
		y: {
			get: function() {
				neverCount++;
			}
		},
		[Symbol('z')]: {
			enumerable: true,
			get function() {
				neverCount++;
			}
		},
		foo: { 
			enumerable: true,
			value: 0
		},
		bar: { 
			enumerable: true,
			get: function() {
				getterCount++;
				return globals.foo;
			},
			set: function(it) {
				setterCount++;
				globals.foo = it;
			},
		},
		shared: {
			enumerable: true,
			value: {
				foo: 0,
				get bar() {
					return this.foo;
				},
				set bar(it) {
					this.foo = it;
				},
			}
		}
	}
);

const c1 = new Compartment({ globals });
c1.evaluate(`
	foo++;
	bar++;
	shared.foo++;
	shared.bar++;
	globalThis.which = 1;
`);

const c2 = new Compartment({ globals });
c2.evaluate(`
	foo++;
	bar++;
	shared.foo++;
	shared.bar++;
	globalThis.which = 2;
`);

assert.sameValue(getterCount, 2, "getterCount");
assert.sameValue(setterCount, 0, "setterCount");
assert.sameValue(setterCount, 0, "neverCount");
assert.sameValue(globals.foo, 0, "globals.foo");
assert.sameValue(globals.bar, 0, "globals.bar");
assert.sameValue(globals.shared.foo, 4, "globals.shared.foo");
assert.sameValue(globals.shared.bar, 4, "globals.shared.bar");
assert.sameValue(globals.which, undefined, "globals.which");
assert.sameValue(c1.globalThis.foo, 1, "c1.globalThis.foo");
assert.sameValue(c1.globalThis.bar, 1, "c1.globalThis.bar");
assert.sameValue(c1.globalThis.which, 1, "c1.globalThis.which");
assert.sameValue(c2.globalThis.foo, 1, "c2.globalThis.foo");
assert.sameValue(c2.globalThis.bar, 1, "c2.globalThis.bar");
assert.sameValue(c2.globalThis.which, 2, "c2.globalThis.which");

