/*---
description: 
flags: [onlyStrict]
---*/


new Application;

let position = application.position;
assert.sameValue(position.x, 0, `position.x of application should be 0`);
assert.sameValue(position.y, 0, `position.y of application should be 0`);

application.moveBy(-40, -40);
assert.sameValue(position.x, 0, `position.x of application should be 0`);
assert.sameValue(position.y, 0, `position.y of application should be 0`);