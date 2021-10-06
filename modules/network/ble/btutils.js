/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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
 * This file incorporates work covered by the following copyright and  
 * permission notice:  
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */
/*
 * Portions based on Kinoma LowPAN Framework: Kinoma Bluetooth 4.2 Stack
 */

import GAP from "gap";

const toArrayBuffer = Object.freeze({
	Uint8Array(value) {
		return value.buffer;
	},
	Int8Array(value) {
		return value.buffer;
	},
	Int16Array(value) {
		return value.buffer;
	},
	Uint16Array(value) {
		return value.buffer;
	},
	Array(value) {
		return new Uint8Array(value).buffer;
	},
	String(value) {
		return ArrayBuffer.fromString(value);
	},
	Uint8(value) {
		return Uint8Array.of(value & 0xFF).buffer;
	},
	Int16(value) {
		return Uint8Array.of(value & 0xFF, (value >> 8) & 0xFF).buffer;
	},
	Uint16(value) {
		return Uint8Array.of(value & 0xFF, (value >> 8) & 0xFF).buffer;
	},
	Uint32(value) {
		return Uint8Array.of(value & 0xFF, (value >> 8) & 0xFF, (value >> 16) & 0xFF, (value >> 24) & 0xFF).buffer;
	}
}, true);

export function typedValueToBuffer(type, value) {
	type = toArrayBuffer[type];
	return type ? type(value) : value;
}

const fromArrayBuffer = Object.freeze({
	Uint8Array(buffer) {
		return new Uint8Array(buffer);
	},
	Int8Array(buffer) {
		return new Int8Array(buffer);
	},
	Int16Array(buffer) {
		return new Int16Array(buffer);
	},
	Uint16Array(buffer) {
		return new Uint16Array(buffer);
	},
	Array(buffer) {
		return new Uint8Array(buffer);
	},
	String(buffer) {
		return String.fromArrayBuffer(buffer);
	},
	Uint8(buffer) {
		return new Uint8Array(buffer)[0] & 0xFF;
	},
	Int16(buffer) {
		return (new DataView(buffer)).getInt16(0, true);
	},
	Uint16(buffer) {
		return (new DataView(buffer)).getUint16(0, true);
	},
	Uint32(buffer) {
		return (new DataView(buffer)).getUint32(0, true);
	}
}, true);

export function typedBufferToValue(type, buffer) {
	type = fromArrayBuffer[type];
	return type ? type(buffer) : buffer;
}

const hex = "0123456789ABCDEF";
function toHexString(buffer, delimeter = '') {
	const bytes = new Uint8Array(buffer);
	const length = buffer.byteLength;
	let result = new Array(length);
	for (let index = 0; index < length; ++index) {
		const byte = bytes[index];
		result[length - index - 1] = hex[byte >> 4] + hex[byte & 0x0F];
	}
	return result.join(delimeter);
}

export class Bytes extends ArrayBuffer {
	constructor(bytes, littleEndian) {
		let byteLength;
		if ("string" === typeof bytes)
			byteLength = bytes.length >> 1;
		else if (("object" === typeof bytes) && (bytes instanceof ArrayBuffer))
			byteLength = bytes.byteLength;
		else
			throw new Error("unsupported type");
		super(byteLength);
		this.set(bytes, true === littleEndian);
	}
	toString() {
		// this function assumes the bytes are in little endian order
		const byteLength = this.byteLength;
		if (6 === byteLength)
			return toHexString(this, ':');
		if (16 === byteLength) {
			const bytes = new Uint8Array(this);
			const string =
				toHexString(bytes.slice(12, 16)) + '-' +
				toHexString(bytes.slice(10, 12)) + '-' +
				toHexString(bytes.slice(8, 10)) + '-' +
				toHexString(bytes.slice(6, 8)) + '-' +
				toHexString(bytes.slice(0, 6));
			return string;
		}

		return toHexString(this);
	}
	set(bytes, littleEndian) @ "xs_bytes_set"
	equals(bytes) @ "xs_bytes_equals"
	
}
Object.freeze(Bytes.prototype);

function uuid(strings) {
	return new Bytes(strings[0].replaceAll("-", ""), true);
}

function address(strings) {
	return new Bytes(strings[0].replaceAll(":", ""), true);
}

