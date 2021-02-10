/*
 * Copyright (c) 2019  Moddable Tech, Inc.
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

/*
	An implementation of a Firmata host using the proposed TC53 IO JavaScript APIs.

		Implements Serial, TCP client (compatible with Etherport), and TCP server transports

	All firmata transport implemented using TC53 IO.

	All pin operations using TC53 IO: Digital, Analog, Serial, I2C.

	Other pin operations unimplemented.

	Tested against Firmata.js client.

	This is for ESP8266 device in general and Moddable One in particular.

	Depeendencies on the target devices are mostly isolated:

		this.Pins contains the pin map
		I2C pins are reserevd from the I2CBus object - could freed up if I2C unused
		If screen global present, assumes usual Moddable SPI display configuration (often, but not always, correct)
		If Firmata runs over serial, the serial TX & RX pins are reserved

	Pin 16 GPIO is not fully implemented - reportDigital won't work

	Implements basic Poco graphics (mostly rectanngle) as an extension to Firmata through User Command 1
*/

import Analog from "embedded:io/analog";
import DigitalBank from "embedded:io/digitalbank";
import I2C from "embedded:io/i2c";
import Listener from "embedded:io/socket/listener";
import Serial from "embedded:io/serial";
import TCP from "embedded:io/socket/tcp";
import Poco from "commodetto/Poco";

const I2CBus = Object.freeze({
	sda: 5,
	scl: 4,
	hz: 600000
});

const FIRMATA_PROTOCOL_MAJOR_VERSION = 2;
const FIRMATA_PROTOCOL_MINOR_VERSION = 6;

const FIRMATA_FIRMWARE_MAJOR_VERSION = 2
const FIRMATA_FIRMWARE_MINOR_VERSION = 10;

const fDIGITAL_INPUT =      (0x00)
const fDIGITAL_OUTPUT =     (0x01)
const fANALOG_INPUT =       (0x02)
const fPWM =                (0x03)
const fSERVO =              (0x04)
const fSHIFT =              (0x05)
const fI2C =                (0x06)
const fONEWIRE =            (0x07)
const fSTEPPER =            (0x08)
const fENCODER =            (0x09)
const fSERIAL =             (0x0A)
const fINPUT_PULLUP =       (0x0B)

const DigitalModes = Object.freeze([fDIGITAL_INPUT, fINPUT_PULLUP, fDIGITAL_OUTPUT]);
const I2Mode = Object.freeze([fI2C]);
const AnalogMode = Object.freeze([fANALOG_INPUT]);
const SerialMode = Object.freeze([fSERIAL]);
const ReservedMode = Object.freeze([]);

