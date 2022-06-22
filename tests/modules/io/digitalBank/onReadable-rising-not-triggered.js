/*---
description: 
flags: [async, module]
---*/

import Timer from "timer";
const DigitalBank = device.io.DigitalBank;

const INPUT_PIN = $TESTMC.config.digital[0];
const OUTPUT_PIN = $TESTMC.config.digital[1];

const output = new DigitalBank({
	pins: 1 << OUTPUT_PIN,
	mode: DigitalBank.Output
});
output.write(1 << OUTPUT_PIN);

const input = new DigitalBank({
	pins: 1 << INPUT_PIN,
	rises: 1 << INPUT_PIN,
	mode: DigitalBank.Input,
	onReadable(triggers) {
		$DONE("`onReadable` should not have been triggered");
	}
});

output.write(0);
Timer.set(() => $DONE(), 100);
