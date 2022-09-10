/*---
description: 
flags: [async, onlyStrict]
---*/

let getterCount = 0;
let setterCount = 0;
let neverCount = 0;
const object = Object.create(
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
				return object.foo;
			},
			set: function(it) {
				setterCount++;
				object.foo = it;
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

const c1 = new Compartment({ 
	modules: {
		foo: { namespace: object },
	},
});
const c2 = new Compartment({ 
	modules: {
		foo: { namespace: object },
	},
});

Promise.all([
	c1.import("foo"),
	c2.import("foo"),
])
.then(([ foo1, foo2 ]) => {
	assert.sameValue(getterCount, 2, "getterCount");
	assert.sameValue(setterCount, 0, "setterCount");
	assert.sameValue(setterCount, 0, "neverCount");
	assert.sameValue(foo1.foo, 0, "foo1.foo");
	assert.sameValue(foo1.bar, 0, "foo1.bar");
	assert.sameValue(foo2.foo, 0, "foo1.foo");
	assert.sameValue(foo2.bar, 0, "foo1.bar");
	assert.sameValue(foo1.shared, foo2.shared, "shared");
})
.then($DONE, $DONE);
