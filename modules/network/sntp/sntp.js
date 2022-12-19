/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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
	sntp
*/

import {Socket} from "socket";
import Net from "net";
import Timer from "timer";

class SNTP extends Socket {
	#callback;
	#retry;
	#timer;
	#address;

	constructor(dictionary, callback) {
		super({kind: "UDP"});
		this.#callback = callback;
		this.#timer = Timer.repeat(() => {
			if (!--this.#retry)
				return this.failed("no response from SNTP server");

			this.write();
			this.#callback(SNTP.retry);
		}, 5000);
		this.start(dictionary.host);
	}
	start(host) {
		this.#retry = 5;

		Net.resolve(host, (host, address) => {
			if (!this.#timer) return;	// resolution completed after SNTP closed

			if (address) {
				this.#address = address;
				this.write();
				Timer.schedule(this.#timer, 5000, 5000);
			}
			else
				this.failed("cannot resolve " + host);
		});
	}
	failed(message) {
		const host = this.#callback(SNTP.error, message);
		if (host) {
			Timer.schedule(this.#timer, 5000, 5000);
			this.start(host);
		}
		else
			this.close();
	}
	write() {
		if (!this.#address)
			return;

		try {
			super.write(this.#address, 123, packet());
		}
		catch {
			trace("SNTP: ignore UDP write fail\n")
		}
	}
	close() {
		Timer.clear(this.#timer);
		this.#timer = undefined;

		super.close();
	}
	callback(message, value) {
		if (2 !== message)
			return;

		if (48 !== value)
			this.failed("unexpected SNTP packet length");
		else
		if (4 !== (7 & this.read(Number)))	// NTP serrver mode
			this.failed("unexpected SNTP packet first byte");
		else if (0 === this.read(Number))
			return;	// Kiss-o'-Death - "...KoD packets have no protocol significance and are discarded after inspection"
		else {
			this.read(null, 38);
			const time = toNumber(this.read(Number), this.read(Number), this.read(Number), this.read(Number));
			this.close();
			return this.#callback(SNTP.time, time);
		}
	}

	static time = 1;
	static retry = 2;
	static error = -1;
}

function packet() @ "xs_sntp_packet";
function toNumber() @ "xs_sntp_toNumber";

export default SNTP;
