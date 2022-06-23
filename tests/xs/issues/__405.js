/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/405
flags: [onlyStrict]
---*/

var x = Map . prototype . keys . call ( new Map ( ) ) ;
assert.sameValue("[]", JSON.stringify(Object.getOwnPropertyNames(x)));

x = Set . prototype . values . call ( new Set ( ) ) ;
assert.sameValue("[]", JSON.stringify(Object.getOwnPropertyNames(x)));
