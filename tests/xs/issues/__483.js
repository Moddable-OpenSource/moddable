/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/483
flags: [onlyStrict]
---*/

assert.throws(SyntaxError, () => eval(1e7 + "nu"));
