/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/652
flags: [onlyStrict]
---*/

const x = 65n;
assert.sameValue(x.toString(), "65");
