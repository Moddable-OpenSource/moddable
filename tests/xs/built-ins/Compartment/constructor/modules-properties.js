/*---
description: 
flags: [async, onlyStrict]
---*/

let getterCount = 0;
let setterCount = 0;
let neverCount = 0;
const modules = Object.create(
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
			value: { source: new ModuleSource(`export default 0`) }
		},
		bar: { 
			enumerable: true,
			get: function() {
				getterCount++;
				return modules.foo;
			},
			set: function(it) {
				setterCount++;
				modules.foo = it;
			},
		},
	}
);

const c1 = new Compartment({ modules });
const c2 = new Compartment({ modules });

assert.sameValue(getterCount, 2, "getterCount");
assert.sameValue(setterCount, 0, "setterCount");
assert.sameValue(neverCount, 0, "neverCount");

Promise.all([
	c1.import("foo"),
	c1.import("bar"),
	c2.import("foo"),
	c2.import("bar"),
])
.then(([ foo1, bar1, foo2, bar2 ]) => {
	assert(foo1 != bar1, "separate namepaces");
	assert(foo2 != bar2, "separate namepaces");
	assert(foo1 != foo2, "separate namepaces");
	assert(bar1 != bar2, "separate namepaces");
})
.then($DONE, $DONE);
