/*---
description: 
flags: [onlyStrict]
---*/

new Application;

assert.sameValue(application.measure().width, 240, "`application`'s measured width should be 240");
assert.sameValue(application.measure().height, 320, "`application`'s measured height should be 320");