/*---
description: 
flags: [onlyStrict]
---*/

const DigitalBank = device.io.DigitalBank;

let input, output;

// Check that required properties in constructor are actually required
assert.throws(Error, () => {
	input = new DigitalBank({
		mode: DigitalBank.Input
	});
}, `DigitalBank constructor should throw an error when no pins property is specified`);

assert.throws(Error, () => {
	output = new DigitalBank({
		pins: 1
	});
}, `DigitalBank constructor should throw an error when no mode property is specified`);

// Check that invalid property values are not accepted
let invalidPins = [
	0,
	undefined,
	null,
	// -1	 // TO DO: put this back after bug fix
]
for (let invalidPin of invalidPins) {
	assert.throws(Error, () => {
		output = new DigitalBank({
			pins: invalidPin,
			mode: DigitalBank.Input,
		});
	}, `DigitalBank constructor should throw an error when invalid pins value "${0}" is specified`);
}

let invalidModes = [
	-1, 
	500
]
for (let invalidMode of invalidModes) {
	assert.throws(RangeError, () => {			// RangeError (digitalBank.c)
		output = new DigitalBank({
			pins: 1,
			mode: invalidMode,
		});
	}, `DigitalBank constructor should throw an error when invalid mode "${invalidMode}" is specified`);
}

invalidModes = [
	undefined
]
for (let invalidMode of invalidModes) {
	assert.throws(Error, () => {					// Error (builtinGetSignedInteger in builtinCommon.c)
		output = new DigitalBank({
			pins: 1,
			mode: invalidMode,
		});
	}, `DigitalBank constructor should throw an error when invalid mode "${invalidMode}" is specified`);
}

output = new DigitalBank({
	pins: 1,
	mode: DigitalBank.Output,
});
const invalidFormats = [
	null,
	undefined,
	123,
	"buffer",
	"object",
	"string;ascii",
	"string;utf8"
]
for (let invalidFormat of invalidFormats) {
	assert.throws(RangeError, () => {
		output.format = invalidFormat;
	}, `DigitalBank instance should throw an error when invalid format "${invalidFormat}" is specified`);
}