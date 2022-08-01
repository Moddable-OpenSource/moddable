/*---
description: 
flags: [async]
---*/

const DigitalBank = device.io.DigitalBank;

const INPUT_PIN = $TESTMC.config.digital[0];
const OUTPUT_PIN = $TESTMC.config.digital[1];

let count = 0, value = 1;

const output = new DigitalBank({
	pins: 1 << OUTPUT_PIN,
	mode: DigitalBank.Output
});
output.write(0);

const input = new DigitalBank({
	pins: 1 << INPUT_PIN,
	rises: 1 << INPUT_PIN,
	falls: 1 << INPUT_PIN,
	mode: DigitalBank.Input,
	onReadable(triggers) {
		count++;
		if (count < 20) {
			value = !value;
			output.write(value << OUTPUT_PIN);
		} else {
			$DONE();
		}
	}
});

output.write(value << OUTPUT_PIN);
$TESTMC.timeout(100, "`onReadable` should have been triggered");
