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

import Serial from "embedded:io/serial";
import TCP from "embedded:io/socket/tcp";

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

const PIN_USED =			1 << 0;
const PIN_REPORT =			1 << 1;

export class FirmataClient {
	#onReady
	constructor(dictionary) {
		this.waiters = [];
		this.digitalReaders = [];
		this.i2cReaders = [];
		this.buffer = new Uint8Array(0);
		if (dictionary.interval)
			this.interval = dictionary.interval;
		if (dictionary.onReady)
			this.#onReady = dictionary.onReady;
	}
	async onConnect() {
		let version = await this.doRequestReportVersion();
		if (FIRMATA_PROTOCOL_MAJOR_VERSION !== version.major)
			throw new Error("version mismatch");

		this.pins = await this.doRequestCapabilityQuery();

		const hasAnalog = this.pins.find(pin => pin.modes & (1 << fANALOG_INPUT));
		if (hasAnalog)
			this.analogMap = await this.doRequestAnalogMapping();

		if (this.interval)
			this.doSetSamplingInterval(this.interval);
		delete this.interval;

		const firmata = this;
		this.ports = new Uint8Array((this.pins.length + 7) >> 3);
		if (this.pins.find(pin => pin.modes & ((1 << fDIGITAL_INPUT) | (1 << fINPUT_PULLUP)) | (1 << fDIGITAL_OUTPUT))) {
			this.Digital = class extends Digital {
				constructor(dictionary) {
					super(dictionary, firmata, true);
				}
			}
			this.DigitalBank = class extends Digital {
				constructor(dictionary) {
					super(dictionary, firmata);
				}
			}
		}
		if (hasAnalog) {
			this.Analog = class extends Analog {
				constructor(dictionary) {
					super(dictionary, firmata);
				}
			}
		}
		if (this.pins.find(pin => pin.modes & (1 << fI2C))) {
			this.I2C = class extends I2C {
				constructor(dictionary) {
					super(dictionary, firmata);
				}
			}
		}

		if (this.Poco) {
			const dimensions = await this.doPocoGetDimensions();
			delete dimensions.command;
			this.Poco = class extends Poco {
				constructor(dictionary) {
					super(dictionary, firmata);
					this.width = dimensions.width;
					this.height = dimensions.height;
				}
			}
		}

		this.#onReady();
	}
	read() {
		return this.transport.read();
	}
	write(value) {
		this.transport.write(value);
	}
	onReadable(count) {
		let length = this.buffer.length;
		let buffer = new Uint8Array(length + count);
		buffer.set(this.buffer);
		for (let i = 0; i < count; i++)
			buffer[length + i] = this.read();
		let position = this.onReceive(buffer, 0);
		this.buffer = buffer.slice(position);
	}
	onReceive(bytes, position) {
		while (position < bytes.length) {
			const available = bytes.length - position;

			let command = bytes[position], channel;
			if (command < 0xF0) {
				command &= 0xF0;
				channel = command & 0x0F;
			}

			switch (command) {
				case 0x90:		// DIGITAL_MESSAGE
				case 0xE0:		// ANALOG_MESSAGE
					if (available < 3)
						return position;

					this.messageReceived(command, {
						channel,
						value: bytes[position + 1] | ((bytes[position + 2] & 0x7f) << 7)
					});
					position += 3;
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

				case 0xF9:		// REPORT_VERSION
					if (available < 3)
						return position;

					this.messageReceived(command, {
						major: bytes[position + 1],
						minor: bytes[position + 2],
					});
					position += 2;
					break;

				default:
					position += 1;
					break;
			}
		}

		return position;
	}
	onSysex(bytes) {
		const message = bytes[0];
		switch (message) {
			case 0x01:		// POCO_COMMAND
				switch (bytes[1]) {
					case 1:	// POCO get dimensions
						this.messageReceived(message, {
							command: bytes[1],
							width: bytes[2] | (bytes[3] << 7),
							height: bytes[4] | (bytes[5] << 7),
						})
				}
				break;

			case 0x6A:		// ANALOG_MAPPING_RESPONSE
				this.messageReceived(message, bytes.slice(1));
				break;

			case 0x6C: {	// CAPABILITY_RESPONSE
				const pins = [];
				let pin;
				for (let position = 1; position < bytes.length; position++) {
					let byte = bytes[position];
					if (!pin)
						pin = {modes: 0}

					if (0x7F == byte) {
						pin.flags = pin.modes ? 0 : PIN_USED;
						pins.push(pin);
						pin = undefined;
					}
					else {
						pin.modes |= 1 << byte;
						position += 1;		// skip resolution
					}
				}
				this.messageReceived(message, pins);
			} break;

			case 0x71: {	// STRING_DATA
				let string = ""
				for (let i = 1; i < bytes.length; i += 2)
					string += String.fromCharCode(bytes[i] | (bytes[i + 1] << 7));
				this.messageReceived(message, string);
				}
				break;

			case 0x77: {	// I2C_REPLY
				const address = bytes[1] | (bytes[2] << 7);
				const register = bytes[3] | (bytes[4] << 7);
				const payload = new Uint8Array((bytes.length - 5) >> 1);
				for (let offset = 5, i = 0; i < payload.length; i++, offset += 2)
					payload[i] = bytes[offset] | (bytes[offset + 1] << 7);

				for (let i = 0, i2cReaders = this.i2cReaders, length = i2cReaders.length; i < length; i++) {
					if (address !== i2cReaders[i].address)
						continue;
					i2cReaders[i].received.push(payload.buffer);
					if (i2cReaders[i].onReadable)
						i2cReaders[i].onReadable.call(i2cReaders[i].target);
					break;
				}
				} break;
		}
	}
	messageWait(message) {
		const waiter = {message};
	 	waiter.promise = new Promise((resolve, reject) => {
	 		waiter.resolve = resolve;
	 		waiter.reject = reject;
		});
		this.waiters.push(waiter);
		return waiter.promise;
	}
	messageReceived(message, value) {
		const index = this.waiters.findIndex(waiter => message === waiter.message);
		if (index >= 0) {
			const waiter = this.waiters[index];
			this.waiters.splice(index, 1);
			waiter.resolve(value);
			return;
		}

		switch (message) {
			case 0x90: {	// DIGITAL_MESSAGE
				let changed = this.ports[value.channel] ^ value.value;
				if (!changed) return;

				this.ports[value.channel] = value.value;
				for (let i = 0, digitalReaders = this.digitalReaders, length = digitalReaders.length; i < length; i++) {
					const reader = digitalReaders[i];
					if (reader.pins & (changed << (value.channel << 3)))
						reader.onReadable.call(reader.target);
				}
				} break;

			case 0x71:		// STRING_DATA
				if (("hasPoco" === value) && (undefined === this.Poco))
					this.Poco = true;
				break;

			case 0xE0: {	// ANALOG_MESSAGE
				let pin = this.analogMap.indexOf(value.channel);
				pin = this.pins[pin];
				pin.value = value.value;
				if (pin.onReadable)
					pin.onReadable.call(pin.target);
				} break;

			default:
				trace(`unhandled message 0x${message.toString(16)}\n`);
				trace("  ", JSON.stringify(value), "\n");
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

	usePins(pins, use) {
		for (let pin = 0; pins; pin++, pins >>= 1) {
			if (pins & 1)
				this.usePin(pin, use);
		}
	}
	usePin(pin, use) {
		pin = this.pins[pin];
		if (use) {
			if (pin.flags & PIN_USED)
				throw new Error("in use");
			pin.flags |= PIN_USED;
		}
		else
			pin.flags &= ~(PIN_USED | PIN_REPORT);
	}
	rebuildDigitalReports() {
		for (let port = 0; port < this.ports.length; port++) {
			let report;

			for (let i = 0, pin = port << 3; (i < 8) && (pin < this.pins.length); i++, pin++) {
				if (PIN_REPORT & this.pins[pin].flags) {
					report = true;
					break;
				}
			}
			this.doReportDigital(port, report);
		}
	}

	doRequestReportVersion() {
		this.write(0xF9);				// REPORT_VERSION
		return this.messageWait(0xF9);
	}
	doRequestCapabilityQuery() {
		this.write(0xF0);				// SYSEX BEGIN
		this.write(0x6B);				// CAPABILITY_QUERY
		this.write(0xF7);				// SYSEX END
		return this.messageWait(0x6C);	// CAPABILITY_RESPONSE
	}
	doRequestAnalogMapping() {
		this.write(0xF0);				// SYSEX BEGIN
		this.write(0x69);				// ANALOG_MAPPING_QUERY
		this.write(0xF7);				// SYSEX END
		return this.messageWait(0x6A);	// ANALOG_MAPPING_RESPONSE
	}
	doSetSamplingInterval(interval) {
		this.write(0xF0);				// SYSEX BEGIN
		this.write(0x7A);				// SAMPLING_INTERVAL
		this.write14Bits(interval);
		this.write(0xF7);				// SYSEX END
	}
	doSetPinMode(pin, mode) {
		this.write(0xF4);				// PIN_MODE
		this.write(pin);
		this.write(mode);
	}
	doDigitalChannelWrite(channel, value) {
		this.write(0x90 | channel);		// DIGITAL_WRITE
		this.write14Bits(value);
	}
	doReportDigital(channel, enable) {
		this.write(0xD0 | channel);		// REPORT_DIGITAL
		this.write(enable ? 1 : 0);
	}
	doReportAnalog(channel, enable) {
		this.write(0xC0 | channel);		// REPORT_ANALOG
		this.write(enable ? 1 : 0);
	}
	doI2CConfig() {
		this.write(0xF0);				// SYSEX BEGIN
		this.write(0x78);				// I2C_CONFIG
		this.write(0);					//@@ implement delay
		this.write(0);
		this.write(0xF7);				// SYSEX END
	}
	doI2CRead(address, count) {
		if (undefined == count) {
			return;
		}

		this.write(0xF0);				// SYSEX BEGIN
		this.write(0x76);				// I2C_REQUEST

		this.write(address & 0x7F);		// low 7 bits of address
		this.write(0x08);				// read once, stop, 7 bit address

		this.write14Bits(count);

		this.write(0xF7);				// SYSEX END
	}
	doI2CWrite(address, bytes) {
		if (!(bytes instanceof ArrayBuffer))
			bytes = bytes.buffer;
		bytes = new Uint8Array(bytes);

		this.write(0xF0);				// SYSEX BEGIN
		this.write(0x76);				// I2C_REQUEST

		this.write(address & 0x7F);		// low 7 bits of address
		this.write(0);					// write, stop, 7 bit address

		for (let i = 0; i < bytes.length; i++)
			this.write14Bits(bytes[i]);

		this.write(0xF7);				// SYSEX END
	}
	doPocoGetDimensions() {
		this.write(0xF0);				// SYSEX BEGIN
		this.write(0x01);				// POCO_COMMAND (user command)
		this.write(0x01);				// POCO get dimensions
		this.write(0xF7);				// SYSEX END
		return this.messageWait(0x01);	// POCO_COMMAND
	}
	doPocoBegin(x, y, width, height) {
		this.write(0xF0);				// SYSEX BEGIN
		this.write(0x01);				// POCO_COMMAND (user command)
		this.write(0x02);				// POCO begin
		this.write14Bits(x);
		this.write14Bits(y);
		this.write14Bits(width);
		this.write14Bits(height);
		this.write(0xF7);				// SYSEX END
	}
	doPocoEnd() {
		this.write(0xF0);				// SYSEX BEGIN
		this.write(0x01);				// POCO_COMMAND (user command)
		this.write(0x03);				// POCO end
		this.write(0xF7);				// SYSEX END
	}
	doPocoClip(x, y, width, height) {
		this.write(0xF0);				// SYSEX BEGIN
		this.write(0x01);				// POCO_COMMAND (user command)
		this.write(0x04);				// POCO clip
		if (undefined !== x) {
			this.write14Bits(x);
			this.write14Bits(y);
			this.write14Bits(width);
			this.write14Bits(height);
		}
		this.write(0xF7);				// SYSEX END
	}
	doPocoOrigin(x, y) {
		this.write(0xF0);				// SYSEX BEGIN
		this.write(0x01);				// POCO_COMMAND (user command)
		this.write(0x05);				// POCO origin
		if (undefined !== x) {
			this.write14Bits(x);
			this.write14Bits(y);
		}
		this.write(0xF7);				// SYSEX END
	}
	doPocoFillRectangle(color, x, y, width, height) {
		this.write(0xF0);				// SYSEX BEGIN
		this.write(0x01);				// POCO_COMMAND (user command)
		this.write(0x06);				// POCO fillRectangle
		this.write(color >> 14);		// r
		this.write((color >> 7) & 0x7F);// g
		this.write(color & 0x7F);		// b
		this.write14Bits(x);
		this.write14Bits(y);
		this.write14Bits(width);
		this.write14Bits(height);
		this.write(0xF7);				// SYSEX END
	}
	// blendRectangle (7)
	doPocoDrawPixel(color, x, y) {
		this.write(0xF0);				// SYSEX BEGIN
		this.write(0x01);				// POCO_COMMAND (user command)
		this.write(0x08);				// POCO drawPixel
		this.write(color >> 14);		// r
		this.write((color >> 7) & 0x7F);// g
		this.write(color & 0x7F);		// b
		this.write14Bits(x);
		this.write14Bits(y);
		this.write(0xF7);				// SYSEX END
	}
}

class Digital {
	#firmata;
	#pins;
	#port;
	#mode;		// Digital.* mode (not Firmata mode)
	#flag;
	constructor(dictionary, firmata, flag) {
		let mode = dictionary.mode;
		this.#mode = mode;
		if (Digital.Input === mode)
			mode = fDIGITAL_INPUT;
		else if (Digital.InputPullUp === mode)
			mode = fINPUT_PULLUP;
		else if (Digital.Output === mode)
			mode = fDIGITAL_OUTPUT;
		else
			throw new RangeError("invalid mode");

		let pins, port;
		if (flag) {
			const pin = dictionary.pin;
			if ((pin < 0) || (pin >= firmata.pins))
				throw new RangeError("invalid pin");
			firmata.usePin(pin, true);
			firmata.doSetPinMode(pin, mode);
			port = pin >> 3;
			pins = 1 << (pin & 7);
		}
		else {
			port = dictionary.bank;
			if ((port < 0) || (port >= firmata.ports.length))
				throw new RangeError("invalid bank");

			pins = dictionary.pins & 0xFF
			firmata.usePins(pins << (port << 3), true);

			for (let i = 0; i < 8; i++) {
				if (pins & (1 << i))
					firmata.doSetPinMode(i + (port << 3), mode);
			}
		}

		this.#firmata = firmata;
		this.#port = port;
		this.#pins = pins;
		this.#flag = flag;

		if (fDIGITAL_OUTPUT !== mode) {
			const onReadable = dictionary.onReadable;
			if (onReadable)
				firmata.digitalReaders.push({target: this, onReadable, pins: pins << (port << 3)});

			for (let i = 0; i < 8; i++) {
				if (pins & (1 << i))
					firmata.pins[i + (port << 3)].flags |= PIN_REPORT;
			}
		}

		firmata.rebuildDigitalReports();
	}
	close() {
		const firmata = this.#firmata;

		firmata.usePins(this.#pins, false);

		this.#pins = undefined;
		this.#mode = -1;
		this.#port = -1;

		const index = firmata.digitalReaders.findIndex(item => item.target === this);
		if (index >= 0)
			firmata.digitalReaders.splice(index, 1);

		firmata.rebuildDigitalReports();
		this.#firmata = undefined;
	}
	write(value) {
		if (Digital.Output !== this.#mode)
			throw new Error("write only");
		let bits = this.#firmata.ports[this.#port] & ~this.#pins;
		if (this.#flag)
			value = value ? this.#pins : 0;
		bits |= value;
		this.#firmata.ports[this.#port] = bits;
		this.#firmata.doDigitalChannelWrite(this.#port, bits);
	}
	read() {
		if (Digital.Output === this.#mode)
			throw new Error("read only");
		let result = this.#firmata.ports[this.#port] & this.#pins;
		if (this.#flag)
			result = result ? 1 : 0;
		return result;
	}
}
Digital.Input = 0;
Digital.InputPullUp = 1;

Digital.Output = 8;

class Analog {
	#firmata;
	#pin;
	constructor(dictionary, firmata) {
		const pin = dictionary.pin;
		if (!(firmata.pins[pin].modes & (1 << fANALOG_INPUT)))
			throw new RangeError("not analog");

		firmata.usePin(pin, true);
		firmata.doSetPinMode(pin, fANALOG_INPUT);

		this.#pin = pin;
		this.#firmata = firmata;

		const onReadable = dictionary.onReadable;
		if (onReadable) {
			firmata.pins[pin].onReadable = onReadable;
			firmata.pins[pin].target = this;
		}

		firmata.doReportAnalog(firmata.analogMap[pin], true);
	}
	close() {
		const firmata = this.#firmata;

		firmata.usePin(this.#pin, false);
		firmata.doReportAnalog(firmata.analogMap[this.#pin], false);

		const pin = firmata.pins[this.#pin];
		delete pin.onReadable;
		delete pin.target;

		this.#pin = -1;
		this.#firmata = undefined;
	}
	read() {
		return this.#firmata.pins[this.#pin].value;
	}
}

class I2C {
	#firmata;
	#state;
	constructor(dictionary, firmata) {
		this.#firmata = firmata;
		this.#state = {
			target: this,
			onReadable: dictionary.onReadable,
			address: dictionary.address,
			received: [],
		}

		firmata.i2cReaders.push(this.#state);
		firmata.doI2CConfig();
	}
	close() {
		firmata.i2cReaders.splice(firmata.i2cReaders.indexOf(this.#state), 1);

		this.#firmata = undefined;
		this.#state = undefined;
	}
	write(buffer) {
		this.#firmata.doI2CWrite(this.#state.address, buffer);
	}
	read(count) {
		if (!count)
			return this.#state.received.shift();

		this.#firmata.doI2CRead(this.#state.address, count);
	}
}

class Poco {
	#firmata;
	constructor(dictionary = {}, firmata) {
		this.#firmata = firmata;
		firmata.poco = {
			target: this,
			onReadable: dictionary.onReadable,
			received: [],
		}
	}
	begin(x, y, width, height) {
		if (undefined === x)
			x = 0;
		if (undefined === y)
			y = 0;
		if (undefined === width)
			width = this.width - x;
		if (undefined === height)
			height = this.height - y;
		this.#firmata.doPocoBegin(x, y, width, height);
	}
	end() {
		this.#firmata.doPocoEnd();
	}
	clip(x, y, width, height) {
		if (undefined === x)
			this.#firmata.doPocoClip();
		else
			this.#firmata.doPocoClip(x, y, width, height);
	}
	origin(x, y) {
		if (undefined === x)
			this.#firmata.doPocoOrigin();
		else
			this.#firmata.doPocoOrigin(x, y);
	}
	makeColor(r, g, b) {
		return ((r >> 1) << 14) | ((g >> 1) << 7) | (b >> 1);		// RGB 7:7:7
	}
	fillRectangle(color, x, y, width, height) {
		this.#firmata.doPocoFillRectangle(color, x, y, width, height);
	}
	drawPixel(color, x, y) {
		this.#firmata.doPocoDraw(color, x, y);
	}
}

export class FirmataClientSerial extends FirmataClient {
	constructor(dictionary) {
		super(dictionary);

		this.transport = new Serial({
			baud: dictionary.baud ?? 57600,
			onReadable: count => this.onReadable(count),
			onWritable: () => {
				if (!this.transport.first)
					return;
				delete this.transport.first;
				this.onConnect();
			}
		});
		this.transport.first = true;
	}
}

export class FirmataClientTCP extends FirmataClient {
	constructor(dictionary) {
		super(dictionary);

		this.transport = new TCP({
			address: dictionary.address,
			port: dictionary.port ?? 3030,
			onReadable: count => this.onReadable(count),
			onWritable: () => {
				if (!this.transport.first)
					return;
				delete this.transport.first;
				this.onConnect();
			}
		});
		this.transport.first = true;
		this.transport.format = "number";
	}
}

export default Object.freeze({
	FirmataClient,
	FirmataClientSerial,
	FirmataClientTCP,
});
