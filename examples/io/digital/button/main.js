/*
 * Copyright (c) 2019  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <https://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import Digital from "embedded:io/digital";

let led = new Digital({
   pin: 2,
   mode: Digital.Output,
});
led.write(1);		// off

let button = new Digital({
	pin: 0,
	mode: Digital.InputPullUp,
	edge: Digital.Rising | Digital.Falling,
	onReadable() {
		led.write(this.read());
	}
});