function serializeUUID16List(data) {
	let count = data.length;
	let result = new Uint8Array(count * 2);
	for (let i = 0; i < count; ++i)
		result.set(new Uint8Array(data[i]), i * 2);
	return result;
}

function serializeUUID128List(data) {
	let count = data.length;
	let result = new Uint8Array(count * 16);
	for (let i = 0; i < count; ++i)
		result.set(new Uint8Array(data[i]), i * 16);
	return result;
}

function serializeString(data) {
	let buffer = ArrayBuffer.fromString(data);
	let result = new Uint8Array(buffer);
	return result;
}

function serializeManufacturerSpecificData({identifier, data}) {
	let length = 2 + (data?.length ?? 0);
	let result = new Uint8Array(length);
	result[0] = identifier & 0xFF;
	result[1] = (identifier >> 8) & 0xFF;
	if (data)
		result.set(data, 2);
	return result;
}

function serializeConnectionInterval({intervalMin, intervalMax}) {
	let buffer = new ArrayBuffer(4);
	let result = new DataView(buffer);
	result.setUint16(0, intervalMin, true);
	result.setUint16(2, intervalMax, true);
	return new Uint8Array(buffer);
}

function serializeServiceData16({uuid, data = null}) {
	let length = 2 + (data ? data.length : 0);
	let result = new Uint8Array(length);
	result.set(new Uint8Array(uuid), 0);
	if (data)
		result.set(data, 2);
	return result;
}

function serializeServiceData128({uuid, data = null}) {
	let length = 16 + (data ? data.length : 0);
	let result = new Uint8Array(length);
	result.set(new Uint8Array(uuid), 0);
	if (data)
		result.set(data, 16);
	return result;
}

function serializeAddress(data) {
	let result = new Uint8Array(data);
	return result;
}

class AdvertisementSerializer {
	static incompleteUUID16List(param) {
		return {
			type: GAP.ADType.INCOMPLETE_UUID16_LIST,
			data: serializeUUID16List(param)
		};
	}
	static completeUUID16List(param) {
		return {
			type: GAP.ADType.COMPLETE_UUID16_LIST,
			data: serializeUUID16List(param)
		};
	}
	static incompleteUUID128List(param) {
		return {
			type: GAP.ADType.INCOMPLETE_UUID128_LIST,
			data: serializeUUID128List(param)
		};
	}
	static completeUUID128List(param) {
		return {
			type: GAP.ADType.COMPLETE_UUID128_LIST,
			data: serializeUUID128List(param)
		};
	}
	static shortName(param) {
		return {
			type: GAP.ADType.SHORTENED_LOCAL_NAME,
			data: serializeString(param)
		};
	}
	static completeName(param) {
		return {
			type: GAP.ADType.COMPLETE_LOCAL_NAME,
			data: serializeString(param)
		};
	}
	static flags(param) {
		return {
			type: GAP.ADType.FLAGS,
			data: [param & 0xFF]
		};
	}
	static manufacturerSpecific(param) {
		return {
			type: GAP.ADType.MANUFACTURER_SPECIFIC_DATA,
			data: serializeManufacturerSpecificData(param)
		};
	}
	static txPowerLevel(param) {
		return {
			type: GAP.ADType.TX_POWER_LEVEL,
			data: [param & 0xFF]
		};
	}
	static connectionInterval(param) {
		return {
			type: GAP.ADType.SLAVE_CONNECTION_INTERVAL_RANGE,
			data: serializeConnectionInterval(param)
		};
	}
	static solicitationUUID16List(param) {
		return {
			type: GAP.ADType.SOLICITATION_UUID16_LIST,
			data: serializeUUID16List(param)
		};
	}
	static solicitationUUID128List(param) {
		return {
			type: GAP.ADType.SOLICITATION_UUID128_LIST,
			data: serializeUUID128List(param)
		};
	}
	static serviceDataUUID16(param) {
		return {
			type: GAP.ADType.SERVICE_DATA_UUID16,
			data: serializeServiceData16(param)
		};
	}
	static serviceDataUUID128(param) {
		return {
			type: GAP.ADType.SERVICE_DATA_UUID128,
			data: serializeServiceData128(param)
		};
	}
	static appearance(param) {
		return {
			type: GAP.ADType.APPEARANCE,
			data: [param & 0xFF, (param >> 8) & 0xFF]
		};
	}
	static publicAddress(param) {
		return {
			type: GAP.ADType.PUBLIC_TARGET_ADDRESS,
			data: serializeAddress(param)
		};
	}
	static randomAddress(param) {
		return {
			type: GAP.ADType.RANDOM_TARGET_ADDRESS,
			data: serializeAddress(param)
		};
	}
	static advertisingInterval(param) {
		return {
			type: GAP.ADType.ADVERTISING_INTERVAL,
			data: serializeUint16(param)
		};
	}
	//static deviceAddress(param) {}
	static role(param) {
		return {
			type: GAP.ADType.LE_ROLE,
			data: serializeByte(param)
		};
	}
	static uri(param) {
		return {
			type: GAP.ADType.URI,
			data: serializeString(param)
		};
	}
};

