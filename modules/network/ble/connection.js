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
		this.onDisconnected = function() {};
		
		for (let property in dictionary) {
			switch (property) {
				case "client":
					this.client = dictionary.client;
					break;
				case "address":
					this.address = dictionary.address;
					break;
			}
		}
		this.initialize(this.client);
	}
	initialize(params) @ "xs_gap_connection_initialize"
	
	disconnect() {
		this._disconnect(this.client.connection);
	}
	
	readRSSI() {
		this._readRSSI(this.client.connection);
	}
	
	_disconnect() @ "xs_gap_connection_disconnect"
	_readRSSI() @ "xs_gap_connection_read_rssi"
	
	callback(event, params) {
		this[event](params);
	}
};

export default Connection;
