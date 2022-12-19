/*---
description: 
flags: [onlyStrict]
---*/

assert.sameValue("" > "\0", false, '"" > "\0"');
assert.sameValue("\0" > "\0\0", false, '"\0" > "\0\0"');
assert.sameValue("\0\0ps" > "oops", false, '"\0\0ps" > "oops"');
