/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/1813
flags: [onlyStrict]
---*/

// A for-let loop with no increment expression must still create a fresh
// per-iteration binding, so closures capture the value from their own iteration.

var fs = [];
for (let i = 0; i < 2; ) { fs.push(() => i); i++; }
assert.sameValue(fs.map(f => f()).join(), "1,2", "no increment expression");

var gs = [];
for (let i = 0, j = 10; i < 2; ) { gs.push(() => [i, j]); i++; j++; }
assert.sameValue(gs.map(g => g().join()).join(" "), "1,11 2,12", "two bindings, no increment expression");
