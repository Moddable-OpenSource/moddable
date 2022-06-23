/*---
description: 
flags: [onlyStrict]
---*/

const DigitalBank = device.io.DigitalBank;

const INPUT_PIN = $TESTMC.config.digital[0];
const OUTPUT_PIN = $TESTMC.config.digital[1];

// Digital bank IO instance configured as an input does NOT implement `write`
const input = new DigitalBank({
	pins: 1 << INPUT_PIN,
	mode: DigitalBank.Input
});
assert.throws(Error, () => {
	input.write(0);
}, "`write` should not be implemented for digital bank input");
input.close();


// Digital bank IO instance configured as an output does implement `write`
const output = new DigitalBank({
	pins: 1 << OUTPUT_PIN,
	mode: DigitalBank.Output
});
output.write(0);

// `write` should only accept input that can be coerced to numbers
let invalidInputs = [
	Symbol(),
	BigInt("1")
]
for (let invalidInput of invalidInputs) {
	assert.throws(TypeError, () => {
		output.write(invalidInput);
	}, `Digital write should throw an error when input that can't be coerced to a number is given`);
}

// `write` should not be callable after calling `close`
output.close();
assert.throws(SyntaxError, () => {
	output.write(0);
}, `Cannot write after pin is closed`);
