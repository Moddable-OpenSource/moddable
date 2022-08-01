/*---
description: 
flags: [onlyStrict]
---*/

new Application;

assert.sameValue(application.x, 0, "application should have x coordinate 0");
assert.sameValue(application.y, 0, "application should have y coordinate 0");

application.moveBy(100,100);	// Does nothing

assert.sameValue(application.x, 0, "application should have x coordinate 0");
assert.sameValue(application.y, 0, "application should have y coordinate 0");