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

export class Client {
	constructor(dictionary) {
		this.onServices = function() {};
		
		for (let property in dictionary) {
			switch (property) {
				case "iface":
					this.iface = dictionary.iface;
					break;
				case "connection":
					this.connection = dictionary.connection;
					break;
			}
		}
	}
	
	discoverAllPrimaryServices() {
		this.services = [];
		this._discoverAllPrimaryServices(this.iface, this.connection);
	}
	
	_discoverAllPrimaryServices() @ "xs_ble_gatt_client_discover_all_primary_services"

	_onService(service) {
		if (!service)
			this.onServices(this.services);
		else
			this.services.push(service);
	}

	callback(event, params) {
		this[event](params);
	}
};

export class Service {
	constructor(dictionary) {
	}
	
	callback(event, params) {
		//this[event](params);
	}
};

export default {
	Client,
	Service
};
