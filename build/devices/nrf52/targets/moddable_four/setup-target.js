/*
 * Copyright (c) 2018-2024  Moddable Tech, Inc.
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

import config from "mc/config";
import Digital from "pins/digital";
import Button from "button";
import LED from "led";
import JogDial from "jogdial";
import LIS3DH from "lis3dh";

globalThis.Host = {
	JogDial: class extends JogDial {
		#onTurn;
		#onPushAndTurn;
		constructor(options) {
			super({
				...options,
				jogdial: config.jogdial,
				onTurn: delta => {
					if (180 === screen?.rotation)
						delta = -delta;
					this.#onTurn?.(delta);
				}, 
				onPushAndTurn: delta => {
					if (180 === screen?.rotation)
						delta = -delta;
					this.#onPushAndTurn?.(delta);
				} 
			});
			this.#onTurn = options.onTurn ?? this.onTurn;
			this.#onPushAndTurn = options.onPushAndTurn ?? this.onPushAndTurn ?? this.#onTurn;
		}
	},
	Accelerometer: class extends LIS3DH {
		constructor(options) {
			super({
				...options,
				address: 0x19,
			});
		}
		sample() {
			const result = super.sample();
			if (180 === screen?.rotation) {
				result.x = -result.x;
				result.y = -result.y;
			}
			return result;
		}
	},
	LED: {
		Default: class {
			constructor(options) {
				return new LED({
					...options,
					pin: config.led1_pin,
				});
			}
		}
	},
	Button: class {
		constructor(options) {
			return new Button({
				...options,
				invert: true,
				pin: config.button1_pin,
				mode: Digital.InputPullUp,
			});
		}
	},
	pins: {
		led: config.led1_pin,
		button: config.button1_pin,
	}
};
Object.freeze(Host, true);

export default function (done) {
	if (config.autobacklight)
		Digital.write(config.lcd_power_pin, 0);
	Digital.write(config.led1_pin, 0);

	done();
}
