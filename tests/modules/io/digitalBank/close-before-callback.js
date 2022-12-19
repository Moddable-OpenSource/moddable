/*---
description: 
flags: [async, module]
---*/

import Timer from "timer";
const DigitalBank = device.io.DigitalBank

const INPUT_PIN = $TESTMC.config.digital[0];
const OUTPUT_PIN = $TESTMC.config.digital[1];

const output = new DigitalBank({
	pins: 1 << OUTPUT_PIN,
	mode: DigitalBank.Output
});
output.write(0);

const input = new DigitalBank({
	pins: 1 << INPUT_PIN,
	mode: DigitalBank.Input,
	rises: 1 << INPUT_PIN,
	onReadable(triggered) {
		$DONE("Shouldn't have executed callback after close was called");
	}
});

output.write(1 << OUTPUT_PIN);
input.close();
Timer.set(() => $DONE(), 100);
