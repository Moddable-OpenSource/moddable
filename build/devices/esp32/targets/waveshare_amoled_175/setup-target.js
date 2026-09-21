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
import AudioOut from "embedded:io/audio/out";

export default function (done) {
	globalThis.power = new device.peripheral.Power.PMIC();
	globalThis.backlight = device.peripheral.Backlight;

	if (device.peripheral.Power.Amplifier)
		new device.peripheral.Power.Amplifier();

	try {
		const clocks = new AudioOut({
			onWritable() {}
		});
		clocks.start();
		Timer.delay(20);
		new ES8311();
		clocks.stop();
		clocks.close();
	}
	catch (e) {
		trace(`ES8311: ${e}\n`);
	}

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
		es.writeUint8(0x02, 0x00);
		es.writeUint8(0x03, 0x20);
		es.writeUint8(0x04, 0x20);
		es.writeUint8(0x05, 0x00);
		es.readUint8(0x06);
		es.writeUint8(0x06, 0x07);
		es.readUint8(0x07);
		es.writeUint8(0x07, 0x00);
		es.writeUint8(0x08, 0xFF);
		es.writeUint8(0x00, es.readUint8(0x00) & 0xBF);
		es.writeUint8(0x09, 0x0C);
		es.writeUint8(0x0A, 0x0C);
		es.writeUint8(0x0D, 0x01);
		es.writeUint8(0x0E, 0x02);
		es.writeUint8(0x12, 0x00);
		es.writeUint8(0x13, 0x10);
		es.writeUint8(0x14, 0x1A);
		es.writeUint8(0x1C, 0x6A);
		es.writeUint8(0x37, 0x08);
		es.writeUint8(0x32, 0xD0);
		es.close();
	}
}
