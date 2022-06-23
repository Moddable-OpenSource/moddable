/*---
description: 
flags: [onlyStrict]
---*/

const content = new Text(null, { 
    left: 50, top: 50, height: 50, width: 50
});

const content2 = new Text(null, { 
    left: 50, right: 50, top: 50, bottom: 50
});

new Application;

let position = content.position;
assert.sameValue(position, undefined, `position of an unbound content object should be undefined`);

position = content2.position;  // undefined
assert.sameValue(position, undefined, `position of an unbound content object should be undefined`);

application.add(content);
application.add(content2);

position = content.position;
assert.sameValue(position.x, 50, `position.x of content should be 50`);
assert.sameValue(position.y, 50, `position.y of content should be 50`);

position = content2.position;
assert.sameValue(position.x, 50, `position.x of content2 should be 50`);
assert.sameValue(position.y, 50, `position.y of content2 should be 50`);

content.moveBy(-40, -40);
position = content.position;
assert.sameValue(position.x, 10, `position.x of content should be 10`);
assert.sameValue(position.y, 10, `position.y of content should be 10`);

content2.moveBy(-40, -40);  // Does nothing
position = content2.position;
assert.sameValue(position.x, 50, `position.x of content2 should be 50`);
assert.sameValue(position.y, 50, `position.y of content2 should be 50`);