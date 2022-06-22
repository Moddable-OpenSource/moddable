/*---
description: 
flags: [onlyStrict]
---*/

const container = new Scroller(null, {
	contents: [
		new Content,
		new Content,
		new Content,
		new Content,
		new Content
	]
});

assert.sameValue(container.length, 5, `Container should have length 5`);

container.empty(-1);
assert.sameValue(container.length, 4, `Container should have length 4`);

container.empty(2, 3)
assert.sameValue(container.length, 3, `Container should have length 3`);

container.empty();
assert.sameValue(container.length, 0, `Container should have length 0`);

