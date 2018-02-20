/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

/**
 * Kinoma LowPAN Framework: Kinoma Bluetooth Stack
 * Bluetooth v4.2 - GAP Advertising Data
 */

import Utils from "utils";
import {UUID, BluetoothAddress} from "btutils";
import ByteBuffer from "buffers";
import GAP from "gap";

function serializeUUIDList(data) {
	let buffer = ByteBuffer.allocateUint8Array(GAP.MAX_AD_LENGTH - 2, true);
	for (let uuidStr of data) {
		let uuid = UUID.getByString(uuidStr);
		if (uuid.isUUID16()) {
			buffer.putInt16(uuid.toUUID16());
		} else {
			buffer.putByteArray(uuid.toUUID128());
		}
	}
	buffer.flip();
	return buffer.getByteArray();
}

function parseUUIDList(size, data) {
	let buffer = ByteBuffer.wrap(data);
	let uuidList = new Array();
	while (buffer.remaining() > 0) {
		uuidList.push(UUID.getByUUID(buffer.getByteArray(size)).toString());
	}
	return uuidList;
}

function serializeManufacturerSpecificData({identifier, data = null}) {
	let buffer = ByteBuffer.allocateUint8Array(GAP.MAX_AD_LENGTH - 2, true);
	buffer.putInt16(identifier);
	if (data != undefined && data != null) {
		buffer.putByteArray(data);
	}
	buffer.flip();
	return buffer.getByteArray();
}

function parseManufacturerSpecificData(data) {
	return {
		identifier: Utils.toInt16(data),
		data: Array.from(data.slice(2))
	};
}

function serializeConnectionInterval({intervalMin, intervalMax}) {
	let buffer = ByteBuffer.allocateUint8Array(4, true);
	buffer.putInt16(intervalMin);
	buffer.putInt16(intervalMax);
	return buffer.array;
}

function parseConnectionInterval(data) {
	return {
		intervalMin: Utils.toInt(data, 0, 2),
		intervalMax: Utils.toInt(data, 2, 2)
	};
}

function serializeServiceData({uuid, data = null}) {
	let buffer = ByteBuffer.allocateUint8Array(GAP.MAX_AD_LENGTH - 2, true);
	buffer.putByteArray(UUID.getByString(uuid).getRawArray());
	if (data != null) {
		buffer.putByteArray(data);
	}
	buffer.flip();
	return buffer.getByteArray();
}

function parseServiceData(size, data) {
	let buffer = ByteBuffer.wrap(data);
	return {
		uuid: UUID.getByUUID(buffer.getByteArray(size)).toString(),
		data: Array.from(buffer.getByteArray())
	};
}

function parseCharArray(data) {
	let str = "";
	for (let c of data) {
		str = str + String.fromCharCode(c);
	}
	return str;
}

const AdvertisingDataSerializer = {
	["incompleteUUID16List"]: param => {
		return {
			type: GAP.ADType.INCOMPLETE_UUID16_LIST,
			data: serializeUUIDList(param)
		};
	},
	["completeUUID16List"]: param => {
		return {
			type: GAP.ADType.COMPLETE_UUID16_LIST,
			data: serializeUUIDList(param)
		};
	},
	["incompleteUUID128List"]: param => {
		return {
			type: GAP.ADType.INCOMPLETE_UUID128_LIST,
			data: serializeUUIDList(param)
		};
	},
	["completeUUID128List"]: param => {
		return {
			type: GAP.ADType.COMPLETE_UUID128_LIST,
			data: serializeUUIDList(param)
		};
	},
	["shortName"]: param => {
		return {
			type: GAP.ADType.SHORTENED_LOCAL_NAME,
			data: Utils.toCharArray(param)
		};
	},
	["completeName"]: param => {
		return {
			type: GAP.ADType.COMPLETE_LOCAL_NAME,
			data: Utils.toCharArray(param)
		};
	},
	["flags"]: param => {
		return {
			type: GAP.ADType.FLAGS,
			data: [param & 0xFF]
		};
	},
	["manufacturerSpecific"]: param => {
		return {
			type: GAP.ADType.MANUFACTURER_SPECIFIC_DATA,
			data: serializeManufacturerSpecificData(param)
		};
	},
	["txPowerLevel"]: param => {
		return {
			type: GAP.ADType.TX_POWER_LEVEL,
			data: [param & 0xFF]
		};
	},
	["connectionInterval"]: param => {
		return {
			type: GAP.ADType.MANUFACTURER_SPECIFIC_DATA,
			data: serializeConnectionInterval(param)
		};
	},
	["solicitationUUID16List"]: param => {
		return {
			type: GAP.ADType.SOLICITATION_UUID16_LIST,
			data: serializeUUIDList(param)
		};
	},
	["solicitationUUID128List"]: param => {
		return {
			type: GAP.ADType.SOLICITATION_UUID128_LIST,
			data: serializeUUIDList(param)
		};
	},
	["serviceDataUUID16"]: param => {
		return {
			type: GAP.ADType.SERVICE_DATA_UUID16,
			data: serializeServiceData(param)
		};
	},
	["serviceDataUUID128"]: param => {
		return {
			type: GAP.ADType.SERVICE_DATA_UUID128,
			data: serializeServiceData(param)
		};
	},
	["appearance"]: param => {
		return {
			type: GAP.ADType.APPEARANCE,
			data: Utils.toByteArray(param, 2)
		};
	},
	["publicAddress"]: param => {
		return {
			type: GAP.ADType.PUBLIC_TARGET_ADDRESS,
			data: BluetoothAddress.getByString(param).getRawArray()
		};
	},
	["randomAddress"]: param => {
		return {
			type: GAP.ADType.RANDOM_TARGET_ADDRESS,
			data: BluetoothAddress.getByString(param).getRawArray()
		};
	},
	["advertisingInterval"]: param => {
		return {
			type: GAP.ADType.ADVERTISING_INTERVAL,
			data: Utils.toByteArray(param, 2)
		};
	},
//	["deviceAddress"]: null,
	["role"]: param => {
		return {
			type: GAP.ADType.LE_ROLE,
			data: [param & 0xFF]
		};
	},
	["uri"]: param => {
		return {
			type: GAP.ADType.URI,
			data: Utils.toCharArray(param)
		};
	}
};

