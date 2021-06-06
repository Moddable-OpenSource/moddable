/*
* Copyright (c) 2021  Moddable Tech, Inc.
*
*   This file is part of the Moddable SDK.
*
*   This work is licensed under the
*       Creative Commons Attribution 4.0 International License.
*   To view a copy of this license, visit
*       <http://creativecommons.org/licenses/by/4.0>.
*   or send a letter to Creative Commons, PO Box 1866,
*   Mountain View, CA 94042, USA.
*
*/

import Timer from  "timer";
const Digital = device.io.Digital;
const SPI = device.io.SPI;

const CTRLY = 0b10010011;
const CTRLX = 0b11010011;
const CTRL_RESET = 0b11010100;

// pin 16 on ESP8266 doesn't support an interrupt, so this polls instead of using onReadable
const touchPin = new Digital({
	pin: 16,
	mode: Digital.Input
});
touchPin.state = 0;
Timer.repeat(() => {
	if (touchPin.state !== touchPin.read())
		touchPin.state = 1 - touchPin.state;
}, 33);

const spi = new SPI({
	...device.SPI.default,
	hz: 1_000_000,
	select: 0,
	active: 0
});

const calibrate = {
	left: 1941,
	right: 107,
	top: 1980,
	bottom: 186,

	width: 240,
	height: 320
};

Timer.repeat(() => {
	spi.write(Uint8Array.of(CTRL_RESET));
	spi.flush(true);

	if (touchPin.state)
		return;

	const sample = Uint16Array.of(CTRLX);
	spi.transfer(sample);
	let x = sample[0] >> 4;

	sample[0] = CTRLY;
	spi.transfer(sample);
	let y = sample[0] >> 4;

	spi.write(Uint32Array.of(0));

	spi.write(Uint8Array.of(CTRL_RESET));
	spi.flush(true);

	x = ((x - calibrate.left) * calibrate.width / (calibrate.right - calibrate.left)) | 0;
	y = ((y - calibrate.top) * calibrate.height / (calibrate.bottom - calibrate.top)) | 0;

	if (x < 0) x = 0;
	if (x > (calibrate.width - 1)) x = calibrate.width - 1;
	if (y < 0) y = 0;
	if (y > (calibrate.height - 1)) y = calibrate.height - 1;

	trace(x + ", " + y + "\n");
}, 100);
