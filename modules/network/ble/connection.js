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

export class Connection {
	constructor(dictionary) {
		for (let property in dictionary) {
			switch (property) {
				case "ble":
					this.ble = dictionary.ble;
					break;
				case "client":
					this.client = dictionary.client;
					break;
				case "address":
					this.address = dictionary.address;
					break;
				case "addressType":
					this.addressType = dictionary.addressType;
					break;
				default:
					throw new Error(`invalid property "${property}`);
					break;
			}
		}
		this.initialize(this.client);
	}
	initialize(params) @ "xs_gap_connection_initialize"
	
	disconnect() {
		this._disconnect(this.client.connection);
	}
	
	_disconnect() @ "xs_gap_connection_disconnect"
	
	callback(event, params) {
		//trace(`Connection callback ${event}\n`);
		switch(event) {
			case "onDisconnected":
				this.ble.onDisconnected(this.client);
				break;
			case "onRSSI":
				this.ble.onRSSI(this.client, params);
				break;
			case "onPasskeyConfirm":
				this.ble.onPasskeyConfirm(this.client, params);
				break;
			case "onPasskeyDisplay":
				this.ble.onPasskeyDisplay(this.client, params);
				break;
			case "onPasskeyRequested":
				return this.ble.onPasskeyRequested(this.client, params);
				break;
			case "onAuthenticated":
				return this.ble.onAuthenticated(this.client);
				break;
			case "onMTUExchanged":
				this.ble.onMTUExchanged(this.client, params);
				break;
		}
	}
};
Object.freeze(Connection.prototype);

export default Connection;