const AdvertisingDataParser = {
	[GAP.ADType.INCOMPLETE_UUID16_LIST]: (data, params) => {
		params.incompleteUUID16List = parseUUIDList(2, data);
	},
	[GAP.ADType.COMPLETE_UUID16_LIST]: (data, params) => {
		params.completeUUID16List = parseUUIDList(2, data);
	},
	[GAP.ADType.INCOMPLETE_UUID128_LIST]: (data, params) => {
		params.incompleteUUID128List = parseUUIDList(16, data);
	},
	[GAP.ADType.COMPLETE_UUID128_LIST]: (data, params) => {
		params.completeUUID128List = parseUUIDList(16, data);
	},
	[GAP.ADType.SHORTENED_LOCAL_NAME]: (data, params) => {
		params.shortName = parseCharArray(data);
	},
	[GAP.ADType.COMPLETE_LOCAL_NAME]: (data, params) => {
		params.completeName = parseCharArray(data);
	},
	[GAP.ADType.FLAGS]: (data, params) => {
		params.flags = data[0] & 0xFF;
	},
	[GAP.ADType.MANUFACTURER_SPECIFIC_DATA]: (data, params) => {
		params.manufacturerSpecific = parseManufacturerSpecificData(data);
	},
	[GAP.ADType.TX_POWER_LEVEL]: (data, params) => {
		params.flags = data[0] & 0xFF;
	},
	[GAP.ADType.SLAVE_CONNECTION_INTERVAL_RANGE]: (data, params) => {
		params.connectionInterval = parseConnectionInterval(data);
	},
	[GAP.ADType.SOLICITATION_UUID16_LIST]: (data, params) => {
		params.solicitationUUID16List = parseUUIDList(2, data);
	},
	[GAP.ADType.SOLICITATION_UUID128_LIST]: (data, params) => {
		params.solicitationUUID128List = parseUUIDList(16, data);
	},
	[GAP.ADType.SERVICE_DATA_UUID16]: (data, params) => {
		params.serviceDataUUID16 = parseServiceData(2, data);
	},
	[GAP.ADType.SERVICE_DATA_UUID128]: (data, params) => {
		params.serviceDataUUID128 = parseServiceData(16, data);
	},
	[GAP.ADType.APPEARANCE]: (data, params) => {
		params.appearance = Utils.toInt16(data);
	},
	[GAP.ADType.PUBLIC_TARGET_ADDRESS]: (data, params) => {
		params.publicAddress = BluetoothAddress.getByAddress(data).toString();
	},
	[GAP.ADType.RANDOM_TARGET_ADDRESS]: (data, params) => {
		params.randomAddress = BluetoothAddress.getByAddress(data).toString();
	},
	[GAP.ADType.ADVERTISING_INTERVAL]: (data, params) => {
		params.advertisingInterval = Utils.toInt16(data);
	},
//	[GAP.ADType.LE_BLUETOOTH_DEVICE_ADDRESS]: null,
	[GAP.ADType.LE_ROLE]: (data, params) => {
		params.role = data[0] & 0xFF;
	},
	[GAP.ADType.URI]: (data, params) => {
		params.uri = parseCharArray(data);
	}
};

function toAdvertisingDataArray(structures) {
	let buffer = ByteBuffer.allocateUint8Array(GAP.MAX_AD_LENGTH, true);
	writeAdvertisingData(buffer, structures);
	buffer.flip();
	return buffer.getByteArray();
}

function writeAdvertisingData(buffer, structures) {
	for (let i = 0; i < structures.length; i++) {
		let structure = structures[i]
		if (structure.data == null) {
			break;
		}
		buffer.putInt8(structure.data.length + 1);
		buffer.putInt8(structure.type);
		buffer.putByteArray(structure.data);
	}
}

class Advertisement {
	static parse(structures) {
		let params = new Object();
		for (let structure of structures) {
			if (AdvertisingDataParser.hasOwnProperty(structure.type)) {
				AdvertisingDataParser[structure.type](structure.data, params);
			}
		}
		return params;
	}
	static serialize(params) {
		let structures = new Array();
		for (let key in params) {
			if (params.hasOwnProperty(key) && AdvertisingDataSerializer.hasOwnProperty(key)) {
				structures.push(AdvertisingDataSerializer[key](params[key]));
			}
		}
		return structures;
	}
	static toByteArray(params) {
		let structures = Advertisement.serialize(params);
		let array = toAdvertisingDataArray(structures);
		return array;
	}
}

export default Advertisement;

