/*---
description: 
flags: [async]
---*/

const DigitalBank = device.io.DigitalBank;

const INPUT_PIN = $TESTMC.config.digital[0];
const OUTPUT_PIN = $TESTMC.config.digital[1];

let count = 0;

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
		if ((this.read() == (1 << INPUT_PIN)) && (count == 1)) {
			$DONE();
		} else {
			$DONE(`onReadable was triggered ${count} times, but only should have been triggered 1 time`);
		}
	}
});

output.write(0);
output.write(0);
output.write(0);
output.write(1 << OUTPUT_PIN);
output.write(1 << OUTPUT_PIN);
output.write(1 << OUTPUT_PIN);
$TESTMC.timeout(100, "`onReadable` should have been triggered");
