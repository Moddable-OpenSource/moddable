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

class Advertisement extends ArrayBuffer {
	get(type) { return native("xs_advertisement_get").call(this, type); }
	get name() {
		const data = this.get(9) ?? this.get(8);
		return data ? String.fromArrayBuffer(data) : "";
	}
	get services() {
		const services = [];
		let s = this.get(3) ?? this.get(2);		// 16
		if (s) {
			s = new Uint8Array(s);
			for (let j = 0; j < s.byteLength; j += 2)
				services.push((s[j] + (s[j + 1] << 8)).toString(16).padStart(4, "0"));
		}

		s = this.get(5) ?? this.get(4);			// 32
		if (s) {
			s = new Uint8Array(s);
			for (let j = 0; j < s.byteLength; j += 4)
				services.push((s[j] + (s[j + 1] << 8) + (s[j + 2] << 16) + (s[j + 3] << 24)).toString(16).padStart(8, "0"));
		}

		s = this.get(7) ?? this.get(6);			// 128
		if (s) {
			s = new Uint8Array(s);
			//@@
		}

		return services.length ? services : undefined;
	}
	get manufacturerData() {
		let data = this.get(255);
		if (!data || (data.byteLength < 2)) return undefined;
		data = new Uint8Array(data);
		const manufacturer = data[0] | (data[1] << 8);
		return {manufacturer, data: data.slice(2).buffer};		
	}
}

class GAPClient extends Native("xs_gapclient_destructor") {
	constructor(options) { super(); native("xs_gapclient_build").call(this, options, Advertisement); }
	close() { return native("xs_gapclient_close").call(this); }
	read() { return native("xs_gapclient_read").call(this); }
}

const features = Object.freeze({
	constructor(options) { native("xs_gattclient_constructor").call(this, options); },
	close() { return native("xs_gattclient_close").call(this); },
	connect(request) { return native("xs_gattclient_connect").call(this, request); },
	getPrimaryServices(request, uuids) { return native("xs_gattclient_getPrimaryServices").call(this, request, uuids); },
	getCharacteristics(request, service, uuids) { return native("xs_gattclient_getCharacteristics").call(this, request, service, uuids); },
	getDescriptors(request, characteristic, uuids) { return native("xs_gattclient_getDescriptors").call(this, request, characteristic, uuids); },
	read(request, what, options) { return native("xs_gattclient_read").call(this, request, what, options); },
	readValue(what) { return native("xs_gattclient_readValue").call(this, what); },
	write(request, what, options) { return native("xs_gattclient_write").call(this, request, what, options); },
	restore(item) { return native("xs_gattclient_restore").call(this, item); },
	getCCCD(item) { return native("xs_gattclient_getCCCD").call(this, item); }
});

class GATTClientService extends Native("xs_gattclientservice_destructor") {
	constructor() {throw new Error};
	get uuid() { return native("xs_gattclientservice_get_uuid").call(this); }
}

class GATTClientCharacteristic extends Native("xs_gattclientcharacteristic_destructor") {
	constructor() {throw new Error}
	get uuid() { return native("xs_gattclientcharacteristic_get_uuid").call(this); }
	get handle() { return native("xs_gattclientcharacteristic_get_handle").call(this); }
	get properties() { return native("xs_gattclientcharacteristic_get_properties").call(this); }
}

class GATTClientDescriptor extends Native("xs_gattclientdescriptor_destructor") {
	constructor() {throw new Error};
	get uuid() { return native("xs_gattclientdescriptor_get_uuid").call(this); }
}

class GATTClient extends Native("xs_gattclient_destructor") {
	#requests = [];

