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
	constructor(connection) {
		this.onServices = function() {};
		this.services = [];
		
		this.connection = connection;
		this.initialize(connection);
	}
	initialize() @ "xs_gatt_client_initialize"
	
	discoverAllPrimaryServices() {
		this.services = [];
		this._discoverAllPrimaryServices(this.connection);
	}
	
	_discoverAllPrimaryServices() @ "xs_gatt_client_discover_all_primary_services"

	_onService(params) {
		if (!params)
			this.onServices(this.services);
		else {
			params.uuid = UUID.getByUUID(new Uint8Array(params.uuid)).toString();
			params.connection = this.connection;
			this.services.push(new Service(params));
		}
	}

	callback(event, params) {
		this[event](params);
	}
};

export class Service {
	constructor(dictionary) {
		this.onCharacteristics = function() {};
		this.characteristics = [];
		
		for (let property in dictionary) {
			switch (property) {
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
			}
		}
	}
	
	discoverAllCharacteristics() {
		this.characteristics = [];
		this._discoverAllCharacteristics(this.connection, this.start, this.end);
	}
	
	_onCharacteristic(params) {
		if (!params)
			this.onCharacteristics(this.characteristics);
		else {
			params.uuid = UUID.getByUUID(new Uint8Array(params.uuid)).toString();
			params.connection = this.connection;
			this.characteristics.push(new Characteristic(params));
		}
	}
	
	_discoverAllCharacteristics() @ "xs_gatt_service_discover_all_characteristics"

	callback(event, params) {
		this[event](params);
	}
};

export class Characteristic {
	constructor(dictionary) {
		this.onDescriptors = function() {};
		this.descriptors = [];
		
		for (let property in dictionary) {
			switch (property) {
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
			}
		}
	}
	
	discoverAllCharacteristicDescriptors() {
		this.descriptors = [];
		this._discoverAllCharacteristicDescriptors(this.connection, this.start, this.end);
	}
	
	_onDescriptor(params) {
		if (!params)
			this.onDescriptors(this.descriptors);
		else {
			params.uuid = UUID.getByUUID(new Uint8Array(params.uuid)).toString();
			params.connection = this.connection;
			this.descriptors.push(new Descriptor(params));
		}
	}
	
	_discoverAllCharacteristicDescriptors() @ "xs_gatt_characteristic_discover_all_characteristic_descriptors"

	callback(event, params) {
		this[event](params);
	}
};

export class Descriptor {
	constructor(dictionary) {
		for (let property in dictionary) {
			switch (property) {
				case "connection":
					this.connection = dictionary.connection;
					break;
				case "uuid":
					this.uuid = dictionary.uuid;
					break;
				case "handle":
					this.handle = dictionary.handle;
					break;
			}
		}
	}
};

export default {
	Client,
	Service,
	Characteristic,
	Descriptor
};
