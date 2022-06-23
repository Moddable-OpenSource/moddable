/*---
description: 
flags: [onlyStrict]
---*/

const one = new Content;
const two = new Content;
const container = new Layout(null, {
	contents: [ one, two ]
});

assert.sameValue(one.index, 0, "Index of `one` should be 0");
assert.sameValue(two.index, 1, "Index of `two` should be 1");

container.swap(one, two);
assert.sameValue(one.index, 1, "Index of `one` should be 1");
assert.sameValue(two.index, 0, "Index of `two` should be 0");

const sampleContent = new Content;
const container2 = new Container(null, {
    contents: [ sampleContent ]
});
assert.throws(Error, () => {
	container.swap(one, sampleContent);
}, "Should not be able to swap content with content in another container");