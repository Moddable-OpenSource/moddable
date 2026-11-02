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
	Simulated ECMA-419 Wi-Fi Access Point Network Interface.

	The access point is simulated: nothing here touches the host computer's Wi-Fi
	radio, and no device can really associate with it. One station is simulated a
	couple of seconds after the access point comes up, so that onConnect and the
	Station class can be exercised.

	The IP and MAC addresses reported are the host computer's, because a project that
	brings up an access point usually goes on to serve something at that address, and
	a server started here really is reachable there.
*/

import Timer from "timer";

const kStartDelay = 250;			// delay from the constructor to running (400)
const kStationDelay = 2000;			// delay from running to the simulated station joining
const kStationAddressDelay = 250;	// delay from joining to the station's address being assigned

const kMinPasswordLength = 8;
const kMaxChannel = 13;
const kMaxStations = 10;
const kMinInterval = 100;
const kMaxInterval = 60000;

const kStationMAC = "b8:27:eb:41:9c:07";
const kStationHost = "200";			// last octet of the simulated station's address

const authentications = Object.freeze(["none", "wpa2_psk", "wpa_wpa2_psk", "wpa3_psk", "wpa2_wpa3_psk"]);

// one radio, so one access point, as on a device
const state = {
	instance: undefined,
	generation: 0					// invalidates pending transitions when the access point closes
};

// the host computer's network interface, from modules/embedded/network/interface/wifi/sim/hostinterface.c
function hostAddress() { return native("xs_wifisim_address").call(this); }
function hostMAC() { return native("xs_wifisim_MAC").call(this); }

/*
	A station's mutable state lives in a record the access point creates and both share,
	because a class cannot reach another class's private fields. The record's close() is
	a closure back into the access point.
*/
class Station {
	#record;

	constructor(record) {
		if (!record?.close)
			throw new Error("no constructor");
		this.#record = record;
	}
	close() {
		if (!this.#record.closed)
			this.#record.close();
	}
	get MAC() {
		this.#validate();
		return this.#record.MAC;
	}
	get address() {
		this.#validate();
		return this.#record.address;
	}

	#validate() {
		if (this.#record.closed)
			throw new Error("closed");
	}

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

class WiFiAccessPoint {
	#onChanged;
	#onConnect;
	#onDisconnect;
	#connection = 200;
	#SSID;
	#channel;
	#station;					// record of the connected station, if any
	#timer;
	#closed;

	constructor(options) {
		if (state.instance)
			throw new Error("access point already open");

		const requested = options.SSID;
		if (undefined === requested)
			throw new Error("SSID required");
		const SSID = String(requested);
		if (!SSID.length)
			throw new RangeError("SSID empty");

		let password = options.password;
		if (undefined !== password) {
			password = String(password);
			if (!password.length)
				password = undefined;
			else if (password.length < kMinPasswordLength)
				throw new RangeError("password too short - 8 bytes min");
		}

		const authentication = options.authentication;
		if (undefined !== authentication) {
			const value = String(authentication);
			if (!authentications.includes(value))
				throw new RangeError("invalid authentication");
			if (("none" !== value) && (undefined === password))
				throw new Error("password required for authentication mode");
		}

		let channel = options.channel;
		if (undefined === channel)
			channel = 1;			// the host picks one; a device reports the channel in use
		else {
			channel = Number(channel);
			if (!((channel >= 1) && (channel <= kMaxChannel)))
				throw new RangeError("invalid channel");
		}

		const max = options.max;
		if (undefined !== max) {
			const value = Number(max);
			if (!((value >= 1) && (value <= kMaxStations)))
				throw new RangeError("invalid max");
		}

		const interval = options.interval;
		if (undefined !== interval) {
			const value = Number(interval);
			if (!((value >= kMinInterval) && (value <= kMaxInterval)))
				throw new RangeError("invalid interval");
		}

		for (let name of ["onChanged", "onConnect", "onDisconnect"]) {
			const callback = options[name];
			if ((undefined !== callback) && ("function" !== typeof callback))
				throw new Error(`invalid ${name}`);
		}
		this.#onChanged = options.onChanged;
		this.#onConnect = options.onConnect;
		this.#onDisconnect = options.onDisconnect;

		this.#SSID = SSID;
		this.#channel = channel;

		state.instance = this;
		state.generation += 1;

		const current = state.generation;
		this.#timer = Timer.set(() => {
			this.#timer = undefined;
			if (current !== state.generation)
				return;

			this.#connection = 400;
			this.#onChanged?.call(this, "connection");
			if (current !== state.generation)		// closed from the callback
				return;

			this.#scheduleStation(current);
		}, kStartDelay);
	}
	close() {
		if (this.#closed)
			return;

		this.#closed = true;
		state.generation += 1;						// abandons anything still pending
		if (state.instance === this)
			state.instance = undefined;

		Timer.clear(this.#timer);
		this.#timer = undefined;

		// stations are dropped without notification, as when the radio stops
		if (this.#station)
			this.#station.closed = true;
		this.#station = undefined;
		this.#connection = 200;
	}
	configure(options) {
		this.#validate();

		const portal = options.portal;
		if (undefined !== portal)
			String(portal);			// read and validated, but unused: there is no DHCP server here
	}

	get connection() {
		this.#validate();
		return this.#connection;
	}
	get address() {
		this.#validate();
		return (this.#connection >= 400) ? hostAddress() : undefined;
	}
	get MAC() {
		this.#validate();
		return hostMAC();
	}
	get SSID() {
		this.#validate();
		return this.#SSID;
	}
	get channel() {
		this.#validate();
		return this.#channel;
	}

	#validate() {
		if (this.#closed)
			throw new Error("closed");
	}
	// one station joins a couple of seconds after the access point is running, and is
	// assigned an address a moment later, as a real station is by DHCP
	#scheduleStation(current) {
		this.#timer = Timer.set(() => {
			this.#timer = undefined;
			if (current !== state.generation)
				return;

			const record = {
				MAC: kStationMAC,
				address: undefined,
				closed: false,
				close: () => this.#disconnect(record)
			};
			record.station = new Station(record);

			this.#station = record;
			this.#onConnect?.call(this, record.station);
			if ((current !== state.generation) || (this.#station !== record))
				return;

			this.#timer = Timer.set(() => {
				this.#timer = undefined;
				if ((current !== state.generation) || (this.#station !== record))
					return;

				record.address = stationAddress();
			}, kStationAddressDelay);
		}, kStationDelay);
	}
	// a station leaving is reported asynchronously, as the radio event would be
	#disconnect(record) {
		if (this.#station !== record)
			return;

		this.#station = undefined;
		Timer.set(() => {
			if (this.#closed) {
				record.closed = true;
				return;
			}

			this.#onDisconnect?.call(this, record.station);
			record.closed = true;			// the station is usable during onDisconnect, then detached
		});
	}

	static {
		this.prototype[Symbol.dispose] = this.prototype.close;
	}
}

// the simulated station shares the host's subnet, so its address reads sensibly next
// to the access point's
function stationAddress() {
	const address = hostAddress();
	if (undefined === address)
		return;

	const octets = address.split(".");
	if (4 !== octets.length)
		return;

	octets[3] = kStationHost;
	return octets.join(".");
}

export default WiFiAccessPoint;
