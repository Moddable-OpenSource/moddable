/*---
description:
features: [harden]
flags: [onlyStrict]
---*/

lockdown();

class Class {
	constructor(it) {
		this.property = it;
	}
}

const object = new Class({});
Object.freeze(object);
assert.sameValue(Object.isFrozen(object), true, "frozen object is frozen");
assert.sameValue(Object.isFrozen(Object.getPrototypeOf(object)), false, "frozen object prototype is not frozen");
assert.sameValue(Object.isFrozen(object.property), false, "frozen object property is not frozen");
harden(object);
assert.sameValue(Object.isFrozen(object), true, "hardened object is frozen");
assert.sameValue(Object.isFrozen(Object.getPrototypeOf(object)), true, "hardened object prototype is frozen");
assert.sameValue(Object.isFrozen(object.property), true, "hardened object property is frozen");

