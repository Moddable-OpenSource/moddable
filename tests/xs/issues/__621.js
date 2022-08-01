/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/621
flags: [onlyStrict]
---*/

const x = Symbol.for('bar').toString();
assert.sameValue(x, "Symbol(bar)");
