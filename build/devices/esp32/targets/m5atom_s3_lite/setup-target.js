/*
 * Copyright (c) 2020-2023  Moddable Tech, Inc.
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

import M5Button from "m5button";
import Button from "button";
import NeoPixel from "neopixel";

class Flash {
	constructor(options) {
		return new Button({
			...options,
			pin: 0,
			invert: true
		});
	}
}

globalThis.Host = Object.freeze({
	Button: {
		Default: Flash,
		Flash
	}
}, true);

export default function (done) {
	globalThis.button = {
		a: new M5Button(41)
	};

	Object.defineProperty(globalThis, "lights", {
		enumerable: true,
		configurable: true,
		get() {		// instantiate lights on first access
			const value = new NeoPixel({}); 
			Object.defineProperty(globalThis, "lights", {
				enumerable: true,
				configurable: true,
				writable: true,
				value
			});
			return value;
		}
	});

	done();
}
