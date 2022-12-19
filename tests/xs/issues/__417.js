/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/417
flags: [onlyStrict]
---*/

var x = Function . prototype . bind . call ( ( ) => { } , ( Symbol . replace ) ) ;
assert.sameValue('["length","name"]', JSON.stringify(Object.getOwnPropertyNames(x)));
