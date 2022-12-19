/*---
description: 
flags: [onlyStrict]
---*/

const DigitalBank = device.io.DigitalBank;

const output = new DigitalBank({
	pins: 1,
	mode: DigitalBank.Output
});
output.close();
assert.throws(SyntaxError, () => {
	output.write(1);
}, "Should not be able to write to output after closing");

let input = new DigitalBank({
	pins: 1,
	mode: DigitalBank.Input
});
input.close();
assert.throws(SyntaxError, () => {
	input.read();
}, "Should not be able to read from input after closing");
