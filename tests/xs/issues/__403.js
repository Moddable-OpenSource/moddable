/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/403
flags: [onlyStrict]
---*/

var x = undefined;
var x = x?.();

assert.sameValue(undefined, x);
