/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/1803
flags: [onlyStrict]
---*/

"use strict";
var b = [1, 2, 3, 4];
Object.defineProperty(b, 1, { value: 9, configurable: false });
var threw = false;
try {
	b.length = 0;
} catch (e) {
	threw = e instanceof TypeError;
}
if (!threw)
	throw new Error("expected TypeError on strict length truncation");
if (b.length !== 2)
	throw new Error("expected length 2, got " + b.length);
