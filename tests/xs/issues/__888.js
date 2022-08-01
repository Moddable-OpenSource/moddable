/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/888
---*/

assert.sameValue((new Set([NaN, Number("x"), Math.sqrt(-1), Infinity * 0, Infinity - Infinity])).size, 1, "NaN in Set");
assert.sameValue((new Map([[NaN, true]])).get(Infinity / Infinity), true, "NaN in  Map");
