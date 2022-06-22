/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/704
flags: [onlyStrict]
---*/

// 1. Create a global and/or sticky regular expression.
let re = /./g;
// 2. Manually set its `lastIndex` to Infinity.
re.lastIndex = Infinity;
//   2a. This bug also manifests for some large finite values
//   re.lastIndex = 2**32 + 3;
//   2b. ...but not for others (seemingly related to the length of the string).
//   re.lastIndex = 2**32 + 4;
// 3. Execute the regular expression against a nonempty string.
re.exec("test");
// 4. Retrieve its new lastIndex. It should be 0, but is not.
assert.sameValue(0, re.lastIndex)
// => 1
