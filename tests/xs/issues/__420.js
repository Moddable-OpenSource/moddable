/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/420
flags: [onlyStrict]
---*/

var x = (function * () {}) ();
assert.sameValue(0, Object.getOwnPropertySymbols(x).length);
