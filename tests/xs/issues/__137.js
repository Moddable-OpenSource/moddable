/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/137
flags: [onlyStrict]
---*/

assert.sameValue(true, Object.is(JSON.parse("-0"), -0));
assert.sameValue(false, Object.is(JSON.parse("-0"), 0));
assert.sameValue(false, Object.is(JSON.parse("0"), -0));
assert.sameValue(true, Object.is(JSON.parse("0"), 0));
