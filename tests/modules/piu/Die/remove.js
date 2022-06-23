/*---
description: 
flags: [onlyStrict]
---*/

const sampleContent = new Content;
const sampleContainer = new Container(null, { contents: [ sampleContent ]});

const one = new Content;
const two = new Content;
const three = new Content;
const container = new Die(null, {
	contents: [ one, two ]
});

assert.sameValue(three.index, -1, "Index of `three` should be -1");
container.remove(one);
assert.sameValue(one.index, -1, "Index of `one` should be -1");
assert.sameValue(two.index, 0, "Index of `two` should be 0");

assert.throws(Error, () => {
	container.remove(sampleContent);
}, "Should not be able to remove contents from other containers");
