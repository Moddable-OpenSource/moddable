/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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

class GATTServerConnection extends Native("xs_gattserverconnection_destructor") {
	constructor() {throw new Error};
	notify(characteristic, value) { return native("xs_gattserverconnection_notify").call(this, characteristic, value); }
	close() { return native("xs_gattserverconnection_close").call(this); }
	get maximumWrite() { return native("xs_gattserverconnection_get_maximumWrite").call(this); }
	replyToPasskey(action, value) { return native("xs_gattserverconnection_replyToPasskey").call(this, action, value); }
}

class GATTServerCharacteristic extends Native("xs_gattservercharacteristic_destructor") {
	constructor() {throw new Error};
}

class GATTServer extends Native("xs_gattserver_destructor") {
	constructor(options) {
		super();
		native("xs_gattserver_build").call(this, options, GATTServerConnection.prototype, GATTServerCharacteristic.prototype);
		const services = options.services;
		for (let i = 0; i < services.length; i++) {
			const service = services[i];
			if ("1800" !== service.uuid) {
				this.addService(service);
				continue;
			}
			let name, appearance;
			service.characteristics?.forEach(characteristic => {
				if ("2a00" === characteristic.uuid) {
					name = characteristic.value;
					if (undefined === name)
						options.onWarning?.(`service 1800, characteristic 2a00 must have static value with NimBLE`);
				}
				else if ("2a01" === characteristic.uuid) {
					appearance = characteristic.value;
					if (undefined === appearance)
						options.onWarning?.(`service 1800, characteristic 2a01 must have static value with NimBLE`);
				}
				else
					options.onWarning?.(`service 1800, characteristic ${characteristic} unsupported with NimBLE`);
			});
			native("xs_gattserver_configure").call(this, name, appearance);
		}
	}
	close() { return native("xs_gattserver_close").call(this); }

	addService(service) { return native("xs_gattserver_addService").call(this, service); }
	deleteService(service) { return native("xs_gattserver_deleteService").call(this, service); }

	startAdvertising(scan, response) {
		if (response)
			native("xs_gattserver_startAdvertising").call(this, convert(scan), convert(response));
		else
			native("xs_gattserver_startAdvertising").call(this, convert(scan));
	}
	stopAdvertising() { return native("xs_gattserver_stopAdvertising").call(this); }

	static properties = Object.freeze({
		authenticatedSignedWrites: (1 << 6),
		broadcast: (1 << 0),
		indicate: (1 << 5),
		notify: (1 << 4),
		read: (1 << 1),
		readAuthenticated: (1 << 10) | (1 << 1),
		readEncrypted: (1 << 9) | (1 << 1),
		reliableWrite: (1 << 7),
		subscribeAuthenticated: (1 << 16) | (1 << 4),
		subscribeEncrypted: (1 << 15) | (1 << 4),
		write: (1 << 3),
		writeAuthenticated: (1 << 13) | (1 << 3),
		writeEncrypted: (1 << 12) | (1 << 3),
		writeWithOutResponse: (1 << 2),
	});
}

function convert(ad) {
	const buffer = new ArrayBuffer(0, {maxByteLength: 31});
	const bytes = new Uint8Array(buffer);
	for (let name in ad) {
		let offset = bytes.length;
		let value = ad[name];
		if (ArrayBuffer.isView(value))
			value = new Uint8Array(value.buffer, value.byteOffset, value.byteLength);

		switch (name) {
			case "name":
				value = ArrayBuffer.fromString(value);
				buffer.resize(offset + value.byteLength + 2);
				bytes[offset++] = value.byteLength + 1;
				bytes[offset++] = 9;		// local name
				bytes.set(new Uint8Array(value), offset);
				break;

			case "services":		// array of 8, 16, and 32 bit UUIDs....
				value = value.map(uuid => {
					uuid = Uint8Array.fromHex(uuid.replaceAll("-", ""));
					if ((2 !== uuid.length) && (4 !== uuid.length) && (16 !== uuid.length))
						throw new Error("invalid UUID");
					return uuid;
				});

				[2, 4, 16].forEach(length => {
					let i = value.filter(uuid => uuid.length === length);
					if (i.length) {
						buffer.resize(offset + i.length * length + 2);
						bytes[offset++] = i.length * length + 1;
						bytes[offset++] = (2 === length) ? 3 : ((4 === length) ? 5 : 7);		// incomplete 16 / 32 / 128
						i.forEach(uuid => {
							for (let j = 0; j < length; j++)
								bytes[offset++] = uuid[(length - 1)- j];
						});
					}
				});
				break;

			case "manufacturerData":
				buffer.resize(offset + value.data.byteLength + 2 + 2);
				bytes[offset++] = value.data.byteLength + 1 + 2;
				bytes[offset++] = 255;		// manufacturer data
				bytes[offset++] = value.manufacturer;
				bytes[offset++] = value.manufacturer >> 8;
				value = value.data;
				if (ArrayBuffer.isView(value))
					bytes.set(new Uint8Array(value.buffer, value.byteOffset, value.byteLength), offset);
				else
					bytes.set(new Uint8Array(value.buffer), offset);
				break;

			case "flags":
				buffer.resize(offset + 2 + 1);
				bytes[offset++] = 1 + 1;
				bytes[offset++] = 1;		// flags
				bytes[offset++] = value;
				break;

			default:
				if (("string" === typeof name) && (name === parseInt(name).toString()))
					name = parseInt(name);
				if (("number" === typeof name) && ((0 < name) && (name <= 255))) {
					buffer.resize(offset + value.byteLength + 2);
					bytes[offset++] = value.byteLength + 1;
					bytes[offset++] = name;
					bytes.set(value, offset);
				}
				else
					throw new Error("unrecognized " + name);
		}
	}

	return buffer;
}

export {GATTServer}
