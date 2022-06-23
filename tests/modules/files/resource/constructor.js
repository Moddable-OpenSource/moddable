/*---
description: 
flags: [module]
---*/

import Resource from "Resource";

const r = new Resource("image-info.txt");
assert.sameValue("object", typeof r, "returns object");

if (!new Resource({
	[Symbol.toPrimitive]() {
		return "image-info.txt";
	}
})) {
	assert(false, "no resource");
}

assert.throws(URIError, () => new Resource("NOT FOUND"), "no resource")
assert.throws(URIError, () => new Resource(""), "no resource")
assert.throws(SyntaxError, () => new Resource(), "no path")
assert.throws(TypeError, () => new Resource(Symbol()), "no path")
