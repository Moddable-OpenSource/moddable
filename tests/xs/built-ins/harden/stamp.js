/*---
description:
features: [harden]
flags: [onlyStrict]
---*/

lockdown();

const object = {};
const frozenObject = Object.freeze({});
const hardenedObject = harden({});

class Stamper extends class {
  constructor(obj) {
    return obj;
  }
} {
  #stamp = "oops";
  static getStamp(obj) {
    return obj.#stamp;
  }
}

function test(it) {
	return function() {
		new Stamper(it);
		return Stamper.getStamp(it);
	}
}

assert.sameValue(test(object)(), "oops", "object can be stamped");
assert.sameValue(test(frozenObject)(), "oops", "frozen object can be stamped");
assert.throws(TypeError, test(hardenedObject), "hardened object cannot be stamped");
