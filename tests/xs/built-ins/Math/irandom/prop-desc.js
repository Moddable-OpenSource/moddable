/*---
description: Math.irandom is a non-enumerable, writable, configurable method of length 0 that is not a constructor
includes: [propertyHelper.js]
flags: [onlyStrict]
---*/

verifyProperty(Math, "irandom", {
	writable: true,
	enumerable: false,
	configurable: true
}, { restore: true });

verifyProperty(Math.irandom, "length", {
	value: 0,
	writable: false,
	enumerable: false,
	configurable: true
}, { restore: true });

verifyProperty(Math.irandom, "name", {
	value: "irandom",
	writable: false,
	enumerable: false,
	configurable: true
}, { restore: true });

assert.sameValue(Object.prototype.hasOwnProperty.call(Math.irandom, "prototype"), false,
	"a built-in method has no prototype property");

assert.throws(TypeError, function () {
	new Math.irandom();
}, "Math.irandom is not a constructor");
