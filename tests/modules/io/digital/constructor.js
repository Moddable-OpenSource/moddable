/*---
description: 
flags: [onlyStrict]
---*/

const Digital = device.io.Digital;

let input, output;

// Check that required properties in constructor are actually required
assert.throws(Error, () => {
	input = new Digital({
	   mode: Digital.Input
	});
}, `Digital constructor should throw an error when no pin property is specified`);

assert.throws(Error, () => {
	output = new Digital({
	   pin: $TESTMC.config.digital[1]
	});
}, `Digital constructor should throw an error when no mode property is specified`);

// Check that invalid property values are not accepted
let invalidPins = [
	undefined,
	500,
	-1
]
for (let invalidPin of invalidPins) {
	assert.throws(Error, () => {
		output = new Digital({
		   pin: invalidPin,
		   mode: Digital.Input,
		});
	}, `Digital constructor should throw an error when invalid pin "${invalidPin}" is specified`);
}

let invalidModes = [
	-1, 
	500
]
for (let invalidMode of invalidModes) {
	assert.throws(RangeError, () => {			// RangeError (digitalBank.c)
		output = new Digital({
		   pin: $TESTMC.config.digital[1],
		   mode: invalidMode,
		});
	}, `Digital constructor should throw an error when invalid mode "${invalidMode}" is specified`);
}

invalidModes = [
	undefined
]
for (let invalidMode of invalidModes) {
	assert.throws(Error, () => {					// Error (builtinGetSignedInteger in builtinCommon.c)
		output = new Digital({
		   pin: $TESTMC.config.digital[1],
		   mode: invalidMode,
		});
	}, `Digital constructor should throw an error when invalid mode "${invalidMode}" is specified`);
}

output = new Digital({
   pin: $TESTMC.config.digital[1],
   mode: Digital.Output,
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
	}, `Digital instance should throw an error when invalid format "${invalidFormat}" is specified`);
}
