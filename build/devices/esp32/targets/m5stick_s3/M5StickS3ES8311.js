/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

export default class ES8311 {
	#io;

	constructor(options) {
		const {io: IO, ...dictionary} = options;
		this.#io = new IO({
			...dictionary,
			address: 0x18,
			hz: 100_000
		});
	}

	close() {
		this.#io?.close();
		this.#io = undefined;
	}

	configureInput() {
		this.#configure(true);
	}

	configureOutput() {
		this.#configure(false);
	}

	#configure(input) {
		const codec = this.#io;
		// Reset retained settings, including mute and the serial port format.
		codec.writeUint8(0x00, 0x1F);
		Timer.delay(20);
		codec.writeUint8(0x00, 0x00);
		codec.writeUint8(0x00, 0x80);

		// 16-bit stereo I2S slots give BCLK = 32 * sampleRate.
		// Derive the codec's 256 * sampleRate clock from BCLK with an 8x multiplier.
		const registers = [
			[0x01, 0xBF], // BCLK source, enable clocks
			[0x02, 0x18], // pre-divider 1, multiplier 8
			[0x03, 0x10], // ADC oversampling
			[0x04, 0x10], // DAC oversampling
			[0x05, 0x00], // ADC/DAC divider 1
			[0x06, 0x07], // BCLK divider 8
			[0x07, 0x00], // LRCLK divider high bits
			[0x08, 0xFF], // LRCLK divider 256
			[0x09, 0x0C], // slave I2S, 16-bit DAC, left slot
			[0x0A, 0x0C], // slave I2S, 16-bit ADC, left slot
			[0x0D, 0x01], // analog power
			[0x0E, input ? 0x02 : 0x6A], // ADC/PGA power
			[0x12, input ? 0x02 : 0x00], // DAC power
			[0x13, 0x10], // headphone output
			[0x14, 0x17], // analog microphone, PGA +21 dB
			[0x17, 0xCB], // ADC digital volume +6 dB
			[0x1C, 0x6A], // ADC EQ bypass and DC cancellation
			[0x32, 0xB3], // DAC -6 dB
			[0x37, 0x08]  // DAC EQ bypass
		];
		for (const [register, value] of registers)
			codec.writeUint8(register, value);

		if (input) {
			// AudioIn starts I2S on another task. Re-arm the analog capture path
			// after clocks have settled; register readback alone cannot prove this.
			for (let i = 0; i < 3; i++) {
				Timer.delay(30);
				codec.writeUint8(0x0D, 0x01);
			}
		}
	}

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}
