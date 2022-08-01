/*---
description: 
flags: [onlyStrict]
---*/

const one = new Content;
const two = new Content;
const three = new Content;
const four = new Content;
new Application(null, {
	contents: [ one, two ]
});

assert.sameValue(one.index, 0, "Index of `one` should be 0");
assert.sameValue(two.index, 1, "Index of `two` should be 1");
assert.sameValue(three.index, -1, "Index of `three` should be -1");
assert.sameValue(four.index, -1, "Index of `four` should be -1");

application.replace(one, three);
application.replace(two, four);
assert.sameValue(one.index, -1, "Index of `one` should be -1");
assert.sameValue(two.index, -1, "Index of `two` should be -1");
assert.sameValue(three.index, 0, "Index of `three` should be 0");
assert.sameValue(four.index, 1, "Index of `four` should be 1");

const container2 = new Container(null, {
	contents: [ one ]
})
assert.throws(Error, () => {
	application.replace(three, one);
}, "Should not be able to replace content with bound content");