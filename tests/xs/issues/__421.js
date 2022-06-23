/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/421
flags: [onlyStrict]
---*/

var x = Object . defineProperty ( { } , "p" , { [ "get" ] : ( ) => { } , [ "set" ] : ( x ) => { } } ) ;
assert.sameValue("get", (Object.getOwnPropertyDescriptor(x, "p").get.name))
assert.sameValue(("set", Object.getOwnPropertyDescriptor(x, "p").set.name));
