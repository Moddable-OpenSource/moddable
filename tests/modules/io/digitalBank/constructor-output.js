/*---
description: 
flags: [onlyStrict]
---*/

const DigitalBank = device.io.DigitalBank;

let output = new DigitalBank({
	pins: 1,
	mode: DigitalBank.Output
});

// Same pin can't be used by two instances of the `DigitalBank` class configured as output
assert.throws(Error, () => {
	let duplicate = new DigitalBank({
		pins: 1,
		mode: DigitalBank.Output,
	});
}, "DigitalBank constructor should throw an error when pin specified is already in use");

// Verify that pins can be reused after calling `close`
output.close();
output = new DigitalBank({
	pins: 1,
	mode: DigitalBank.Output
});
output.close();