export class Advertisement {
	#buffer;
	#data;
	
	constructor(buffer) {
		this.#buffer = buffer;
		this.#data = new Uint8Array(buffer);
	}
	get buffer() {
		return this.#buffer;
	}
	get appearance() {
		const index = this.findIndex(GAP.ADType.APPEARANCE);
		if (-1 != index) {
			let start = index + 2;
			const data = this.#data;
			return (data[start] | (data[start+1] << 8));
		}
	}
	get completeName() {
		const index = this.findIndex(GAP.ADType.COMPLETE_LOCAL_NAME);
		if (-1 != index) {
			return this.#getStringType(index);
		}
	}
	get shortName() {
		const index = this.findIndex(GAP.ADType.SHORTENED_LOCAL_NAME);
		if (-1 != index)
			return this.#getStringType(index);
	}
	get manufacturerSpecific() {
		const index = this.findIndex(GAP.ADType.MANUFACTURER_SPECIFIC_DATA);
		if (-1 != index) {
			let start = index + 2;
			const data = this.#data;
			const adLength = data[index];
			const identifier = data[start] | (data[start+1] << 8);
			start += 2;
			let end = start + adLength - 1
			return { identifier, data: new Uint8Array(this.#buffer.slice(start, end)) };
		}
	}
	get flags() {
		const index = this.findIndex(GAP.ADType.FLAGS);
		if (-1 != index)
			return this.#data[index+2];
	}
	get completeUUID16List() {
		const index = this.findIndex(GAP.ADType.COMPLETE_UUID16_LIST);
		if (-1 != index) 
			return this.#getUUID16ListType(index);
	}
	get incompleteUUID16List() {
		const index = this.findIndex(GAP.ADType.INCOMPLETE_UUID16_LIST);
		if (-1 != index) 
			return this.#getUUID16ListType(index);
	}
	findIndex(type, index = 0) {
		const data = this.#data;
		const length = data.byteLength;
		while (index < length) {
			const adLength = data[index]; ++index;
			const adType = data[index]; ++index;
			if (type == adType)
				return index - 2;
			else
				index += adLength - 1;
		}
		return -1;
	}
	static serialize(obj) {
		let parts = [];
		for (let key in obj)
			parts.push(AdvertisementSerializer[key](obj[key]));
		let length = 2 * parts.length;	// one byte each for type and length
		for (let i = 0; i < parts.length; ++i) {
			let part = parts[i];
			length += part.data.length;
		}
		if (length > GAP.MAX_AD_LENGTH)
			throw new Error("advertisement exceeds maximum length");
		let advertisement = new Uint8Array(length);
		for (let i = 0, j = 0; i < parts.length; ++i) {
			let part = parts[i];
			advertisement[j++] = 1 + part.data.length;
			advertisement[j++] = part.type;
			advertisement.set(part.data, j);
			j += part.data.length;
		}
		return advertisement.buffer;
	}
	
	#getStringType(index) {
		const adLength = this.#data[index];
		const start = index + 2, end = start + adLength - 1;
		return String.fromArrayBuffer(this.#buffer.slice(start, end));
	}
	#getUUID16ListType(index) {
		const adLength = this.#data[index];
		const count = (adLength - 1) / 2;
		let uuidList = new Array(count);
		for (let i = 0, start = index + 2; i < count; ++i, start += 2)
			uuidList[i] = new Bytes(this.#buffer.slice(start, start + 2));
		return uuidList;
	}
}
Object.freeze(Advertisement.prototype);

export { uuid, address };
