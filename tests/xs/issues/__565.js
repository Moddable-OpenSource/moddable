/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/565
flags: [onlyStrict]
---*/

const { default: greeting } = { default: 1 }
const { in: stuff } = { in: 1 };

assert.sameValue(greeting, 1);
assert.sameValue(stuff, 1);