export class Firmata {
	constructor(dictionary) {
		this.onBuildPins();

		this.buffer = new Uint8Array(0);

		// this.ports could be done at build time...
		this.ports = new Array(Math.ceil(this.pins.length / 8));
		for (let port = 0; port < this.ports.length; port++) {
			let pins = 0, inputs = 0;
			for (let i = 0; i < 8; i++) {
				const pin = this.pins[(port << 3) + i];
				if (!pin || !pin.modes.length || (pin.modes.indexOf(fI2C) >= 0) || (pin.modes.indexOf(fSERIAL) >= 0)) continue;
				pins |= 1 << i;
				if ((pin.modes.indexOf(fDIGITAL_INPUT) >= 0) || (pin.modes.indexOf(fINPUT_PULLUP) >= 0))
					inputs |= 1 << i;
			}
			pins <<= port << 3;
			inputs <<= port << 3;
			this.ports[port] = {
				shift: port << 3,
				pins,
				inputs,
				outputs: 0,
				// report: false
			};

			this.rebuildPort(this.ports[port]);
		}
	}
	read(count) {
		return this.transport.read(count);
	}
	write(value) {
		this.transport.write(Uint8Array.of(value).buffer);
	}
	onReadable(count) {
		let length = this.buffer.length;
		let buffer = new Uint8Array(length + count);
		buffer.set(this.buffer);
		buffer.set(new Uint8Array(this.read(count)), length);
		let position = this.onReceive(buffer, 0);
		this.buffer = buffer.slice(position);
	}
	onReceive(bytes, position) {
		while (position < bytes.length) {
			const available = bytes.length - position;

			let command = bytes[position], channel;
			if (command < 0xF0) {
				channel = command & 0x0F;
				command &= 0xF0;
			}

			switch (command) {
				case 0x90:		// DIGITAL_MESSAGE
					if (available < 3)
						return position;

					this.doDigitalWrite(channel, (bytes[position + 1] & 0x7F) | ((bytes[position + 2] & 1) << 7));		//@@ masking may be unnecesary
					position += 3;
					break;

				case 0xC0:		// REPORT_ANALOG
					if (available < 2)
						return position;

					this.doReportAnalog(channel, bytes[position + 1]);
					position += 2;
					break;

				case 0xD0:		// REPORT_DIGITAL
					if (available < 2)
						return position;

					this.doReportDigital(channel, bytes[position + 1]);
					position += 2;
					break;

				case 0xF0: {	// SYSEX begin
					let end = position, found;
					while ((++end < bytes.length) && !found)
						found = (0xF7 === bytes[end])
					if (!found)
						return position;
					this.onSysex(bytes.slice(position + 1, end - 1));
					position = end;
					} break;

				case 0xF4:		// PIN_MODE
					if (available < 3)
						return position;
					this.doSetPinMode(bytes[position + 1], bytes[position + 2])
					position += 3;
					break;

				case 0xF9:		// REPORT_VERSION
					this.write(0xF9);
					this.write(FIRMATA_PROTOCOL_MAJOR_VERSION);
					this.write(FIRMATA_PROTOCOL_MINOR_VERSION);
					position += 1;
					break;

				case 0xFF:		// SYSTEM_RESET
					System.restart();
					break;
				default:
					position += 1;
					break;
			}
		}

		return position;
	}
	onSysex(bytes) {
		switch (bytes[0]) {
			case 0x01: {// POCO_COMMAND (user command)
				let offset = 2, r, g, b;
				switch (bytes[1]) {
					case 0x01:								// POCO get dimensions
						this.write(0xF0);					// sysex begin
						this.write(0x01);					// POCO_COMMAND (user command)
						this.write(0x01);					// POCO get dimensions
						this.write14Bits(screen.width);
						this.write14Bits(screen.height);
						this.write(0xF7);					// sysex end
						break;

					case 0x02:								// POCO begin
						if (!this.poco)
							this.poco = new Poco(screen);

						this.poco.begin(
							bytes[offset + 0] | (bytes[offset + 1] << 7),
							bytes[offset + 2] | (bytes[offset + 3] << 7),
							bytes[offset + 4] | (bytes[offset + 5] << 7),
							bytes[offset + 6] | (bytes[offset + 7] << 7)
						);
						break;

					case 0x03:								// POCO end
						this.poco.end();
						break;

					case 0x06:								// POCO fillRectangle
						r = bytes[offset + 0];
						g = bytes[offset + 1];
						b = bytes[offset + 2];
						offset += 3;
						this.poco.fillRectangle(
							this.poco.makeColor(r << 1, g << 1, b << 1),		//@@ one bit precision error
							bytes[offset + 0] | (bytes[offset + 1] << 7),
							bytes[offset + 2] | (bytes[offset + 3] << 7),
							bytes[offset + 4] | (bytes[offset + 5] << 7),
							bytes[offset + 6] | (bytes[offset + 7] << 7)
						);
						break;

					case 0x08:								// POCO drawPixel
						r = bytes[offset + 0];
						g = bytes[offset + 1];
						b = bytes[offset + 2];
						offset += 3;
						this.poco.drawPixel(
							this.poco.makeColor(r << 1, g << 1, b << 1),		//@@ one bit precision error
							bytes[offset + 0] | (bytes[offset + 1] << 7),
							bytes[offset + 2] | (bytes[offset + 3] << 7),
						);
						break;

					default:
						trace(`unhandled poco command ${bytes[1]}\n`);
						break;
				}
				} break;

			case 0x60: {	// SERIAL_DATA
				const firmata = this;
				const port = bytes[1] & 0x0F;		//@@ validate port
				switch (bytes[1] >> 4) {
					case 1:	// SERIAL_CONFIG
						if (this.serial)
							this.serial.close();
						delete this.serial;

						this.serial = new Serial({
							baud: bytes[2] | (bytes[3] << 7) | (bytes[4] << 14),
							onReadable(count) {
								if (!this.reading) {
									while (count--)
										this.read();
									return;
								}

								firmata.write(0xF0);		// sysex begin
								firmata.write(0x60);		// SERIAL_DATA
								firmata.write(0x40);		// SERIAL_REPLY (@@ port)
								while (count--) {
									const value = this.read();
									firmata.write(value & 0x7F);
									firmata.write(value >> 7);
								}
								firmata.write(0xF7);		// sysex end
							},
						});
						break;

					case 2:	// SERIAL_WRITE
						for (let i = 2, count = (bytes.byteLength - 2) >> 1; count; i += 2, count--)
							this.serial.write(bytes[i] | bytes[i + 1]);
						break;

					case 3:	// SERIAL_READ
						this.serial.reading = 0 === bytes[2];
						break;

					case 5:		// SERIAL_CLOSE
						if (this.serial)
							this.serial.close();
						delete this.serial;
						break;
				}
				} break;

			case 0x69:	// ANALOG_MAPPING_QUERY
				this.write(0xF0);		// sysex begin
				this.write(0x6A);		// ANALOG_MAPPING_RESPONSE

				for (let i = 0; i < this.pins.length; i++) {
					const pin = this.pins[i];
					if (undefined !== pin.analogChannel)
						this.write(pin.analogChannel);
					else
						this.write(0x7F);
				}

				this.write(0xF7);		// sysex end
				break;

			case 0x6B:	// CAPABILITY_QUERY
				this.write(0xF0);		// sysex begin
				this.write(0x6C);		// CAPABILITY_RESPONSE

				for (let i = 0; i < this.pins.length; i++) {
					const modes = this.pins[i].modes;
					if (modes) {
						for (let j = 0; j < modes.length; j++) {
							const mode = modes[j];
							this.write(mode);
							if ((fDIGITAL_INPUT === mode) || (fINPUT_PULLUP === mode) || (fDIGITAL_OUTPUT === mode))
								this.write(1);
							else if (fANALOG_INPUT === mode)
								this.write(10);
							else
								this.write(0);		//@@ configuration error
						}
					}
					this.write(0x7F);
				}

				this.write(0xF7);		// sysex end

				if (global.screen)
					this.doSendString("hasPoco");
				break;

/*			case 0x6D:	// PIN_STATE_QUERY
				//@@ 1 byte payload - pin number
				// PIN_STATE_RESPONSE - 0x6E
				break;
*/
			case 0x76: {				// I2C_REQUEST
				let offset = 1;
				let address = bytes[offset++];
				const mode = (bytes[offset] >> 3) & 0x3;
				if (0x20 & bytes[offset])
					;	//@@ 10 bit address mode
				const stop = 0 != (bytes[offset++] & 0x40);
				switch (mode) {
					case 0: {			// write
						let payload = new Uint8Array((bytes.length - offset) >> 1);
						for (let i = 0; i <= payload.length; i++)
							payload[i] = (bytes[offset + (i << 1)] & 0x7f) |
											((bytes[offset + (i << 1) + 1] & 0x7f) << 7);
							this.onI2CWrite(address, payload, stop);
						} break;
					case 1:				// read
					case 2: {			// read continuous
						let register;
						if (bytes.length > 5) {
							register = bytes[offset++] & 0x7f;
							register |= (bytes[offset++] & 0x7f) << 7;

						}
						let byteLength = bytes[offset++] & 0x7f;
						byteLength |= (bytes[offset++] & 0x7f) << 7;
						if (1 === mode)
							this.onI2CRead(address, register, byteLength, stop);
						else {
							if (!this.i2cReport)
								this.i2cReport = [];
							this.i2cReport.push({address, register, byteLength, stop});
							this.rebuildPoll();
						}
						} break;
					case 3:				// stop
						if (this.i2cReport) {
							for (let i2cReport = this.i2cReport, length = i2cReport.length, i = 0; i < length; i++) {
								if (i2cReport[i].address !== address)
									continue;
								this.i2cReport.splice(i, 1);
								length -= 1;
								i -= 1;
							}
						}
						break;
				}
				} break;

			case 0x78:					// I2C_CONFIG
				this.i2cDelay = bytes[1] | (bytes[2] << 8);
				break;

			case 0x79:					// firmware name/version
				this.write(0xF0);		// sysex begin
				this.write(0x79);		// report firmware
				this.write(FIRMATA_FIRMWARE_MAJOR_VERSION);
				this.write(FIRMATA_FIRMWARE_MINOR_VERSION);
				this.writeString("moddable");
				this.write(0xF7);		// sysex end
				break;

			case 0x7A:					// SAMPLING_INTERVAL
				this.interval = bytes[1] | (bytes[2] << 7);
				if (this.timer)
					System.clearInterval(this.timer);
				delete this.timer;
				this.rebuildPoll();
				break;
		}
	}
	write14Bits(value) {
		this.write(value & 0x7F);
		this.write((value >> 7) & 0x7F);
	}
	writeString(value) {
		for (let i = 0; i < value.length; i++)
			this.write14Bits(value.charCodeAt(i));
	}

