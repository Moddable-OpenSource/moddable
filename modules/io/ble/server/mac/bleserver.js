/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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

class GATTServerConnection extends Native("BLEServerConnection_destructor") {
	constructor() {throw new Error};
	close() { return native("BLEServerConnection_close").call(this); }
	notify(characteristic, value) { return native("BLEServerConnection_notify").call(this, characteristic, value); }
}

class GATTServerCharacteristic extends Native("BLEServerCharacteristic_destructor") {
	constructor() {throw new Error};
}

class GATTServer extends Native("BLEServer_destructor") {
	constructor(options) {
		super();
		native("BLEServer_build").call(this, options, GATTServerConnection.prototype, GATTServerCharacteristic.prototype);
		const services = options.services;
		for (let i = 0; i < services.length; i++) {
			const service = services[i];
			const uuid = service.uuid;
			if (uuid == "1800") {
				options.onWarning?.("no 1800 service on macOS");
				continue;
			}
			if (uuid == "180f") {
				options.onWarning?.("no 180f service on macOS");
				continue;
			}
			this.addService(services[i]);
		}
	}
	close() { return native("BLEServer_close").call(this); }

	addService(service) { return native("BLEServer_addService").call(this, service); }
	deleteService(service) { return native("BLEServer_deleteService").call(this, service); }
	
	startAdvertising(ad) { return native("BLEServer_startAdvertising").call(this, ad); }
	stopAdvertising() { return native("BLEServer_stopAdvertising").call(this); }

	static properties = Object.freeze({
		broadcast: (1 << 0),
		read: (1 << 1),
		writeWithOutResponse: (1 << 2),
		write: (1 << 3),
		notify: (1 << 4),
		indicate: (1 << 5),
		authenticatedSignedWrites: (1 << 6),
		reliableWrite: (1 << 7),
		
		notifyEncrypted: (1 << 8),
		notifyAuthenticated: (1 << 8), // same as notifyEncrypted on macOS
		
		subscribeEncrypted: (1 << 9),
		subscribeAuthenticated: (1 << 9), // same as indicateEncrypted on macOS
		
		readEncrypted: (1 << 10) | (1 << 1),
		readAuthenticated: (1 << 10) | (1 << 1), // same as readEncrypted on macOS
		
		writeEncrypted: (1 << 11) | (1 << 3),
		writeAuthenticated: (1 << 11) | (1 << 3), // same as writeEncrypted on macOS
	});
}

export {GATTServer}
