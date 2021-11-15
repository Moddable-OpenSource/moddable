/*
 * Copyright (c) 2019-2021  Moddable Tech, Inc.
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

import DigitalBank from "embedded:io/digitalbank";

class Digital extends DigitalBank {
	constructor(options) {
		const pin = options.pin;
		if (undefined === pin)
			throw new Error("invalid");
		const pins = 1 << (pin & 0x1F);
		const edge = options.edge ?? 0;
		const o = {
			pins,
			bank: (pin >> 5) & 0xFF,
			mode: options.mode,
			rises: (edge & Digital.Rising) ? pins : 0,
			falls: (edge & Digital.Falling) ? pins : 0,
			onReadable: options.onReadable,
			format: options.format
		};
		if ("target" in options)
			o.target = options.target;
		super(o);
	}
	read() {
		return super.read() ? 1 : 0;
	}
	write(value) {
		super.write(Number(value) ? ~0 : 0);
	}
}
Digital.Input = DigitalBank.Input;
Digital.InputPullUp = DigitalBank.InputPullUp;
Digital.InputPullDown = DigitalBank.InputPullDown;
Digital.InputPullUpDown = DigitalBank.InputPullUpDown;

Digital.Output = DigitalBank.Output;
Digital.OutputOpenDrain = DigitalBank.OutputOpenDrain;

Digital.Rising = 1;
Digital.Falling = 2;

export default Digital;
