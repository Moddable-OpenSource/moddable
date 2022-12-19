/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/410
flags: [onlyStrict]
---*/

var x = undefined;
({x: (x)} = {x: 42});
assert.sameValue("42", JSON.stringify(x));
