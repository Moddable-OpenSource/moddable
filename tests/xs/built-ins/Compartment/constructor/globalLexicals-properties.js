/*---
description: 
flags: [onlyStrict]
---*/

let getterCount = 0;
let setterCount = 0;
let neverCount = 0;
const globalLexicals = Object.create(
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
			writable: true,
			value: 0
		},
		bar: { 
			enumerable: true,
			get: function() {
				getterCount++;
				return globalLexicals.foo;
			},
			set: function(it) {
				setterCount++;
				globalLexicals.foo = it;
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


const c1 = new Compartment({ globalLexicals });
c1.evaluate(`
	foo++;
	bar++;
	shared.foo++;
	shared.bar++;
	try {
		shared = null;
	}
	catch (e) {
		// const
	}
`);

const c2 = new Compartment({ globalLexicals });
c2.evaluate(`
	foo++;
	bar++;
	shared.foo++;
	shared.bar++;
	try {
		shared = null;
	}
	catch (e) {
		// const
	}
`);

assert.sameValue(getterCount, 2, "getterCount");
assert.sameValue(setterCount, 0, "setterCount");
assert.sameValue(neverCount, 0, "neverCount");
assert.sameValue(globalLexicals.foo, 0, "globalLexicals.foo");
assert.sameValue(globalLexicals.bar, 0, "globalLexicals.bar");
assert.sameValue(globalLexicals.shared.foo, 4, "globalLexicals.shared.foo");
assert.sameValue(globalLexicals.shared.bar, 4, "globalLexicals.shared.bar");
assert.sameValue(c1.evaluate("foo"), 1, "c1.globalLexicals.foo");
assert.sameValue(c1.evaluate("bar"), 1, "c1.globalLexicals.bar");
assert.sameValue(c1.globalThis.foo, undefined, "c1.globalThis.foo");
assert.sameValue(c1.globalThis.bar, undefined, "c1.globalThis.bar");
assert.sameValue(c2.evaluate("foo"), 1, "c2.globalLexicals.foo");
assert.sameValue(c2.evaluate("bar"), 1, "c2.globalLexicals.bar");
assert.sameValue(c2.globalThis.foo, undefined, "c2.globalThis.foo");
assert.sameValue(c2.globalThis.bar, undefined, "c2.globalThis.bar");

