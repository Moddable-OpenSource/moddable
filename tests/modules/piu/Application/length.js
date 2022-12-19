/*---
description: 
flags: [onlyStrict]
---*/

const one = new Content;
const two = new Content;
const three = new Content;
new Application(null, {
	contents: [ one, two, three ]
});

assert.sameValue(application.length, 3, `Container length should be 3`);

application.remove(three);
assert.sameValue(application.length, 2, `Container length should be 2`);

application.add(three);
assert.sameValue(application.length, 3, `Container length should be 3`);

application.empty();
assert.sameValue(application.length, 0, `Container length should be 0`);