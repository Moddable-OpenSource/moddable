/*---
description: 
flags: [onlyStrict]
---*/

assert.sameValue("" < "\0", true, '"" < "\0"');
assert.sameValue("\0" < "\0\0", true, '"\0" < "\0\0"');
assert.sameValue("\0\0ps" < "oops", true, '"\0\0ps" < "oops"');
