/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/735
flags: [onlyStrict]
---*/

assert.sameValue((-2147483648 | 0) % -1, -0, "(-2147483648 | 0) % -1");
