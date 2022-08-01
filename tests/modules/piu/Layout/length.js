/*---
description: 
flags: [onlyStrict]
---*/

const one = new Content;
const two = new Content;
const three = new Content;
const container = new Layout(null, {
	contents: [ one, two, three ]
});

assert.sameValue(container.length, 3, `Container length should be 3`);

container.remove(three);
assert.sameValue(container.length, 2, `Container length should be 2`);

container.add(three);
assert.sameValue(container.length, 3, `Container length should be 3`);

container.empty();
assert.sameValue(container.length, 0, `Container length should be 0`);