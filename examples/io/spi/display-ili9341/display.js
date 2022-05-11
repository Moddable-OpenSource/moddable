/*
 * Copyright (c) 2022 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 *
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
 
import Timer from "timer";

class Display @ "xs_display_destructor" {
	#spi;
	#dc;
	#state = {
//		reset
//		mac
//		invert
	};

	constructor(options) {
		let {reset, dc, display} = options;
		this.#spi = new (display.io)({
			hz: 40_000_000,
			...display,
			active: 0
		});
		this.#dc = new (dc.io)({
			mode: dc.io.Output,
			...dc
		});
		if (reset) {
			this.#state.reset = reset = new (reset.io)({
				mode: reset.io.Output,
				...reset
			});
			
			reset.write(0);
			Timer.delay(10);
			reset.write(1);
			Timer.delay(1);
		}

		initialize.call(this, this.#spi, this.#dc, this.#state);

		// registers for display in Moddable One and Moddable Two
		// shouldn't be baked into the driver
		// could either be passed with options, or responsibility of caller to call
		this.command(0xCB, 0x39, 0x2C, 0x00, 0x34, 0x02);
		this.command(0xCF, 0x00, 0xC1, 0X30);
		this.command(0xE8, 0x85, 0x00, 0x78);
		this.command(0xEA, 0x00, 0x00);
		this.command(0xED, 0x64, 0x03, 0x12, 0x81);
		this.command(0xF7, 0x20);
		this.command(0xC0, 0x23);
		this.command(0xC1, 0x10);
		this.command(0xC5, 0x3e, 0x28);
		this.command(0xC7, 0x86);
		this.command(0x3A, 0x55);
		this.command(0xB1, 0x00, 0x18);
		this.command(0xB6, 0x08, 0x82, 0x27);
		this.command(0xF2, 0x00);
		this.command(0x26, 0x01);

		this.command(0xE0, 0xD0, 0x08, 0x11, 0x08, 0x0C, 0x15, 0x39, 0x33, 0x50, 0x36, 0x13, 0x14, 0x29, 0x2d);
		this.command(0xE1, 0xD0, 0x08, 0x11, 0x08, 0x06, 0x06, 0x39, 0x44, 0x51, 0x0b, 0x16, 0x14, 0x2f, 0x31);

		this.command(0x36, 0xF0);	// moddable one, moddable two
		this.command(0x21);

		this.command(0x11);
		
		this.configure({width: 240, height: 320});
	}
	close() {
		this.#spi?.close();
		this.#spi = undefined;

		this.#dc?.close();
		this.#dc = undefined;

		this.#state?.reset?.close();
		this.#state = undefined;

		initialize.call(this);
	}
	configure(options) {
		if ("width" in options) {
			if (!("height" in options))
				throw new RangeError;
			configure.call(this, 0, options.width, options.height);
		}
		if ("x" in options) {
			if (!("y" in options))
				throw new RangeError;
			configure.call(this, 3, options.x, options.y);
		}
		if ("format" in options)
			configure.call(this, 1, options.format);
		if ("rotation" in options) {
			const rotation = options.rotation / 90;
			if ((0 !== rotation) && (1 !== rotation) && (2 !== rotation) && (3 !== rotation)) 
				throw new RangeError;

			configure.call(this, 2, rotation);

			this.command(0x36, this.#state.mac ^ [0x00, 0x60, 0xc0, 0xa0][rotation]);
		}
		if ("sleep" in options) {
			if (options.sleep) {
				this.command(0x10);
				Timer.delay(120);
			}
			else {
				this.command(0x11);
				Timer.delay(5);
			}
		}
		if ("invert" in options)
			this.command((0x20 | this.#state.invert) ^ (options.invert ? 1 : 0));
		if ("display" in options)
			this.command(options.display ? 0x29 : 0x28);
		if ("idle" in options)
			this.command(options.idle ? 0x39 : 0x38);
		if ("adaptiveBrightness" in options) {
			const i = ["off", "ui", "still", "moving"].indexOf(options.adaptiveBrightness);
			if (i < 0)
				throw new RangeError;
			this.command(0x55, i);
		}
		if ("adaptiveBrightnessMinimum" in options)
			this.command(0x5e, options.adaptiveBrightnessMinimum);
		if ("async" in options)
			configure.call(this, 4, options.async);
	}
	begin(options) @ "xs_display_begin"
	send(buffer) @ "xs_display_send"
	end() @ "xs_display_end"
	adaptInvalid(options) @ "xs_display_adaptInvalid" 
	get width() @ "xs_display_get_width"
	get height() @ "xs_display_get_height"
	command(command, data) @ "xs_display_command"
	get c_dispatch() {
		return this;		//@@ temporary?!
	}
}

function initialize() @ "xs_display_initialize";
function configure() @ "xs_display_configure";
/*
	0 - width / height
	1 - format
	2 - rotation
	3 - position
	4 - async
*/

export default Display;