	static #Request = class {
		constructor(client, feature, callback, ...params) {
			this.client = client;
			this.feature = feature;
			this.callback = callback;
			this.params = params;
			const requests = client.#requests;
			if (requests.closing)
				throw new Error("closing");			
			requests.push(this);
			if (requests.length == 1)
				this.execute();
		}
		execute() {
			this.feature.call(this.client, this, ...this.params);
		}
		executed(error, result) {
			this.callback?.call(this.client, error, result);
			const requests = this.client.#requests;
			const check = requests.shift();
			if (check != this)
				debugger;
			if (requests.closing) {
				while (requests.length > 1)
					requests.shift().callback?.(new Error("canceled"));
			}
			if (requests.length > 0)
				requests.at(0).execute();
		}
	};

	constructor(options) {
		super();

		if (options.target)
			this.target = options.target;

		features.constructor.call(this, options);

		const {onReady, onError} = options;
		new GATTClient.#Request(this, features.connect, error => {
			if (error)
				onError?.call(this, error);
			else
				onReady?.call(this);
		});
	}
	close(callback) {
		if (this.#requests.closing) return;
		features.close.call(this);
		new GATTClient.#Request(this, features.close, callback);
		this.#requests.closing = true;
	}
	getPrimaryServices(options, callback = options) {
		new GATTClient.#Request(this, features.getPrimaryServices, callback, options === callback ? null : options, GATTClientService.prototype);
	}
	getCharacteristics(service, options, callback = options) {
		new GATTClient.#Request(this, features.getCharacteristics, callback, service, options === callback ? null : options, GATTClientCharacteristic.prototype);
	}
	getDescriptors(characteristic, options, callback = options) {
		new GATTClient.#Request(this, features.getDescriptors, callback, characteristic, options === callback ? null : options, GATTClientDescriptor.prototype);
	}
	read(what, options, callback = options) {
		if (!callback)
			return features.readValue.call(this, what);
		new GATTClient.#Request(this, features.read, callback, what, options === callback ? null : options);
	}
	write(what, value, options, callback = options) {
		new GATTClient.#Request(this, features.write, callback, what, value, options === callback ? null : options);
	}
	subscribe(characteristic, callback) {
		return this.#enableNotifications(characteristic, true, callback);
	}
	unsubscribe(characteristic, callback) {
		return this.#enableNotifications(characteristic, false, callback);
	}
	replyToPasskey(action, value) { return native("xs_gattclient_replyToPasskey").call(this, action, value); }
	#enableNotifications(characteristic, enable, callback) {
		function write(gatt, characteristic, descriptor, enable, callback) {
			let flag = 0;
			if (enable) {
				const properties = characteristic.properties;
				if (properties & GATTClient.properties.notify)		// prefer notifications to indications, when both available
					flag = 1;
				else if (properties & GATTClient.properties.indicate)
					flag = 2;
				else
					return callback?.call(gatt, new Error("no notify or indicate"));
			}
			gatt.write(descriptor, Uint8Array.of(flag, 0), callback);
		}
		const descriptor = features.getCCCD(characteristic, GATTClientDescriptor.prototype);
		if (descriptor)
			return write(this, characteristic, descriptor, enable, callback);
		
		this.getDescriptors(characteristic, ["2902"], (error, descriptors) => {
			if (error)
				return callback?.call(this, error);
			write(this, characteristic, descriptors[0], enable, callback);
			this.#requests.splice(1, 0, this.#requests.pop());		// go to the head of the line so enableNotifications fully completes before next operation in queue
		});
	}

	get maximumWrite() { return native("xs_gattclient_get_maximumWrite").call(this); }

	store(item) { return native("xs_gattclient_store").call(this, item); }
	restore(item) {
		return features.restore.call(this, item, GATTClientService.prototype, GATTClientCharacteristic.prototype, GATTClientDescriptor.prototype);
	}

	static properties = Object.freeze({
		authenticatedSignedWrites: (1 << 6),
		broadcast: (1 << 0),
		indicate: (1 << 5),
		notify: (1 << 4),
		read: (1 << 1),
		reliableWrite: (1 << 7),
		write: (1 << 3),
		writeWithOutResponse: (1 << 2),
	});
};

export {GATTClient, GAPClient}