	rebuildPort(port) {
		if (port.inBank) {
			port.inBank.close();
			delete port.inBank;
		}
		if (port.outBank) {
			port.outBank.close();
			delete port.outBank;
		}
		if (port.inputs) {
			//@@ ignoring PULLUP
			const dictionary = {
				pins: port.inputs,
				mode: DigitalBank.Input,
			};
			if (port.report) {
				dictionary.rises = port.inputs;
				dictionary.falls = port.inputs;
				dictionary.onReadable = () => this.onDigitalChanged(port);
			}

			port.inBank = new DigitalBank(dictionary);
		}
		if (port.outputs) {
			port.outBank = new DigitalBank({
				pins: port.outputs,
				mode: DigitalBank.Output,
			});
		}
	}
	rebuildPoll() {
		if (this.analogReport || this.i2cReport) {
			if (!this.timer)
				this.timer = System.setInterval(this.poll.bind(this), this.interval || 19);
		}
		else {
			if (this.timer)
				System.clearInterval(this.timer);
			delete this.timer;
		}
	}
	poll() {
		if (this.analogReport) {
			for (let analog in this.analogReport)
				this.onAnalogChanged(analog);
		}
		if (this.i2cReport) {
			for (let i2cReport = this.i2cReport, length = i2cReport.length, i = 0; i < length; i++) {
				const item = i2cReport[i];
				this.onI2CRead(item.address, item.register, item.byteLength, item.stop);
			}
		}
	}
	doSetPinMode(pin, mode) {
		const port = this.ports[pin >> 3];
		const mask = 1 << ((pin & 7) + port.shift);
		if (fDIGITAL_INPUT === mode) {
			port.inputs |= mask;
			port.outputs &= ~mask;
		}
		else if (fDIGITAL_OUTPUT === mode) {
			port.inputs &= ~mask;
			port.outputs |= mask;
		}
		this.rebuildPort(port);
	}
	doDigitalWrite(port, value) {
		port = this.ports[port];
		port.outBank.write(value << port.shift);
	}
	doReportAnalog(analog, enable) {
		if (enable) {
			if (!this.analogReport)
				this.analogReport = [];
			if (!this.analogReport[analog])
				this.analogReport[analog] = new Analog({pin: analog});		//@@ map channel to pin!
		}
		else if (this.analogReport && this.analogReport[analog]) {
			this.analogReport[analog].close();
			delete this.analogReport[analog];
			if (!this.analogReport.find(item => undefined !== item))
				delete this.analogReport;
		}
		this.rebuildPoll();
	}
	doReportDigital(port, enable) {
		port = this.ports[port];
		if (enable)
			port.report = true;
		else
			delete port.report;
		this.rebuildPort(port);
		if (enable)
			this.onDigitalChanged(port);
	}
	doSendString(msg) {
		this.write(0xF0);		// sysex begin
		this.write(0x71);		// STRING_DATA
		this.writeString(msg);
		this.write(0xF7);		// sysex end
	}
	onDigitalChanged(port) {
		const value = (port.inBank.read() >> port.shift) & 0xFF;
		this.write(0x90 | (port.shift >> 3));		// DIGITAL_MESSAGE | port number
		this.write14Bits(value);
	}
	onAnalogChanged(channel) {
		this.write(0xE0 | channel);		// ANALOG_MESSAGE
		this.write14Bits(this.analogReport[channel].read());
	}

