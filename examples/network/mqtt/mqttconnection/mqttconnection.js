/*
* Copyright (c) 2021-2022  Moddable Tech, Inc.
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
	Connection is designed to be used in place of the "mqtt" module in projects
	that need a continuously available mqtt connection.

		- tries to reconnect on connection dropped (0.5 second delay)
		- 10 second timeout on establishing connection (until onReady)
		- call wait(true) if no network connection to turn off reconnect attempts and wait(true) to restart
		- onConnected and onReady called on each reconnection
		- onClose may be called more than once between connections
		- subscriptions are retained and automatically resubscribed on reconnect. set options.subscribe to false to disable. 

	To do:

		- if needed, could make connection timeout (10 seconds) configurable
		- if needed, could make reconnect interval (500 ms) configurable
*/

import MQTT from "mqtt";
import Timer from "timer";

class Connection {
	#options;
	#mqtt;
	#ready = false;
	#closed = false;	// client called closed -- don't reconnect
	#reconnect;
	#timeout;
	#wait = false;		// if true, will not attempt to reconnect
	#subscriptions;

	constructor(options) {
		this.#options = {...options};
		if (false !== this.#options.subscribe)
			this.#subscriptions = [];

		this.#restart();
	}
	close() {
		const mqtt = this.#mqtt;
		this.#mqtt = undefined;
		this.#ready = false;
		this.#closed = true;

		Timer.clear(this.#reconnect);
		this.#reconnect = undefined;

		Timer.clear(this.#timeout);
		this.#timeout = undefined;

		mqtt?.close();
	}
	publish(topic, data, flags) {
		if (!this.#ready)
			throw new Error;

		try {
			return this.#mqtt.publish(topic, data, flags);
		}
		catch (e) {
			this.#restart();
			throw e;
		}
	}
	subscribe(topic) {
		if (this.#subscriptions && !this.#subscriptions.includes(topic)) {
			this.#subscriptions.push(topic);
			if (!this.ready)
				return;
		}

		if (!this.#ready)
			throw new Error;

		try {
			return this.#mqtt.subscribe(topic);
		}
		catch (e) {
			this.#restart();
			throw e;
		}
	}
	unsubscribe(topic) {
		if (this.#subscriptions && (this.#subscriptions.indexOf(topic) >= 0)) {
			this.#subscriptions.splice(this.#subscriptions.indexOf(topic), 1);
			if (!this.ready)
				return;
		}

		if (!this.#ready)
			throw new Error;

		try {
			return this.#mqtt.unsubscribe(topic);
		}
		catch (e) {
			this.#restart();
			throw e;
		}
	}
	connect() {
		this.#mqtt = new MQTT(this.#options);
		this.#mqtt.onConnected = () => {
			this.onConnected?.();
		};
		this.#mqtt.onReady = () => {
			Timer.clear(this.#timeout);
			this.#timeout = undefined;

			this.#subscriptions?.forEach(topic => this.#mqtt.subscribe(topic));

			this.#ready = true;
			this.onReady?.();
		};
		this.#mqtt.onMessage = (topic, data) => {
			this.onMessage?.(topic, data);
		};
		this.#mqtt.onClose = () => {
			this.#ready = false;
			this.#mqtt = undefined;
			this.onClose?.();

			if (!this.#closed)
				this.#restart();
		};

		this.#timeout = Timer.set(() => {
			this.#timeout = undefined;
			this.#restart();
		}, 10_000);
	}
	#restart() {
		if (this.#reconnect || this.#wait)
			return;

		this.#ready = false;
		this.#reconnect = Timer.set(() => {
			this.#reconnect = undefined;

			try {
				this.close();
				this.onClose?.();
			}
			catch {
			}

			this.#closed = false;
			this.connect();
		}, 500);
	}
	wait(wait) {
		this.#wait = wait;
		if (wait) {
			Timer.clear(this.#reconnect);
			this.#reconnect = undefined;
		}
		else if (!this.#ready && !this.#timeout && !this.#reconnect)
			this.#restart();
	}
	get id() {
		return this.#options.id;
	}
	set id(value) {
		value = value.toString();
		if ((undefined !== this.#options.id) && (value !== this.#options.id))
			throw new Error("may only set once");

		this.#options.id = value;
	}
	get ready() {
		return this.#ready;
	}
}

export default Connection;
