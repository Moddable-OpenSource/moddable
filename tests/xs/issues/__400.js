/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/400
flags: [onlyStrict]
---*/

var x = { 42 : class   { x (  ) {  } } } ;
assert.sameValue("42", (x['42']['name']));
