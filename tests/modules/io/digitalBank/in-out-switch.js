/*---
description: 
flags: [onlyStrict]
---*/

const DigitalBank = device.io.DigitalBank;

const INPUT_PIN = $TESTMC.config.digital[0];
const OUTPUT_PIN = $TESTMC.config.digital[1];

let input = new DigitalBank({
	pins: 1 << INPUT_PIN,
	mode: DigitalBank.Input
});
let output = new DigitalBank({
	pins: 1 << OUTPUT_PIN,
	mode: DigitalBank.Output
});

output.write(1 << OUTPUT_PIN);
assert.sameValue(input.read() , 1 << INPUT_PIN, `Read value should be 0b${(1 << INPUT_PIN).toString(2)}`);
input.close();
output.close();

// Swap pins and try again
input = new DigitalBank({
	pins: 1 << OUTPUT_PIN,
	mode: DigitalBank.Input
});
output = new DigitalBank({
	pins: 1 << INPUT_PIN,
	mode: DigitalBank.Output
});

output.write(1 << INPUT_PIN);
assert.sameValue(input.read() , 1 << OUTPUT_PIN, `Read value should be 0b${(1 << OUTPUT_PIN).toString(2)}`);
input.close();
output.close();
