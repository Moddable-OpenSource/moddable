/*---
description: 
flags: [onlyStrict]
---*/

const Digital = device.io.Digital;

// Verify that pins can be reused after calling `close`
let input = new Digital({
   pin: $TESTMC.config.digital[0],
   mode: Digital.Input
});
input.close();
input = new Digital({
   pin: $TESTMC.config.digital[0],
   mode: Digital.Input
});
input.close();

// Should ignore `edge` property if no `onReadable` callback is passed in
input = new Digital({
   pin: $TESTMC.config.digital[0],
   mode: Digital.Input,
	edge: Digital.INVALID
});
input.close();

// Should NOT ignore `edge` property if `onReadable` callback is passed in
assert.throws(RangeError, () => {
	input = new Digital({
	   pin: $TESTMC.config.digital[0],
	   mode: Digital.Input,
		edge: Digital.INVALID,
		onReadable() {}
	});
}, "Digital constructor should throw an error when invalid edge is specified");
input.close();

// Check that invalid `edge` values are not accepted
const invalidEdges = [
	NaN,
	null,
	undefined,
	Digital.Falling+Digital.Falling,
]
for (let invalidEdge of invalidEdges) {
	assert.throws(RangeError, () => {
		input = new Digital({
		   pin: $TESTMC.config.digital[0],
		   mode: Digital.Input,
		   edge: invalidEdge,
		   onReadable() {}
		});
	}, `Digital constructor should throw an error when invalid edge "${invalidEdge}" is specified`);
}

input.close();
