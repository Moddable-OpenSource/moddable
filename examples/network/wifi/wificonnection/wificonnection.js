/*
* Copyright (c) 2021  Moddable Tech, Inc.
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
	Connection is designed to be used in place of the "wifi" module in projects
	that need a continuously available connection to a Wi-Fi access point.

		- If no connection avaiable at start, retries until one is
		- Automatically attempts to reconnect when connection dropped
		- Disconnects when calling close
		- Supresses redundant WiFi.disconnect messages
		- Callback uses same message constants as "wifi"
		- Getter on "ready" is convenient way to check if Wi-Fi is connected
		- If connection attempt does not succeed with an IP address in 30 seconds, forces disconnect and retries
		- Wait 5 seconds after (unforced) disconnect to ensure clean reconnect
*/

import WiFi from "wifi";
import Timer from "timer";

class Connection extends WiFi {
	#options;
	#callback;
	#reconnect;
	#connecting;
	#state = WiFi.disconnected;

	constructor(options, callback) {
		options = {...options};

		super(options, (msg, code) => {
			if (WiFi.disconnected === msg) {
				if (this.#connecting)
					Timer.clear(this.#connecting);
				this.#connecting = undefined;

				this.#reconnect ??= Timer.set(() => {
					this.#reconnect = undefined;
					WiFi.connect(this.#options);

					this.#connecting = Timer.set(() => {
						this.#connecting = undefined;
						WiFi.disconnect();				// connection attempt failed
					}, 30 * 1000);
				}, 5 * 1000);		// wait a bit. sometimes Wi-Fi stack sends disconnects before finally connecting.

				if (this.#state !== WiFi.disconnected) {		// supress redundant disconnect messages
					this.#state = WiFi.disconnected;
					this.#callback?.(msg, code);
				}

				return;
			}

			if (WiFi.connected === msg) {
				if (this.#reconnect)
					Timer.clear(this.#reconnect);
				this.#reconnect = undefined;
			}
			else if (WiFi.gotIP === msg) {
				if (this.#connecting)
					Timer.clear(this.#connecting);
				this.#connecting = undefined;
			}

			this.#state = msg;

			this.#callback?.(msg, code);
		});

		this.#connecting = Timer.set(() => {
			this.#connecting = undefined;
			WiFi.disconnect();				// connection attempt failed
		}, 30 * 1000);

		this.#callback = callback;
		this.#options = options;
	}
	close() {
		if (this.#reconnect)
			Timer.clear(this.#reconnect);
		this.#reconnect = undefined;

		if (this.#connecting)
			Timer.clear(this.#connecting);
		this.#connecting = undefined;

		WiFi.disconnect();
		super.close();
	}

	get ready() {
		return WiFi.gotIP === this.#state;
	}
}
Connection.scan = WiFi.scan;

export default Connection;
