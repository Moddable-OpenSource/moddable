/*
 * Copyright (c) 2019-2024  Moddable Tech, Inc.
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

if (undefined === device?.pin?.button)
	throw new Error("no button pin provided by device");

const Digital = device.io.Digital;
let led;

if (undefined !== device.pin.led) {
	led = new Digital({
	   pin: device.pin.led,
	   mode: Digital.Output,
	});
	led.write(1);
}

new Digital({
	pin: device.pin.button,
	mode: Digital.InputPullUp,
	edge: Digital.Rising | Digital.Falling,
	onReadable() {
		led?.write(this.read());
		trace(`Button: ${this.read()}\n`);
	}
});
