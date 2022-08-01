/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/748
flags: [onlyStrict]
---*/

function assertThrows() {
	throw new Error;
}

Function.prototype[Symbol.hasInstance].call({}, {
	"getPrototypeOf": assertThrows
});
