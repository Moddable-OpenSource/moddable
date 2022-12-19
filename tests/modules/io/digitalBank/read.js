/*---
description: 
flags: [onlyStrict]
---*/

const DigitalBank = device.io.DigitalBank;

const INPUT_PIN = $TESTMC.config.digital[0];
const OUTPUT_PIN = $TESTMC.config.digital[1];

// Digital IO instance configured as an output does NOT implement `read`
const output = new DigitalBank({
	pins: 1 << OUTPUT_PIN,
	mode: DigitalBank.Output
});
assert.throws(Error, () => {
	output.read();
}, "`read` should not be implemented for digital output");
output.close();

// Digital IO instance configured as an input does implement `read`
const input = new DigitalBank({
	pins: 1 << INPUT_PIN,
	mode: DigitalBank.Input
});

input.read();

// `read` should not be callable after calling `close`
input.close();
assert.throws(SyntaxError, () => {
	input.read();
}, `Cannot read after pin is closed`);
