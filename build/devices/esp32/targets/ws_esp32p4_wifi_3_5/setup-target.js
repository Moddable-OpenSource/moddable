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

import config from "mc/config";
import Timer from "timer";

// ES8311 at I2C 0x18, shared with the FT6336 and AXP2101. The bus is closed
// after this init. Register sequence matches Espressif es8311_init() for slave
// Philips I2S, 16-bit, MCLK = 256 * sample rate, which is what audioOut and
// audioIn program. Those dividers are the same for every rate those drivers
// accept. Playback and capture share MCLK, BCLK, and LRCK on I2S0, so open
// only one at a time. The AXP2101 does not gate this codec.
export default function (done) {
	// NS4150B enable, active high. Held for the life of the app.
	globalThis.amplifier = new device.io.Digital({
		pin: device.pin.amplifier,
		mode: device.io.Digital.Output,
		initialValue: 1
	});

	new ES8311();
	done?.();
}

class ES8311 {
	constructor() {
		const es = new device.io.SMBus({
			...device.i2c.default,
			address: 0x18,
			hz: 100_000
		});

		es.writeUint8(0x00, 0x1F);
		Timer.delay(20);
		es.writeUint8(0x00, 0x00);
		es.writeUint8(0x00, 0x80);

		es.writeUint8(0x01, 0x3F);
		es.writeUint8(0x06, es.readUint8(0x06) & ~0x20);
		es.writeUint8(0x02, es.readUint8(0x02) & 0x07);
		es.writeUint8(0x03, 0x10);
		es.writeUint8(0x04, 0x10);
		es.writeUint8(0x05, 0x00);
		es.writeUint8(0x06, (es.readUint8(0x06) & 0xE0) | 0x03);
		es.writeUint8(0x07, es.readUint8(0x07) & 0xC0);
		es.writeUint8(0x08, 0xFF);

		es.writeUint8(0x00, es.readUint8(0x00) & 0xBF);
		es.writeUint8(0x09, 0x0C);
		es.writeUint8(0x0A, 0x0C);

		es.writeUint8(0x0D, 0x01);
		es.writeUint8(0x0E, 0x02);
		es.writeUint8(0x12, 0x00);
		es.writeUint8(0x13, 0x10);
		es.writeUint8(0x1C, 0x6A);
		es.writeUint8(0x37, 0x08);

		es.writeUint8(0x17, 0xC8);
		es.writeUint8(0x14, 0x1A);

		let gain = config.es8311?.micGain ?? 3;
		if (gain < 0)
			gain = 0;
		else if (gain > 7)
			gain = 7;
		es.writeUint8(0x16, gain);

		let volume = config.es8311?.volume ?? 0.7;
		if (volume < 0)
			volume = 0;
		else if (volume > 1)
			volume = 1;
		es.writeUint8(0x32, (volume * 255) | 0);
		es.writeUint8(0x31, es.readUint8(0x31) & ~0x60);

		es.close();
	}
}
