/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/411
flags: [onlyStrict]
---*/

var x = Array . prototype . slice . call ( (a, b, c) => {} ) ;
assert.sameValue('["length"]', JSON.stringify(Object.getOwnPropertyNames(x)));
