/*
 * Copyright (c) 2022-2026  Moddable Tech, Inc.
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
import AudioOut from "embedded:io/audio/out";
import Resource from "Resource";

export default function (done) {
	new ES8311();
	new PI4IOE5V6408();

	if (config.startupSound) {
		Timer.delay(1000);
		const buf = new Resource(config.startupSound);
		let playing = new Uint8Array(buf, 0, buf.byteLength);
		playing.position = 0;
		const speaker = new AudioOut({
			onWritable(size) {
				do {
					let use = playing.byteLength - playing.position;
					if (use) {
						if (use > size) use = size;
						this.write(playing.subarray(playing.position, playing.position + use));
						playing.position += use;
					}
					if (playing.position === playing.byteLength) {
						this.stop();
						this.close();
						Timer.set(this.done);
						break;
					}
					size -= use;
				} while (size);
			}
		});
		speaker.done = done;
		done = undefined;
		speaker.start();
	}

	done?.();
}

class ES8311 {
	constructor() {
		const es = new device.io.SMBus({
			...device.i2c.internal,
			address: 0x18,
			hz: 100_000,
		});
		es.writeUint8(0x00, 0x1F);
		Timer.delay(20);
		es.writeUint8(0x00, 0x00);
		es.writeUint8(0x00, 0x80);

		es.writeUint8(0x01, 0xBF);
		es.readUint8(0x06);
		es.writeUint8(0x06, 0x03);
		es.readUint8(0x02);
		es.writeUint8(0x02, 0x10);
		es.writeUint8(0x03, 0x10);
		es.writeUint8(0x04, 0x10);
		es.writeUint8(0x05, 0x00);
		es.readUint8(0x06);
		es.writeUint8(0x06, 0x03);
		es.readUint8(0x07);
		es.writeUint8(0x07, 0x00);
		es.writeUint8(0x08, 0xFF);

		es.readUint8(0x00);
		es.writeUint8(0x00, 0x80);
		es.writeUint8(0x09, 0x10);
		es.writeUint8(0x0A, 0x10);

		es.writeUint8(0x0D, 0x01);
		es.writeUint8(0x0E, 0x02);
		es.writeUint8(0x12, 0x00);
		es.writeUint8(0x13, 0x10);
		es.writeUint8(0x1C, 0x6A);
		es.writeUint8(0x37, 0x08);

		// config volume is 0.0–1.0 (ECMA-419); ES8311 DAC reg is 0–255
		let volume = config.es8311?.volume ?? 0.5;
		if (volume < 0)
			volume = 0;
		else if (volume > 1)
			volume = 1;
		es.writeUint8(0x32, (volume * 255) | 0);

		es.writeUint8(0x17, 0xFF);
		es.writeUint8(0x14, 0x1A);
		es.writeUint8(0x16, 0x01);

		es.close();
	}
}

class PI4IOE5V6408 {
	constructor() {
		const pi = new device.io.SMBus({
			...device.i2c.internal,
			address: 0x43,
			hz: 100_000,
		});
		pi.readUint8(0x00);
		pi.writeUint8(0x07, 0x00);
		pi.readUint8(0x07);
		pi.writeUint8(0x0D, 0xFF);
		pi.writeUint8(0x03, 0x6E);
		pi.readUint8(0x03);
		pi.writeUint8(0x05, 0xFF);
		pi.readUint8(0x05);
		pi.close();
	}
}
