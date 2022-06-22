/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/577
flags: [onlyStrict]
---*/

const store = new Map([[1n, "abc"]])
assert.sameValue(store.get(1n), "abc");
