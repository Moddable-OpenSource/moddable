/*---
description: 
flags: [onlyStrict]
---*/

const DigitalBank = device.io.DigitalBank;

// Verify that pins can be reused after calling `close`
let input = new DigitalBank({
	pins: 1,
	mode: DigitalBank.Input
});
input.close();
input = new DigitalBank({
	pins: 1,
	mode: DigitalBank.Input
});
input.close();

// Should ignore `rises` and `falls` properties if no `onReadable` callback is passed in
input = new DigitalBank({
	pins: 1,
	mode: DigitalBank.Input,
	rises: 0,
	falls: 0
});
input.close();

// Should NOT ignore `rises` and `falls` properties if `onReadable` callback is passed in
assert.throws(RangeError, () => {
	input = new DigitalBank({
		pins: 1,
		mode: DigitalBank.Input,
		rises: 0,
		falls: 0,
		onReadable() {}
	});
}, "DigitalBank constructor should throw an error when rises and falls are both 0");
input.close();

// Verify that valid `rises` and `falls` values work
input = new DigitalBank({
	pins: 1,
	mode: DigitalBank.Input,
	rises: 1,
	falls: 0,
	onReadable() {}
});
input.close();

input = new DigitalBank({
	pins: 1,
	mode: DigitalBank.Input,
	rises: 0,
	falls: 1,
	onReadable() {}
});
input.close();

// // Check that invalid `rises` and `falls` values are not accepted
const invalidEdges = [
	1 << 3,
	NaN,
	null,
	undefined
]
for (let invalidEdge of invalidEdges) {
	assert.throws(RangeError, () => {
		input = new DigitalBank({
			pins: 1,
			mode: DigitalBank.Input,
			rises: invalidEdge,
			onReadable() {}
		});
	}, `DigitalBank constructor should throw an error when invalid rises value "${invalidEdge}" is specified`);
	input.close();
	assert.throws(RangeError, () => {
		input = new DigitalBank({
			pins: 1,
			mode: DigitalBank.Input,
			falls: invalidEdge,
			onReadable() {}
		});
	}, `DigitalBank constructor should throw an error when invalid falls value "${invalidEdge}" is specified`);
	input.close();
}