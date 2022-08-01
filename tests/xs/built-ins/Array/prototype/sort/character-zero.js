/*---
description: 
flags: [onlyStrict]
---*/

const a = [ "oops", "\0\0", "", "\0" ];
a.sort();
assert.sameValue(a[0], "", "0");
assert.sameValue(a[1], "\0", "1");
assert.sameValue(a[2], "\0\0", "2");
assert.sameValue(a[3], "oops", "3");
