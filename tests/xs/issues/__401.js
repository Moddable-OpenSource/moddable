/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/401
flags: [onlyStrict]
---*/

var x = [ , ... '' ] ;
assert.sameValue(1, x['length']);
