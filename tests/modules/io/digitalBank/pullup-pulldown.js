/*---
description: 
flags: [onlyStrict]
---*/

// Note: The ESP32 has built-in pull-up/pull-down resistors on all GPIO pins except pins 34–39
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html

const DigitalBank = device.io.DigitalBank;

let input = new DigitalBank({
	pins: 1 << 14,
	mode: DigitalBank.InputPullUp
});
assert.sameValue(input.read(), 1 << 14, `Read value should be 1`);

input.close();

input = new DigitalBank({
	pins: 1 << 14,
	mode: DigitalBank.InputPullDown
});
assert.sameValue(input.read(), 0, `Read value should be 0`);

// TO DO (maybe): DigitalBank.InputPullUpDown