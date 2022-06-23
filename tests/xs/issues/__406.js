/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/407
flags: [onlyStrict]
---*/

assert.throws(TypeError, () => 'str'.split(Symbol(1), 0))
