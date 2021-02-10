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

import Expander from "embedded:io/provider/MCP23017";
import I2C from "embedded:io/i2c";
import Digital from "embedded:io/digital";

const expander = new Expander({
	i2c: {
		io: I2C,
		data: 5,
		clock: 4
	},
	interrupt: {
		io: Digital,
		pin: 0
	}
});

const input = new expander.Digital({
	pin: 0,
	mode: expander.DigitalBank.InputPullUp,
	edge: expander.Digital.Rising | expander.Digital.Falling,
	onReadable() {
		trace("Interrupt ", this.read(), "\n");
	}
});

const outputs = new expander.DigitalBank({
	pins: 0x00f0,
	mode: expander.DigitalBank.Output,
});

let state = 0;
System.setInterval(() => {
	outputs.write(state);
	state = ~state
}, 250);
