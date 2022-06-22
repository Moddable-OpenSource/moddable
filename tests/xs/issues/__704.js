/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/704
flags: [onlyStrict]
---*/

assert.sameValue(2147483648, -(1 << -1))
