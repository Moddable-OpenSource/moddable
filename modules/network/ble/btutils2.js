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

import Hex from "hex";

const MAX_AD_LENGTH = 31;

export class BluetoothAddress {
	static toString(buffer) {
		return Hex.toString(buffer, ':');
	}
	static toBuffer(string) {
		return Hex.toBuffer(string, ':');
	}
}

export class UUID {
	static toString(buffer) @ "xs_btuuid_toString"
	static toBuffer(string) @ "xs_btuuid_toBuffer"
}

const ADType = {
	incompleteUUID16List: 0x02,
	completeUUID16List: 0x03,
	incompleteUUID128List: 0x06,
	completeUUID128List: 0x07,
	/* Local Name */
	shortName: 0x08,
	completeName: 0x09,
	/* Flags */
	flags: 0x01,
	/* Manufacturer Specific Data */
	manufacturerSpecific: 0xFF,
	/* TX Power Level */
	txPowerLevel: 0x0A,
	connectionInterval: 0x12,
	/* Service Solicitation */
	solicitationUUID16List: 0x14,
	solicitationUUID128List: 0x15,
	/* Service Data */
	serviceDataUUID16: 0x16,
	serviceDataUUID128: 0x21,
	/* Appearance */
	appearance: 0x19,
	/* Public Target Address */
	publicAddress: 0x17,
	/* Random Target Address */
	randomAddress: 0x18,
	/* Advertising Interval */
	advertisingInterval: 0x1A,
	/* LE Bluetooth Device Address */
	deviceAddress: 0x1B,
	/* LE Role */
	role: 0x1C,
	/* URI */
	uri: 0x24
};

function serializeUint16(data) {
	let buffer = new ArrayBuffer(2);
	let result = new Uint8Array(buffer);
	result[0] = uuid[1];
	result[1] = uuid[0];
	return buffer;
}

function serializeUUID16List(data) {
	let count = data.length;
	let buffer = new ArrayBuffer(count * 2);
	let result = new Uint8Array(buffer);
	for (let j = 0, i = 0; i < count; ++i) {
		let uuid = new Uint8Array(UUID.toBuffer(data[i]));
		result[j++] = uuid[1];
		result[j++] = uuid[0];
	}
	return buffer;
}

function serializeUUID128List(data) {
	let count = data.length;
	let buffer = new ArrayBuffer(count * 16);
	let result = new Uint8Array(buffer);
	for (let i = 0; i < count; ++i) {
		let uuid = (new Uint8Array(UUID.toBuffer(data[i]))).reverse();
		result.set(uuid, i * 16);
	}
	return buffer;
}

function serializeString(data) {
	let length = data.length;
	let buffer = new ArrayBuffer(length);
	let result = new Uint8Array(buffer);
	for (let i = 0; i < length; i++)
		result[i] = data.charCodeAt(i);
	return buffer;
}

function serializeManufacturerSpecificData({identifier, data = null}) {
	let length = 2 + (data ? data.length : 0);
	let buffer = new ArrayBuffer(length);
	let result = new Uint8Array(buffer);
	result[0] = identifier & 0xFF;
	result[1] = (identifier >> 8) & 0xFF;
	if (data)
		result.set(data, 2);
	return buffer;
}

function serializeConnectionInterval({intervalMin, intervalMax}) {
	let buffer = new ArrayBuffer(4);
	let result = new DataView(buffer);
	result.setUint16(intervalMin, 0, true);
	result.setUint16(intervalMax, 2, true);
	return buffer;
}

function serializeServiceData16({uuid, data = null}) {
	let length = 2 + (data ? data.length : 0);
	let buffer = new ArrayBuffer(length);
	let result = new Uint8Array(buffer);
	result[0] = uuid & 0xFF;
	result[1] = (uuid >> 8) & 0xFF;
	if (data)
		result.set(data, 2);
	return buffer;
}

function serializeServiceData128({uuid, data = null}) {
	let length = 16 + (data ? data.length : 0);
	let buffer = new ArrayBuffer(length);
	let result = new Uint8Array(buffer);
	result.set((new Uint8Array(UUID.toBuffer(uuid))).reverse(), 0);
	if (data)
		result.set(data, 16);
	return buffer;
}

const AdvertisementSerializer = {
	["incompleteUUID16List"]: param => {
		return {
			type: ADType.incompleteUUID16List,
			data: serializeUUID16List(param)
		};
	},
	["completeUUID16List"]: param => {
		return {
			type: ADType.completeUUID16List,
			data: serializeUUID16List(param)
		};
	},
	["incompleteUUID128List"]: param => {
		return {
			type: ADType.incompleteUUID128List,
			data: serializeUUID128List(param)
		};
	},
	["completeUUID128List"]: param => {
		return {
			type: ADType.completeUUID128List,
			data: serializeUUID128List(param)
		};
	},
	["shortName"]: param => {
		return {
			type: ADType.shortName,
			data: serializeString(param)
		};
	},
	["completeName"]: param => {
		return {
			type: ADType.completeName,
			data: serializeString(param)
		};
	},
	["flags"]: param => {
		return {
			type: ADType.flags,
			data: [param & 0xFF]
		};
	},
	["manufacturerSpecific"]: param => {
		return {
			type: ADType.manufacturerSpecific,
			data: serializeManufacturerSpecificData(param)
		};
	},
	["txPowerLevel"]: param => {
		return {
			type: ADType.txPowerLevel,
			data: [param & 0xFF]
		};
	},
	["connectionInterval"]: param => {
		return {
			type: ADType.connectionInterval,
			data: serializeConnectionInterval(param)
		};
	},
	["solicitationUUID16List"]: param => {
		return {
			type: ADType.solicitationUUID16List,
			data: serializeUUID16List(param)
		};
	},
	["solicitationUUID128List"]: param => {
		return {
			type: ADType.solicitationUUID128List,
			data: serializeUUID128List(param)
		};
	},
	["serviceDataUUID16"]: param => {
		return {
			type: ADType.serviceDataUUID16,
			data: serializeServiceData16(param)
		};
	},
	["serviceDataUUID128"]: param => {
		return {
			type: ADType.serviceDataUUID128,
			data: serializeServiceData128(param)
		};
	},
	["appearance"]: param => {
		return {
			type: ADType.appearance,
			data: serializeUint16(param)
		};
	},
	["publicAddress"]: param => {
		return {
			type: ADType.publicAddress,
			data: BluetoothAddress.toBuffer(param)
		};
	},
	["randomAddress"]: param => {
		return {
			type: ADType.randomAddress,
			data: BluetoothAddress.toBuffer(param)
		};
	},
	["advertisingInterval"]: param => {
		return {
			type: ADType.advertisingInterval,
			data: serializeUint16(param)
		};
	},
//	["deviceAddress"]: null,
	["role"]: param => {
		return {
			type: ADType.role,
			data: [param & 0xFF]
		};
	},
	["uri"]: param => {
		return {
			type: ADType.uri,
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
		let index = this.find(ADType.completeName);
		if (-1 != index) {
			return this._getStringType(index);
		}
	}
	get shortName() {
		let index = this.find(ADType.shortName);
		if (-1 != index)
			return this._getStringType(index);
	}
	get manufacturerSpecific() {
		let index = this.find(ADType.manufacturerSpecific);
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
		let index = this.find(ADType.flags);
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
	}
	_getStringType(index) {
		let adLength = this._data[index];
		let start = index + 2, end = start + adLength - 1;
		return String.fromArrayBuffer(this._buffer.slice(start, end));
	}
}

export default {
	BluetoothAddress, UUID, Advertisement
};
