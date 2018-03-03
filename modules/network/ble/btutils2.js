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
			let start = index + 2, end = start + adLength - 1;
			return new Uint8Array(this._buffer.slice(start, end));
		}
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
	_getStringType(index) {
		let adLength = this._data[index];
		let start = index + 2, end = start + adLength - 1;
		return String.fromArrayBuffer(this._buffer.slice(start, end));
	}
}

export default {
	BluetoothAddress, UUID, Advertisement
};
