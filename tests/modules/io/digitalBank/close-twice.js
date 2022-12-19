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
output.close();

let input = new DigitalBank({
	pins: 1,
	mode: DigitalBank.Input
});
input.close();
input.close();
