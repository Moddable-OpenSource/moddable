/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/64
flags: [onlyStrict]
---*/

assert.throws(ReferenceError, () => {'use strict'; eval('e=1'); const e = 0;});
assert.throws(ReferenceError, () => {'use strict'; eval('e=1'); let e = 0;});
