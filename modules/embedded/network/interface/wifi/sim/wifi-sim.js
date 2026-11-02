/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

/*
	Simulated ECMA-419 Wi-Fi Network Interface.

	Scans find a fixed list of access points. Connecting succeeds only for an SSID
	in that list, so applications can exercise both the success and failure paths
	without hardware. Nothing here touches the host computer's Wi-Fi radio.

	The IP and MAC addresses reported are the host computer's, because code that
	binds to, advertises, or displays the address needs one that actually works.
*/

import Timer from "timer";

const kScanFirst = 100;			// delay to the first scan result
const kScanInterval = 40;		// delay between scan results
const kConnectDelay = 250;		// delay from connect() to connected (400) or failed (200)
const kAddressDelay = 250;		// delay from connected (400) to address assigned (500)

const kMinPasswordLength = 8;
const kMaxPasswordLength = 63;
const kMaxHostnameLength = 63;	// RFC 1035 label limit

const kFailPassword = "fail";	// reserved password that always fails to authenticate

// The simulated radio, shared by every instance in the way one radio is shared on a
// device. One mutable object rather than several mutable variables, so that preloading
// this module aliases one object into RAM instead of one per value.
const state = {
	instances: undefined,		// instances that are open
	connection: 200,			// 200 disconnected, 300 connecting, 400 connected, 500 address assigned
	accessPoint: undefined,		// access point of the current connection
	address: undefined,			// IP address, once assigned
	staticAddress: undefined,	// static IP configuration, if any
	scanner: undefined,			// instance with a scan in progress
	scanTimer: undefined,
	connectTimer: undefined,
	generation: 0				// invalidates pending transitions when the connection changes
};

// the host computer's network interface, from modules/embedded/network/interface/wifi/sim/hostinterface.c
function hostAddress() { return native("xs_wifisim_address").call(this); }
function hostMAC() { return native("xs_wifisim_MAC").call(this); }

function isIPv4(value) {
	const parts = String(value).split(".");
	if (4 !== parts.length)
		return false;

	return parts.every(part => {
		if (!/^\d{1,3}$/.test(part))
			return false;
		return Number(part) <= 255;
	});
}

function toIPv4(value, what) {
	if (!isIPv4(value))
		throw new RangeError(`invalid ${what}`);
	return String(value);
}

class WiFi {
	#onChanged;
	#connection;		// state last reported to this instance
	#address;
	#closed;

