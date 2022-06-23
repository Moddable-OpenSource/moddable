/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/335
flags: [onlyStrict]
---*/

assert.sameValue(true, 1 / (0 * -1) < 0);
assert.sameValue(true, 1 / (-1 * 0) < 0);
