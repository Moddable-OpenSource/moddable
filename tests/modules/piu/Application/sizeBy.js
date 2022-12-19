/*---
description: 
flags: [onlyStrict]
---*/

new Application;

assert.sameValue(application.width, 240, "application should have width 240");
assert.sameValue(application.height, 320, "application should have height 320");

application.sizeBy(100,100);

assert.sameValue(application.width, 340, "application should have width 240");
assert.sameValue(application.height, 420, "application should have height 320");