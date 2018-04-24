/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
 * Portions based on Kinoma LowPAN Framework: Kinoma Bluetooth 4.2 Stack
 */

import Hex from "hex";
import GAP from "gap";

export class BluetoothAddress {
	static toString(buffer) {
		return Hex.toString(buffer, ':');
	}
	static toBuffer(string) {
		return Hex.toBuffer(string, ':');
	}
}
Object.freeze(BluetoothAddress.prototype);

export class UUID {
	static toString(buffer) @ "xs_btuuid_toString"
	static toBuffer(string) @ "xs_btuuid_toBuffer"
}
Object.freeze(UUID.prototype);

function serializeUUID16List(data) {
	let count = data.length;
	let result = new Uint8Array(count * 2);
	for (let j = 0, i = 0; i < count; ++i) {
		let uuid = new Uint8Array(UUID.toBuffer(data[i]));
		result[j++] = uuid[1];
		result[j++] = uuid[0];
	}
	return result;
}

function serializeUUID128List(data) {
	let count = data.length;
	let result = new Uint8Array(count * 16);
	for (let i = 0; i < count; ++i) {
		let uuid = (new Uint8Array(UUID.toBuffer(data[i]))).reverse();
		result.set(uuid, i * 16);
	}
	return result;
}

function serializeString(data) {
	let buffer = ArrayBuffer.fromString(data);
	let result = new Uint8Array(buffer);
	return result;
}

function serializeManufacturerSpecificData({identifier, data = null}) {
	let length = 2 + (data ? data.length : 0);
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
	result.setUint16(intervalMin, 0, true);
	result.setUint16(intervalMax, 2, true);
	return result;
}

function serializeServiceData16({uuid, data = null}) {
	let length = 2 + (data ? data.length : 0);
	let result = new Uint8Array(length);
	result[0] = uuid & 0xFF;
	result[1] = (uuid >> 8) & 0xFF;
	if (data)
		result.set(data, 2);
	return result;
}

function serializeServiceData128({uuid, data = null}) {
	let length = 16 + (data ? data.length : 0);
	let result = new Uint8Array(length);
	result.set((new Uint8Array(UUID.toBuffer(uuid))).reverse(), 0);
	if (data)
		result.set(data, 16);
	return result;
}

function serializeAddress(data) {
	let result = new Uint8Array(BluetoothAddress.toBuffer(data));
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
			type: GAP.ADType.MANUFACTURER_SPECIFIC_DATA,
			data: serializeConnectionInterval(param)
		};
	}
	static solicitationUUID16List(param) {
		return {
			type: GAP.ADType.SOLICITATION_UUID16_LIST,
			data: [data & 0xFF, (data >> 8) & 0xFF]
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
			data: [data & 0xFF, (data >> 8) & 0xFF]
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
	constructor(buffer) {
		this._buffer = buffer;
		this._data = new Uint8Array(buffer);
	}
	get completeName() {
		let index = this.find(GAP.ADType.COMPLETE_LOCAL_NAME);
		if (-1 != index) {
			return this._getStringType(index);
		}
	}
	get shortName() {
		let index = this.find(GAP.ADType.SHORTENED_LOCAL_NAME);
		if (-1 != index)
			return this._getStringType(index);
	}
	get manufacturerSpecific() {
		let index = this.find(GAP.ADType.MANUFACTURER_SPECIFIC_DATA);
		if (-1 != index) {
			let data = this._data;
			let adLength = data[index];
			let start = index + 2;
			let identifier = data[start] | (data[start+1] << 8);
			start += 2;
			let end = start + adLength - 1
			return { identifier, data: new Uint8Array(this._buffer.slice(start, end)) };
		}
	}
	get flags() {
		let index = this.find(GAP.ADType.FLAGS);
		if (-1 != index)
			return this._data[index+2];
	}
	find(type) {
		let data = this._data;
		let i = 0, length = data.byteLength;
		while (i < length) {
			let adLength = data[i]; ++i;
			let adType = data[i]; ++i;
			if (type == adType)
				return i - 2;
			else
				i += adLength - 1;
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
	
	_getStringType(index) {
		let adLength = this._data[index];
		let start = index + 2, end = start + adLength - 1;
		return String.fromArrayBuffer(this._buffer.slice(start, end));
	}
}
Object.freeze(Advertisement.prototype);

export default {
	BluetoothAddress, UUID, Advertisement
};
