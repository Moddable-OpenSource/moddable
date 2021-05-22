/*
 * Copyright (c) 2019  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 *
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import {FirmataClientTCP} from "firmataclient";
import Digital from "embedded:io/digital";
import DigitalBank from "embedded:io/digitalbank";

const ServerAddress = "10.0.1.36";

export default function() {
	new FirmataClientTCP({
		address: ServerAddress,
		onReady() {
			const poco = new this.Poco;
			const black = poco.makeColor(0, 0, 0);
			const blue = poco.makeColor(0, 0, 255);
			const green = poco.makeColor(0, 255, 0);
			poco.begin();
				poco.fillRectangle(black, 0, 0, poco.width, poco.height);
			poco.end();

			let localButton = new DigitalBank({
				pins: 0x01,
				bank: 0,
				mode: Digital.Input,
				falls: 0x01,
				onReadable() {
					if (this.read())
						return;

					poco.begin();
						poco.fillRectangle(black, 0, 0, poco.width, poco.height);
					poco.end();
				}
			});

			let i2c = new this.I2C({
				address: 0x38,
				onReadable() {
					let data = new Uint8Array(this.read());
					if (1 === data.length) {				// returned number of touch points
						if (data[0]) {						// non-zero number of touch points
							this.write(Uint8Array.of(3))	// register 3 - touch points
							this.read(6 * data[0]);			// 6 bytes for each touch point
							return;
						}
					}
					else {									// returned touch point data
						for (let offset = 0; offset < data.length; offset += 6) {
							const id = data[offset + 2] >> 4;
							const state = data[offset] >> 6;
							const x = ((data[offset] & 0x0F) << 8) | data[offset + 1];
							const y = ((data[offset + 2] & 0x0F) << 8) | data[offset + 3];
							poco.begin(x - 10, y - 10, 20, 20);
								poco.fillRectangle(id ? blue : green, 0, 0, poco.width, poco.height);
							poco.end();
						}
					}

					this.write(Uint8Array.of(2))		// register 2 - number of touch points
					this.read(1);						// one byte
				}
			});
			i2c.write(Uint8Array.of(2))		// register 2 - number of touch points
			i2c.read(1);					// read one byte
		},
		onReadyx() {
			let poco = new this.Poco;
			let white = poco.makeColor(255, 255, 255);
			let black = poco.makeColor(0, 0, 0);
			let gray = poco.makeColor(128, 128, 128);
			let red = poco.makeColor(255, 0, 0);
			let green = poco.makeColor(0, 255, 0);
			poco.begin();
				poco.fillRectangle(black, 0, 0, poco.width, poco.height);
			poco.end();

//			let led = new this.Digital({
//				pin: 2,
//				mode: this.Digital.Output,
//			});
//
//			let value = 0;
//			System.setInterval(() => {
//				led.write(value);
//				value ^= 1;
//			}, 500);

//			let analog = new this.Analog({
//				pin: 17,
//				onReadable() {
//					const value = this.read();
//					trace(`Analog: ${value}\n`);
//					if (value > 1023)
//						this.close();
//				}
//			});

			let remoteButton = new this.DigitalBank({
				pins: 0x01,
				bank: 0,
				mode: this.Digital.Input,
				onReadable() {
					trace(`Remote Button: ${this.read()}\n`);
					const size = 80;
					poco.begin((poco.width - size) >> 1, (poco.height >> 2) - (size >> 1), size, size);
						poco.fillRectangle(this.read() ? gray : green, 0, 0, poco.width, poco.height);
					poco.end();
				}
			});

			let localButton = new Digital({
				pin: 0,
				mode: Digital.Input,
				edge: Digital.Rising | Digital.Falling,
				onReadable() {
					const size = 80;
					trace(`Local Button: ${this.read()}\n`);
					poco.begin((poco.width - size) >> 1, ((poco.height >> 2) * 3) - (size >> 1), size, size);
						poco.fillRectangle(this.read() ? gray : red, 0, 0, poco.width, poco.height);
					poco.end();
				}
			});

//			let button = new this.Digital({
//				pin: 0,
//				mode: this.Digital.Input,
//				onReadable() {
//					trace(`Button: ${this.read()}\n`);
//				}
//			});
		}
	});
}
