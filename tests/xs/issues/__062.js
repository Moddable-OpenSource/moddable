/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/62
flags: [onlyStrict]
---*/

let q = 2000000000;
assert.sameValue("2000000000", q.toString());
q -= -2000000000;
assert.sameValue("4000000000", q.toString());