	getI2C(address) {
		if (!this.i2c)
			this.i2c = [];
		let i2c = this.i2c[address];
		if (!i2c) {
			i2c = new I2C({...I2CBus, address});
			this.i2c[address] = i2c;
		}
		return i2c;
	}

	onI2CWrite(address, payload, stop) {
		(this.getI2C(address)).write(payload, stop);
	}
	onI2CRead(address, register, byteLength, stop) {
		const i2c = this.getI2C(address);
		if (undefined !== register)
			i2c.write(Uint8Array.of(register));
		const bytes = new Uint8Array(i2c.read(byteLength, stop));
		this.write(0xF0);		// sysex begin
		this.write(0x77);		// I2C_REPLY
		this.write14Bits(address)
		this.write14Bits(register || 0)
		for (let i = 0; i < bytes.length; i++)
			this.write14Bits(bytes[i]);
		this.write(0xF7);		// sysex end
	}
	onBuildPins() {
		const screen = global.screen ? ReservedMode : DigitalModes;	// assume screen owns SPI pins

		this.pins = new Array(16);
		this.pins.fill({modes: ReservedMode});

		this.pins[ 0] = {modes: DigitalModes},		// 0
		this.pins[ 1] = {modes: DigitalModes},		// 1 (serial TX)
		this.pins[ 2] = {modes: screen},			// 2
		this.pins[ 3] = {modes: DigitalModes},		// 3 (serial RX)
		this.pins[ 4] = {modes: DigitalModes},		// 4
		this.pins[ 5] = {modes: DigitalModes},		// 5
		this.pins[12] = {modes: screen},			// 12
		this.pins[13] = {modes: screen},			// 13
		this.pins[14] = {modes: screen},			// 14
		this.pins[15] = {modes: screen},			// 15
		this.pins[16] = {modes: DigitalModes},		// 16		//@@ special
		this.pins[17] = {modes: AnalogMode, analogChannel: 0},		// 17

		this.pins[I2CBus.sda] =
		this.pins[I2CBus.scl] = {modes: I2Mode};
	}
}