	constructor(options) {
		const onChanged = options.onChanged;
		if ((undefined !== onChanged) && ("function" !== typeof onChanged))
			throw new Error("invalid onChanged");

		this.#onChanged = onChanged;
		this.#connection = state.connection;
		this.#address = state.address;

		state.instances ??= [];
		state.instances.push(this);
	}
	close() {
		if (this.#closed)
			return;

		this.#closed = true;
		this.#onChanged = undefined;

		const index = state.instances.indexOf(this);
		if (index >= 0)
			state.instances.splice(index, 1);

		if (state.scanner === this) {
			Timer.clear(state.scanTimer);
			state.scanTimer = state.scanner = undefined;
		}
	}
	scan(options) {
		this.#validate();

		if (state.scanner)
			throw new Error("already scanning");

		const onFound = options.onFound;
		if ("function" !== typeof onFound)
			throw new Error("onFound required");

		const onComplete = options.onComplete;
		if ((undefined !== onComplete) && ("function" !== typeof onComplete))
			throw new Error("invalid onComplete");

		const channel = options.channel;
		let found = accessPoints;
		if (undefined !== channel) {
			const value = Number(channel);
			found = found.filter(ap => value === ap.channel);
		}
		found = found.slice();

		state.scanner = this;
		state.scanTimer = Timer.set(() => {
			if (found.length) {
				onFound.call(this, {...found.shift()});
				return;
			}

			Timer.clear(state.scanTimer);
			state.scanTimer = state.scanner = undefined;
			onComplete?.call(this);
		}, kScanFirst, kScanInterval);
	}
	connect(options) {
		this.#validate();

		if (300 === state.connection)
			throw new Error("already connecting");

		const requested = options.SSID;
		if (undefined === requested)
			throw new Error("SSID required");
		const SSID = String(requested);

		const password = options.password;
		const secret = (undefined === password) ? undefined : String(password);

		let channel = options.channel;
		if (undefined !== channel) {
			channel = Number(channel);
			if (!((channel >= 1) && (channel <= 13)))
				throw new RangeError("invalid channel");
		}

		// the strongest access point with a matching name, as a device selects one
		let ap;
		for (let candidate of accessPoints) {
			if (SSID !== candidate.SSID)
				continue;
			if ((undefined !== channel) && (channel !== candidate.channel))
				continue;
			if (!ap || (candidate.RSSI > ap.RSSI))
				ap = candidate;
		}

		let success = undefined !== ap;
		if (success && ("none" !== ap.security))
			success = (undefined !== secret) && (secret.length >= kMinPasswordLength) && (secret.length <= kMaxPasswordLength);
		if (kFailPassword === secret)
			success = false;		// the reserved password, so projects can test authentication failure

		WiFi.#reset();
		state.connection = 300;
		state.accessPoint = success ? ap : undefined;
		for (let instance of state.instances)
			instance.#sync();		// no notification: connecting is the caller's own doing

		const current = state.generation;
		state.connectTimer = Timer.set(() => {
			state.connectTimer = undefined;
			if (current !== state.generation)
				return;

			if (!success) {
				state.connection = 200;
				WiFi.#broadcast();
				return;
			}

			state.connection = 400;
			WiFi.#broadcast();
			WiFi.#scheduleAddress();
		}, kConnectDelay);
	}
	disconnect() {
		this.#validate();

		WiFi.#reset();
		state.connection = 200;
		this.#sync();				// the caller sees the change immediately, as on a device
		Timer.set(() => WiFi.#broadcast());
	}
	configure(options) {
		this.#validate();

		const name = options.hostname;
		if (undefined !== name) {
			const value = String(name);
			if (!value.length || (value.length > kMaxHostnameLength))
				throw new Error("invalid hostname");
		}

		const configuration = options.static;
		if (undefined === configuration)
			return;

		if (configuration) {
			state.staticAddress = {
				address: toIPv4(configuration.address, "address"),
				mask: toIPv4(configuration.mask, "mask"),
				gateway: toIPv4(configuration.gateway, "gateway")
			};

			if (state.connection >= 400) {
				state.address = state.staticAddress.address;
				state.connection = 500;
				Timer.set(() => WiFi.#broadcast());
			}
		}
		else {
			state.staticAddress = undefined;

			if (state.connection >= 400) {		// drop the address and acquire a new one, as DHCP would
				state.address = undefined;
				state.connection = 400;
				Timer.set(() => WiFi.#broadcast());
				WiFi.#scheduleAddress();
			}
		}
	}

	get connection() {
		this.#validate();
		return state.connection;
	}
	get address() {
		this.#validate();
		return (state.connection >= 500) ? state.address : undefined;
	}
	get MAC() {
		this.#validate();
		return hostMAC();
	}
	get SSID() {
		this.#validate();
		return (state.connection >= 400) ? state.accessPoint?.SSID : undefined;
	}
	get BSSID() {
		this.#validate();
		return (state.connection >= 400) ? state.accessPoint?.BSSID : undefined;
	}
	get RSSI() {
		this.#validate();
		return (state.connection >= 400) ? state.accessPoint?.RSSI : undefined;
	}
	get channel() {
		this.#validate();
		return (state.connection >= 400) ? state.accessPoint?.channel : undefined;
	}

	#validate() {
		if (this.#closed)
			throw new Error("closed");
	}
	#sync() {
		this.#connection = state.connection;
		this.#address = state.address;
	}
	#notify() {
		if (this.#closed)
			return;

		const onChanged = this.#onChanged;
		const connectionChanged = state.connection !== this.#connection;
		const addressChanged = (undefined !== state.address) && (state.address !== this.#address);
		this.#connection = state.connection;
		this.#address = state.address;

		if (connectionChanged)
			onChanged?.call(this, "connection");
		if (addressChanged && !this.#closed)
			onChanged?.call(this, "address");
	}

	// abandons any transition in progress and returns the radio to disconnected
	static #reset() {
		state.generation += 1;
		Timer.clear(state.connectTimer);
		state.connectTimer = undefined;
		state.accessPoint = undefined;
		state.address = undefined;
		state.connection = 200;
	}
	static #scheduleAddress() {
		const current = state.generation;
		state.connectTimer = Timer.set(() => {
			state.connectTimer = undefined;
			if ((current !== state.generation) || (400 !== state.connection))
				return;

			// the host has no network: connected to Wi-Fi but no address, as when DHCP fails
			state.address = state.staticAddress?.address ?? hostAddress();
			if (undefined === state.address)
				return;

			state.connection = 500;
			WiFi.#broadcast();
		}, kAddressDelay);
	}
	static #broadcast() {
		for (let instance of state.instances.slice())		// a callback may close instances
			instance.#notify();
	}

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

export default WiFi;

const accessPoints = [
	{
		SSID: "Moddable Guest",
		BSSID: "ba:27:eb:1f:04:2a",
		RSSI: -47,
		channel: 1,
		security: "wpa2_psk"
	},
	{
		SSID: "468 8th - slow",
		BSSID: "ba:27:eb:6c:19:73",
		RSSI: -84,
		channel: 3,
		security: "wpa2_psk"
	},
	{
		SSID: "JAMS2",
		BSSID: "3c:37:86:5b:e0:11",
		RSSI: -72,
		channel: 6,
		security: "wpa2_psk"
	},
	{
		SSID: "xfinitywifi",
		BSSID: "3c:37:86:5b:e0:12",
		RSSI: -79,
		channel: 6,
		security: "none"
	},
	{
		SSID: "baja",
		BSSID: "5c:e2:8c:37:aa:01",
		RSSI: -81,
		channel: 8,
		security: "wpa2_psk"
	},
	{
		SSID: "baja",
		BSSID: "5c:e2:8c:37:aa:02",
		RSSI: -88,
		channel: 8,
		security: "wpa2_psk"
	},
	{
		SSID: "Breathe",
		BSSID: "f0:9f:c2:14:8d:60",
		RSSI: -85,
		channel: 9,
		security: "wpa3_psk"
	},
	{
		SSID: "Breathe-Guest",
		BSSID: "f0:9f:c2:14:8d:61",
		RSSI: -84,
		channel: 9,
		security: "none"
	},
	{
		SSID: "468 8th - slow",
		BSSID: "ba:27:eb:6c:19:74",
		RSSI: -23,
		channel: 10,
		security: "wpa2_psk"
	},
	{
		SSID: "HOME-608A_2GEXT",
		BSSID: "44:32:c8:79:2b:16",
		RSSI: -86,
		channel: 11,
		security: "wpa_wpa2_psk"
	},
	{
		SSID: "LBBSB",
		BSSID: "44:32:c8:79:2b:17",
		RSSI: -85,
		channel: 11,
		security: "wpa2_psk"
	}
];
Object.freeze(accessPoints, true);
