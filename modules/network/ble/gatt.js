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
import {UUID} from "btutils";

export class Client {
	constructor(dictionary) {
		for (let property in dictionary) {
			switch (property) {
				case "ble":
					this.ble = dictionary.ble;
					break;
				case "connection":
					this.connection = dictionary.connection;
					break;
				default:
					throw new Error(`invalid property "${property}`);
					break;
			}
		}
		this.services = [];
	}
	
	close() {
		this._disconnect(this.connection);
	}

	discoverPrimaryService(uuid) {
		this.services.length = 0;
		this._discoverPrimaryServices(this.connection, UUID.toBuffer(uuid));
	}

	discoverAllPrimaryServices() {
		this.services.length = 0;
		this._discoverPrimaryServices(this.connection);
	}
	
	onServices() {
	}

	_discoverPrimaryServices() @ "xs_gatt_client_discover_primary_services"

	_disconnect() @ "xs_gap_connection_disconnect"
	
	_findCharacteristicByHandle(handle) {
		let service = this.services.find(service => (handle >= service.start && handle <= service.end));
		if (service) {
			let characteristic = service.characteristics.find(characteristic => handle == characteristic.handle);
			return characteristic;
		}
	}
	
	_onService(params) {
		if (!params)
			this.ble.onServices(this.services);
		else {
			params.uuid = UUID.toString(params.uuid);
			params.connection = this.connection;
			params.ble = this.ble;
			this.services.push(new Service(params));
		}
	}
	
	_onCharacteristicNotification(params) {
		let characteristic = this._findCharacteristicByHandle(params.handle);
		if (characteristic)
			characteristic._onNotification.call(characteristic, params);
	}

	_onCharacteristicValue(params) {
		let characteristic = this._findCharacteristicByHandle(params.handle);
		if (characteristic)
			characteristic._onValue.call(characteristic, params);
	}

	callback(event, params) {
		//trace(`Client callback ${event}\n`);
		this[event](params);
	}
};
Object.freeze(Client.prototype);

export class Service {
	constructor(dictionary) {
		this.characteristics = [];
		
		for (let property in dictionary) {
			switch (property) {
				case "ble":
					this.ble = dictionary.ble;
					break;
				case "connection":
					this.connection = dictionary.connection;
					break;
				case "uuid":
					this.uuid = dictionary.uuid;
					break;
				case "start":
					this.start = dictionary.start;
					break;
				case "end":
					this.end = dictionary.end;
					break;
				default:
					throw new Error(`invalid property "${property}`);
					break;
			}
		}
	}
	
	discoverCharacteristic(uuid) {
		this.characteristics.length = 0;
		this._discoverCharacteristics(this.connection, this.start, this.end, UUID.toBuffer(uuid));
	}

	discoverAllCharacteristics() {
		this.characteristics.length = 0;
		this._discoverCharacteristics(this.connection, this.start, this.end);
	}
	
	findCharacteristicByUUID(uuid) {
		return this.characteristics.find(characteristic => uuid == characteristic.uuid);
	}

	_onCharacteristic(params) {
		if (!params)
			this.ble.onCharacteristics(this.characteristics);
		else {
			params.uuid = UUID.toString(params.uuid);
			params.connection = this.connection;
			params.service = this;
			params.ble = this.ble;
			this.characteristics.push(new Characteristic(params));
		}
	}
	
	_discoverCharacteristics() @ "xs_gatt_service_discover_characteristics"

	callback(event, params) {
		//trace(`Service callback ${event}\n`);
		this[event](params);
	}
};
Object.freeze(Service.prototype);

export class Characteristic {
	constructor(dictionary) {
		for (let property in dictionary) {
			switch (property) {
				case "ble":
					this.ble = dictionary.ble;
					break;
				case "service":
					this.service = dictionary.service;
					break;
				case "connection":
					this.connection = dictionary.connection;
					break;
				case "uuid":
					this.uuid = dictionary.uuid;
					break;
				case "handle":
					this.handle = dictionary.handle;
					break;
				case "properties":
					this.properties = dictionary.properties;
					break;
				default:
					throw new Error(`invalid property "${property}`);
					break;
			}
		}
		this.descriptors = [];
	}
	
	discoverAllDescriptors() {
		this.descriptors.length = 0;
		this._discoverAllDescriptors(this.connection, this.handle);
	}
	
	disableNotifications() {
		this._disableNotifications(this.connection, this.handle);
	}
	
	enableNotifications() {
		this._enableNotifications(this.connection, this.handle);
	}
	
	readValue() {
		this._readValue(this.connection, this.handle);
	}

	writeWithoutResponse(value) {
		this._writeWithoutResponse(this.connection, this.handle, value);
	}
	
	_onDescriptor(params) {
		if (!params)
			this.onDescriptors(this.descriptors);
		else {
			params.uuid = UUID.toString(params.uuid);
			params.connection = this.connection;
			params.characteristic = this;
			this.descriptors.push(new Descriptor(params));
		}
	}
	_onNotification(params) {
		this.ble.onCharacteristicNotification(this, params.value);
	}
	_onValue(params) {
		this.ble.onCharacteristicValue(this, params.value);
	}
	
	_discoverAllDescriptors() @ "xs_gatt_characteristic_discover_all_characteristic_descriptors"
	_readValue() @ "xs_gatt_characteristic_read_value"
	_writeWithoutResponse() @ "xs_gatt_characteristic_write_without_response"
	_disableNotifications() @ "xs_gatt_characteristic_disable_notifications"
	_enableNotifications() @ "xs_gatt_characteristic_enable_notifications"

	callback(event, params) {
		//trace(`Characteristic callback ${event}\n`);
		this[event](params);
	}
};
Object.freeze(Characteristic.prototype);

export class Descriptor {
	constructor(dictionary) {
		for (let property in dictionary) {
			switch (property) {
				case "ble":
					this.ble = dictionary.ble;
					break;
				case "connection":
					this.connection = dictionary.connection;
					break;
				case "uuid":
					this.uuid = dictionary.uuid;
					break;
				case "handle":
					this.handle = dictionary.handle;
					break;
				case "characteristic":
					this.characteristic = dictionary.characteristic;
					break;
				default:
					throw new Error(`invalid property "${property}`);
					break;
			}
		}
	}
	
	writeValue(value) {
		this._writeValue(this.connection, this.handle, value);
	}
	
	_writeValue() @ "xs_gatt_descriptor_write_value"
};
Object.freeze(Descriptor.prototype);

export default {
	Client,
	Service,
	Characteristic,
	Descriptor
};
