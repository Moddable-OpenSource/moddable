/*---
description: 
flags: [onlyStrict]
---*/

const one = new Content;
const two = new Content;
const three = new Content;
const container = new Column(null, {
	contents: [ three ]
});

assert.sameValue(three.index, 0, `Index of 'three' should be 0`);

container.insert(one, three);
assert.sameValue(one.index, 0, ` Index of 'one' should be 0`);
assert.sameValue(three.index, 1, `Index of 'three' should be 1`);

container.insert(two, three);
assert.sameValue(one.index, 0, `Index of 'one' should be 0`);
assert.sameValue(two.index, 1, `Index of 'two' should be 1`);
assert.sameValue(three.index, 2, `Index of 'three' should be 2`);