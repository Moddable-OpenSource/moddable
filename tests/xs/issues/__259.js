/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/259
flags: [onlyStrict]
---*/

const undef = undefined;
assert.sameValue("undef:undefined", `undef:${undef}`);