export class FirmataSerial extends Firmata {
	constructor(dictionary = {}) {
		super(dictionary);

		this.transport = new Serial({
			baud: dictionary.baud || 57600,
			onReadable: count => this.onReadable(count),
		});
		this.transport.format = "buffer";
	}
	onBuildPins() {
		super.onBuildPins();

		this.pins[1] =
		this.pins[3] = {modes: SerialMode};
	}
}

export class FirmataTCPClient extends Firmata {
	constructor(dictionary = {}) {
		super(dictionary);

		this.first = true;
		this.transport = new TCP({
			address: dictionary.address,
			port: dictionary.port || 3030,
			nodelay: true,
			onReadable: count => this.onReadable(count),
			onWritable: () => {
				if (!this.first)
					return;
				delete this.first;
				this.onSysex(Uint8Array.of(0x79));	// report firmware name/version
			}
		});
	}
}

export class FirmataTCPServer extends Firmata {
	constructor(dictionary = {}) {
		super(dictionary);

		this.transport = undefined;
		this.listener = new Listener({
			port: dictionary.port || 3030,
			nodelay: true,
			onReadable: count => {
				while (count--) {
					if (this.transport) {
						trace("Firmata: busy, refuse TCP connection\n");
						this.listener.read().close();
						continue;
					}
					this.transport = new TCP({
						from: this.listener.read(),
						onReadable: count => this.onReadable(count),
						onError: () => {
							trace("Firmata: connection dropped - restart\n");
							System.restart();
						}
					});
					trace("Firmata: accept TCP connection\n");
					this.onSysex(Uint8Array.of(0x79));	// report of firmware name/version
				}
			},
			onError: () => {
				this.transport.close();
				delete this.transport;
			},
		});
	}
}

export default Object.freeze({
	Firmata,
	FirmataSerial,
	FirmataTCPClient,
	FirmataTCPServer,
});
